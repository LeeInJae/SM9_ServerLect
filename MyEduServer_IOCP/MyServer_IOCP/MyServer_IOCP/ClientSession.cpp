#include "stdafx.h"
#include "Exception.h"
#include "MyServer_IOCP.h"
#include "SessionManager.h"
#include "IOCPManager.h"
#include "ClientSession.h"


ClientSession::ClientSession( )
{
}


ClientSession::~ClientSession( )
{
}


bool ClientSession::OnConnect( SOCKADDR_IN* Addr )
{
	//연결을 요청한 스레드가 MAIN_ACCEPT스레드가 아니면 이상한거니까 Assert.
	CRASH_ASSERT( LThreadType == EThreadType::THREAD_MAIN_ACCEPT );

	//TODO: 이 영역 lock으로 보호 할 것
	FastSpinLockGuard EnterLock( mLock );
	//nonblocking socket
	u_long Enabled = 1;
	if( ioctlsocket( mStreamSocket , FIONBIO , &Enabled ) == SOCKET_ERROR)
	{
		printf_s( "Nonblocking socket setting error" );
		return false;
	}

	//turn off nagle
	int bOptValue = 1;
	if(setsockopt( mStreamSocket , IPPROTO_TCP , TCP_NODELAY , ( const char* )&bOptValue , sizeof( int ) ) == SOCKET_ERROR)
	{
		printf_s( "Turn off Nagle setting error" );
		return false;
	}

	int RecvBuffSize = 0;
	if(setsockopt( mStreamSocket , SOL_SOCKET , SO_RCVBUF , ( const char* )&RecvBuffSize , sizeof( RecvBuffSize ) ) == SOCKET_ERROR )
	{
		printf_s( "Recv Buf size setting error" );
		return false;
	}

	HANDLE handle = CreateIoCompletionPort( (HANDLE)mStreamSocket , GIOCPManager->GetCompletionPort( ) , ( ULONG_PTR )this , 0 );
	if(handle != GIOCPManager->GetCompletionPort( ))
	{
		printf_s( "Connect StreamSocket with CompletionProt error" );
		return false;
	}

	memcpy( &mClientAddr , Addr , sizeof( SOCKADDR_IN ) );
	mConnected = true;

	
	CHAR AddrStr[ 30 ];
	int SizeAddrStr = sizeof( AddrStr );
	inet_ntop( AF_INET , ( void* )&mClientAddr.sin_addr , AddrStr , SizeAddrStr );
	printf_s( "Client Connected !!!!!!!!!!!!!!!!  : IP : %s, PORT = %d \n" , AddrStr , ntohs( mClientAddr.sin_port ) );

	GSessionManager->IncreaseConnectionCount( );

	return PostRecv();
}

void ClientSession::OnDisconnect( EDisconnectReason InReason )
{
	//todo : lock!
	FastSpinLockGuard EnterLock( mLock );

	if(mConnected == false)
		return;

	//linger option
	LINGER LingerOption;
	LingerOption.l_linger = 1;
	LingerOption.l_onoff = 0;
	if(setsockopt( mStreamSocket , SOL_SOCKET , SO_LINGER , ( const char* )&LingerOption , sizeof( LINGER ) ) == SOCKET_ERROR)
	{
		printf_s( "Linger Option Error" );
	}

	//printf_s( "[DEBUG] Client Disconnected: Reason=%d IP=%s, PORT=%d \n" , dr , inet_ntoa( mClientAddr.sin_addr ) , ntohs( mClientAddr.sin_port ) );
	
	GSessionManager->DecreaseConnectionCount( );
	closesocket( mStreamSocket );
	
	mConnected = false;
}

bool ClientSession::PostRecv( ) const
{
	if(mConnected == false)
		return false;

	auto RecvContext = new OverlappedIOContext( this , EIOType::IO_RECV );
	if(RecvContext == nullptr)
	{
		printf_s( "RecvIOContext create error" );
		return false;
	}

	RecvContext->mWsaBuf.buf = RecvContext->mBuf;
	RecvContext->mWsaBuf.len = sizeof(RecvContext->mBuf);

	DWORD RecvBytes = 0 , RecvFlag = 0;
	if(WSARecv( mStreamSocket , &(RecvContext->mWsaBuf) , 1 , &RecvBytes , &RecvFlag , &( RecvContext->mOverlapped ) , NULL ) == SOCKET_ERROR)
	{
		if(WSAGetLastError( ) != WSA_IO_PENDING)
		{
			printf_s( "WSARecv Error" );
		
			return false;
		}
	}

	return true;
}

bool ClientSession::PostSend( const char* buff , int len ) const
{
	if(mConnected == false)
		return false;

	auto SendContext = new OverlappedIOContext( this , EIOType::IO_SEND );
	if(SendContext == nullptr)
	{
		printf_s( "SendIOContext create error" );
		return false;
	}
	
	memcpy_s( SendContext->mBuf , BUF_SIZE, buff , len );

	SendContext->mWsaBuf.buf = SendContext->mBuf;
	SendContext->mWsaBuf.len = len;

	DWORD SendBytes = 0 , SendFlag = 0;
	if(WSASend( mStreamSocket , &( SendContext->mWsaBuf ) , 1 , &SendBytes , SendFlag , &(SendContext->mOverlapped ) , NULL ) == SOCKET_ERROR)
	{
		if(WSAGetLastError( ) != WSA_IO_PENDING)
		{
			printf_s( "WSASend Error" );

			return false;
		}
	}

	return true;
}