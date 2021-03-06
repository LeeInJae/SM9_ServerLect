#include "stdafx.h"
#include "Exception.h"
#include "EduServer_IOCP.h"
#include "ClientSession.h"
#include "IocpManager.h"
#include "SessionManager.h"

bool ClientSession::OnConnect(SOCKADDR_IN* addr)
{
	//Question : 왜 LOCK?? 
	//TODO: 이 영역 lock으로 보호 할 것

	//local varivable로 LockGauard 변수를 만든다. 이때 spinlock 객체를 통해서 락을 건다.
	FastSpinlockGuard EnterLock(mLock);
	CRASH_ASSERT(LThreadType == THREAD_MAIN_ACCEPT);

	/// make socket non-blocking
	u_long arg = 1 ;
	ioctlsocket(mSocket, FIONBIO, &arg) ;

	/// turn off nagle
	int opt = 1 ;
	setsockopt(mSocket, IPPROTO_TCP, TCP_NODELAY, (const char*)&opt, sizeof(int)) ;

	opt = 0;
	if (SOCKET_ERROR == setsockopt(mSocket, SOL_SOCKET, SO_RCVBUF, (const char*)&opt, sizeof(int)) )
	{
		printf_s("[DEBUG] SO_RCVBUF change error: %d\n", GetLastError()) ;
		return false;
	}

	HANDLE handle = CreateIoCompletionPort( (HANDLE)mSocket, GIocpManager->GetComletionPort(), (ULONG_PTR)this, 0); //TODO: 여기에서 CreateIoCompletionPort((HANDLE)mSocket, ...);사용하여 연결할 것
	if (handle != GIocpManager->GetComletionPort())
	{
		printf_s("[DEBUG] CreateIoCompletionPort error: %d\n", GetLastError());
		return false;
	}

	memcpy(&mClientAddr, addr, sizeof(SOCKADDR_IN));
	mConnected = true ;

	printf_s("[DEBUG] Client Connected: IP=%s, PORT=%d\n", inet_ntoa(mClientAddr.sin_addr), ntohs(mClientAddr.sin_port));

	GSessionManager->IncreaseConnectionCount();
	
	return PostRecv() ;
}

void ClientSession::Disconnect(DisconnectReason dr)
{
	//TODO: 이 영역 lock으로 보호할 것
	//Question : 왜 LOCK?? 

	//아래애 보면 GSessionManager에 접근하고 있다. 이는 싱글톤으로써 글로벌하게 다른 스레드에서도 접근이 가능하다. 따라서 lock을 걸어주어야 한다.
	FastSpinlockGuard EnterLock( mLock );
	if ( !IsConnected() )
		return ;
	
	//lingerOption을 통하여, send buffer에 데이터가 남아있더라도 바로 종료.
	LINGER lingerOption ;
	lingerOption.l_onoff = 1;
	lingerOption.l_linger = 0;

	/// no TCP TIME_WAIT
	if (SOCKET_ERROR == setsockopt(mSocket, SOL_SOCKET, SO_LINGER, (char*)&lingerOption, sizeof(LINGER)) )
	{
		printf_s("[DEBUG] setsockopt linger option error: %d\n", GetLastError());
	}

	printf_s("[DEBUG] Client Disconnected: Reason=%d IP=%s, PORT=%d \n", dr, inet_ntoa(mClientAddr.sin_addr), ntohs(mClientAddr.sin_port));
	
	GSessionManager->DecreaseConnectionCount();

	closesocket(mSocket) ;

	mConnected = false ;
}

bool ClientSession::PostRecv() const
{
	if (!IsConnected())
		return false;

	OverlappedIOContext* recvContext	= new OverlappedIOContext(this, IO_RECV);
	recvContext->mSessionObject			= this;
	recvContext->mWsaBuf.buf			= recvContext->mBuffer;
	recvContext->mWsaBuf.len			= sizeof( recvContext->mBuffer );
	//TODO: WSARecv 사용하여 구현할 것
	DWORD recvBytes = 0 , recvFlag = 0;
	if ( WSARecv( mSocket , &( recvContext->mWsaBuf ) , 1 , &recvBytes , &recvFlag , &( recvContext->mOverlapped ) , NULL ) == SOCKET_ERROR )
	{
		if ( WSAGetLastError( ) != WSA_IO_PENDING )
		{
			printf_s( "PostRecv Error\n" );

			return false;
		}
	}

	return true;
}

bool ClientSession::PostSend(const char* buf, int len) const
{
	if (!IsConnected())
		return false;

	OverlappedIOContext* sendContext = new OverlappedIOContext(this, IO_SEND);

	/// copy for echoing back..
	memcpy_s(sendContext->mBuffer, BUFSIZE, buf, len);
	sendContext->mWsaBuf.buf	= sendContext->mBuffer;
	sendContext->mWsaBuf.len	= len;
	sendContext->mSessionObject = this;
	//TODO: WSASend 사용하여 구현할 것
	DWORD sendBytes = 0 , sendFlag = 0;
	if ( WSASend( mSocket , &( sendContext->mWsaBuf ) , 1 , &sendBytes , sendFlag , &( sendContext->mOverlapped ) , NULL ) == SOCKET_ERROR )
	{
		printf_s( "WSASend Error\n" );

		return false;
	}

	return true;
}