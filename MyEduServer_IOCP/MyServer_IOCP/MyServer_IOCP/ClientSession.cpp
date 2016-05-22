#include "stdafx.h"
#include "Exception.h"
#include "MyServer_IOCP.h"
#include "ClientSession.h"


ClientSession::ClientSession( )
{
}


ClientSession::~ClientSession( )
{
}


bool ClientSession::OnConnect( SOCKADDR_IN* Addr )
{
	//������ ��û�� �����尡 MAIN_ACCEPT�����尡 �ƴϸ� �̻��ѰŴϱ� Assert.
	CRASH_ASSERT( LThreadType == EThreadType::THREAD_MAIN_ACCEPT );

	//TODO: �� ���� lock���� ��ȣ �� ��
	
	//nonblocking socket
	u_long Enabled = 1;
	if( ioctlsocket( mStreamSocket , FIONBIO , &Enabled ) == SOCKET_ERROR)
	{
		printf_s( "nonblocking socket setting error" );
		return false;
	}

	//turn off nagle
	int bOptValue = 1;
	if(setsockopt( mStreamSocket , IPPROTO_TCP , TCP_NODELAY , ( const char* )&bOptValue , sizeof( int ) ) == SOCKET_ERROR)
	{
		printf_s( "turn off Nagle setting error" );
		return false;
	}

	int RecvBuffSize = 0;
	if(setsockopt( mStreamSocket , SOL_SOCKET , SO_RCVBUF , ( const char* )&RecvBuffSize , sizeof( RecvBuffSize ) ) == SOCKET_ERROR )
	{
		printf_s( "Recv Buf size setting error" );
		return false;
	}
	

	return true;
}

void ClientSession::OnDisconnect( EDisconnectReason InReason )
{

}

bool ClientSession::PostRecv( ) const
{
	return true;
}

bool ClientSession::PostSend( const char*buff , int len ) const
{
	return true;
}