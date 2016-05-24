#include "stdafx.h"
#include "SessionManager.h"
#include "ClientSession.h"
#include "MyServer_IOCP.h"
#include "IOCPManager.h"

#define GQCS_TIMEOUT 20

__declspec( thread ) int LThreadID = -1;
IOCPManager* GIOCPManager = nullptr;

IOCPManager::~IOCPManager( )
{

}


bool IOCPManager::Initialize( )
{
	SYSTEM_INFO SysInfo;
	GetSystemInfo( &SysInfo );
	mIOThreadCount = SysInfo.dwNumberOfProcessors * 2; //여기도 확인

	WSADATA WsaData;
	if(WSAStartup( MAKEWORD( 2 , 2 ) , &WsaData ) != 0)
	{
		printf_s( "winsock initializing error" );
		return false;
	}


	mCompletionPort = CreateIoCompletionPort( INVALID_HANDLE_VALUE , NULL , 0 , mIOThreadCount );
	if(mCompletionPort == NULL)
	{
		printf_s( "Create CompletionPort error" );
		return false;
	}

	mListenSocket = WSASocket( AF_INET , SOCK_STREAM , IPPROTO_TCP , NULL , NULL , WSA_FLAG_OVERLAPPED );
	if(mListenSocket == INVALID_SOCKET)
	{
		printf_s( "Create Listen Socket error" );
		return false;
	}

	int opt = 1;
	if(setsockopt( mListenSocket , SOL_SOCKET , SO_REUSEADDR , ( const char* )&opt , sizeof( int ) ) == SOCKET_ERROR)
	{
		printf_s( "Listen Socekt REUSEADDR error" );
		return false;
	}

	SOCKADDR_IN ServerAddr;
	memset( &ServerAddr , 0 , sizeof( SOCKADDR_IN ) );
	int SizeServerAddr = sizeof( SOCKADDR_IN );

	if( WSAStringToAddress( SERVER_ADDRESS , AF_INET , NULL , ( SOCKADDR* )&ServerAddr , &SizeServerAddr ) == SOCKET_ERROR)
	{
		printf_s( "Server Address Setting Error" );
		return false;
	}	

	if(bind( mListenSocket , ( SOCKADDR* )&ServerAddr , SizeServerAddr ) == SOCKET_ERROR)
	{
		printf_s( "bind Error" );
		return false;
	}

	return true;
}

void IOCPManager::Finalize( )
{
	CloseHandle( mCompletionPort );
	WSACleanup( );
}

bool IOCPManager::StartIoThreads( )
{
	for(int i = 0; i < mIOThreadCount; ++i)
	{
		DWORD dwThreadID;
		HANDLE hThread = (HANDLE)_beginthreadex( NULL , 0 , IOWorkerThread , ( void* )&dwThreadID , 0 , ( unsigned int* )&dwThreadID );
	}

	return true;
}

bool IOCPManager::StartAcceptLoop( )
{
	if(listen( mListenSocket , SOMAXCONN ) == SOCKET_ERROR)
	{
		printf_s( "Server Listen Error" );
		return false;
	}

	while(true)
	{
		SOCKET StreamSocket = accept( mListenSocket , NULL , NULL );
		if(StreamSocket == INVALID_SOCKET)
		{
			printf_s( "Not Accept" );
			continue;
		}

		SOCKADDR_IN ClientAddr;
		int SizeClientAddr = sizeof( SOCKADDR_IN );
		if(getpeername( StreamSocket , ( SOCKADDR* )&ClientAddr , &SizeClientAddr ) == SOCKET_ERROR)
		{
			printf_s( "GetPeerName error" );
			continue;
		}

		ClientSession* ClientSessionObject = GSessionManager->CreateClientSession( StreamSocket );
		if(ClientSessionObject == nullptr)
		{
			printf_s( "Create ClienSession error" );
			continue;
		}

		if(ClientSessionObject->OnConnect( &ClientAddr ) == false)
		{
			ClientSessionObject->OnDisconnect( EDisconnectReason::DR_CONNECT_ERROR );
			GSessionManager->DeleteClientSession( ClientSessionObject );
		}
	}

	return true;
}

unsigned int WINAPI IOCPManager::IOWorkerThread( LPVOID lpParam )
{
	LThreadType = EThreadType::THREAD_IO_WORKER;
	LThreadID	= reinterpret_cast<int>(lpParam);
	
	HANDLE CompletionPort = GIOCPManager->GetCompletionPort( );

	while(true)
	{
		DWORD dwTransferred = 0;
		OverlappedIOContext*	IOContext		= nullptr;
		ClientSession*			AsCompletionKey = nullptr;

		int GQCS_RetVal = GetQueuedCompletionStatus( CompletionPort , &dwTransferred ,(ULONG_PTR*)&AsCompletionKey , (LPOVERLAPPED*)&IOContext , GQCS_TIMEOUT );

		//check time out
		if(GQCS_RetVal == 0 && GetLastError() == WAIT_TIMEOUT)
		{
			continue;
		}

		//recv zero
		if(GQCS_RetVal == 0 || dwTransferred == 0)
		{
			AsCompletionKey->OnDisconnect( EDisconnectReason::DR_RECV_ZERO );
			GSessionManager->DeleteClientSession( AsCompletionKey );
			if(IOContext != nullptr)
			{
				delete IOContext;
				IOContext = nullptr;
			}

			continue;
		}

		if(IOContext == nullptr)
		{
			printf_s( "Not dequeue Completion packet\n" );
			continue;
		}

		bool CompletionOk = true;
		switch(IOContext->IOType)
		{
		case EIOType::IO_SEND:
			CompletionOk = SendCompletion( AsCompletionKey , IOContext , dwTransferred );
			break;
		case EIOType::IO_RECV:
			CompletionOk = ReceiveCompletion( AsCompletionKey , IOContext , dwTransferred );
			break;
		default:
			break;
		}

		if(CompletionOk == false)
		{
			AsCompletionKey->OnDisconnect( EDisconnectReason::DR_COMPLETION_ERROR );
			GSessionManager->DeleteClientSession( AsCompletionKey );
		}
	}

	return 0;
}

bool IOCPManager::ReceiveCompletion( const ClientSession* InClientSession , OverlappedIOContext* InOverlappedIOContext , DWORD dwTransferred )
{
	//echo back
	if(InClientSession == nullptr || InOverlappedIOContext == nullptr)
		return false;

	InClientSession->PostSend( InOverlappedIOContext->mBuf , dwTransferred );
	delete InOverlappedIOContext;
	
	return InClientSession->PostRecv( );
}

bool IOCPManager::SendCompletion( const ClientSession* InClientSession , OverlappedIOContext* InOverlappedIOContext , DWORD dwTransferred )
{
	if(InClientSession == nullptr || InOverlappedIOContext == nullptr)
		return false;

	if(InOverlappedIOContext->mWsaBuf.len != dwTransferred)
		return false;

	delete InOverlappedIOContext;

	return true;
}