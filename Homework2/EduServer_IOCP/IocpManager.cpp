#include "stdafx.h"
#include "Exception.h"
#include "IocpManager.h"
#include "EduServer_IOCP.h"
#include "ClientSession.h"
#include "SessionManager.h"

#define GQCS_TIMEOUT	INFINITE //20

__declspec(thread) int	LIoThreadId		= 0;
IocpManager*			GIocpManager	= nullptr;

//TODO AcceptEx DisconnectEx 함수 사용할 수 있도록 구현.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL DisconnectEx( SOCKET hSocket , LPOVERLAPPED lpOverlapped , DWORD dwFlags , DWORD reserved )
{
	if (DisconnectExFunctionPtr != nullptr)
	{
		if (DisconnectExFunctionPtr(hSocket, lpOverlapped, dwFlags, reserved) == false)
		{
			int err = WSAGetLastError();
			if (err != ERROR_IO_PENDING)
			{
				//DisconnectEx error
				return false;
			}
		}
		return true;
	}
	return false;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// 참고: 최신 버전의 Windows SDK에서는 그냥 구현되어 있음
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CustomAcceptEx( SOCKET sListenSocket , SOCKET sAcceptSocket , PVOID lpOutputBuffer , DWORD dwReceiveDataLength ,
	DWORD dwLocalAddressLength , DWORD dwRemoteAddressLength , LPDWORD lpdwBytesReceived , LPOVERLAPPED lpOverlapped )
{
	if (AcceptExFunctionPtr != nullptr)
	{
		if (AcceptExFunctionPtr(sListenSocket, sAcceptSocket, lpOutputBuffer, dwReceiveDataLength, dwLocalAddressLength, dwRemoteAddressLength, lpdwBytesReceived, lpOverlapped) == false)
		{
			int err = WSAGetLastError();
			if (err != ERROR_IO_PENDING)
			{
				//AcceptEx Error
				return false;
			}
		}
		return true;
	}
	return false;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

IocpManager::IocpManager() : mCompletionPort(NULL), mIoThreadCount(2), mListenSocket(NULL)
{	
}


IocpManager::~IocpManager()
{
}

bool IocpManager::Initialize()
{
	/// set num of I/O threads
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	mIoThreadCount = si.dwNumberOfProcessors * 2;//Question core * 2 만큼 있어야 되지 않나?

	/// winsock initializing
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return false;

	/// Create I/O Completion Port
	mCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (mCompletionPort == NULL)
		return false;

	/// create TCP socket
	mListenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (mListenSocket == INVALID_SOCKET)
		return false;

	HANDLE handle = CreateIoCompletionPort((HANDLE)mListenSocket, mCompletionPort, 0, mIoThreadCount);
	if (handle != mCompletionPort)
	{
		printf_s("[DEBUG] listen socket IOCP register error: %d\n", GetLastError());
		return false;
	}

	int opt = 1;
	setsockopt(mListenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(int));

	/// bind
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family		= AF_INET;
	serveraddr.sin_port			= htons(LISTEN_PORT);
	serveraddr.sin_addr.s_addr	= htonl(INADDR_ANY);

	if (SOCKET_ERROR == bind(mListenSocket, (SOCKADDR*)&serveraddr, sizeof(serveraddr)))
		return false;

	//TODO : WSAIoctl을 이용하여 AcceptEx, DisconnectEx 함수 사용가능하도록 하는 작업..
	/////////////////////////////////////////////////////////////////////////////////
	//Question : WSAIoctl을 통한 함수 로드시, 소켓파라미터에는 무엇을 넣어주어야하는건가 acceptEx는 listen socket을 넣는다고 쳐도, disconnectEx는 ?
	//			그냥 함수로드의 목적이라면, 아무 소켓이라도 상관은 없나?(일단 NULL은 안됨)
	GUID			GuidAcceptEx	= WSAID_ACCEPTEX;
	DWORD			AcceptExDwBytes = 0;
	if (WSAIoctl(mListenSocket, SIO_GET_EXTENSION_FUNCTION_POINTER, &GuidAcceptEx, sizeof(GuidAcceptEx), &AcceptExFunctionPtr, sizeof(AcceptExFunctionPtr), &AcceptExDwBytes, NULL, NULL) != 0)
	{
		//Get AcceptEx Pointer failed
		return false;
	}
	
	GUID				GuidDisconnectEx	= WSAID_DISCONNECTEX;
	DWORD				DisconnectExDwBytes = 0;
	if (WSAIoctl(mListenSocket, SIO_GET_EXTENSION_FUNCTION_POINTER, &GuidDisconnectEx, sizeof(GuidDisconnectEx), &DisconnectExFunctionPtr, sizeof(DisconnectExFunctionPtr), &DisconnectExDwBytes, NULL, NULL) != 0)
	{
		//Get DisconnectEx Pointer failed
		return false;
	}
	/////////////////////////////////////////////////////////////////////////////////

	/// make session pool
	GSessionManager->PrepareSessions();

	return true;
}


bool IocpManager::StartIoThreads()
{
	/// I/O Thread
	for (int i = 0; i < mIoThreadCount; ++i)
	{
		DWORD dwThreadId;
		HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, IoWorkerThread, (LPVOID)(i+1), 0, (unsigned int*)&dwThreadId);
		if (hThread == NULL)
			return false;
	}

	return true;
}


void IocpManager::StartAccept()
{
	/// listen
	if (SOCKET_ERROR == listen(mListenSocket, SOMAXCONN))
	{
		printf_s("[DEBUG] listen error\n");
		return;
	}
		
	while (GSessionManager->AcceptSessions())
	{
		Sleep(100);
	}

}


void IocpManager::Finalize()
{
	CloseHandle(mCompletionPort);

	/// winsock finalizing
	WSACleanup();

}

unsigned int WINAPI IocpManager::IoWorkerThread(LPVOID lpParam)
{
	LThreadType				= THREAD_IO_WORKER;

	LIoThreadId				= reinterpret_cast<int>(lpParam);
	
	HANDLE hComletionPort	= GIocpManager->GetComletionPort();
	
	
	while (true)
	{
		DWORD					dwTransferred		= 0;
		OverlappedIOContext*	context				= nullptr;
		ULONG_PTR				completionKey		= 0;

		int ret = GetQueuedCompletionStatus(hComletionPort, &dwTransferred, (PULONG_PTR)&completionKey, (LPOVERLAPPED*)&context, GQCS_TIMEOUT);

		ClientSession* theClient = context ? context->mSessionObject : nullptr ;
		
		if (ret == 0 || dwTransferred == 0)
		{
			int gle = GetLastError();

			//TODO: check time out first ... GQCS 타임 아웃의 경우는 어떻게?
			////////////////////////////////////////////////
			//time out 이면 아직 IO작업이 끝나지 않았다는 거니까 연결을 끊으면 안되고, I/O 작업이 완료되길 다시 기다린다.
			if ( gle == WAIT_TIMEOUT )
			{
				continue;
			}
			////////////////////////////////////////////////

			///////////////////////////
			if (context == nullptr)
			{
				continue;
			}
			///////////////////////////
			if (context->mIoType == IO_RECV || context->mIoType == IO_SEND )
			{
				CRASH_ASSERT(nullptr != theClient);
			
				theClient->DisconnectRequest(DR_COMPLETION_ERROR);

				DeleteIoContext(context);

				continue;
			}
		}

		CRASH_ASSERT(nullptr != theClient);
	
		bool completionOk = false;
		switch (context->mIoType)
		{
		case IO_DISCONNECT:
			theClient->DisconnectCompletion(static_cast<OverlappedDisconnectContext*>(context)->mDisconnectReason);
			completionOk = true;
			break;

		case IO_ACCEPT:
			theClient->AcceptCompletion();
			completionOk = true;
			break;

		case IO_RECV_ZERO:
			completionOk = PreReceiveCompletion(theClient, static_cast<OverlappedPreRecvContext*>(context), dwTransferred);
			break;

		case IO_SEND:
			completionOk = SendCompletion(theClient, static_cast<OverlappedSendContext*>(context), dwTransferred);
			break;

		case IO_RECV:
			completionOk = ReceiveCompletion(theClient, static_cast<OverlappedRecvContext*>(context), dwTransferred);
			break;

		default:
			printf_s("Unknown I/O Type: %d\n", context->mIoType);
			CRASH_ASSERT(false);
			break;
		}

		if ( !completionOk )
		{
			/// connection closing
			theClient->DisconnectRequest(DR_IO_REQUEST_ERROR);
		}

		DeleteIoContext(context);
	}

	return 0;
}

bool IocpManager::PreReceiveCompletion(ClientSession* client, OverlappedPreRecvContext* context, DWORD dwTransferred)
{
	/// real receive...
	return client->PostRecv();
}

bool IocpManager::ReceiveCompletion(ClientSession* client, OverlappedRecvContext* context, DWORD dwTransferred)
{
	client->RecvCompletion(dwTransferred);

	/// echo back
	return client->PostSend();
}

bool IocpManager::SendCompletion(ClientSession* client, OverlappedSendContext* context, DWORD dwTransferred)
{
	client->SendCompletion(dwTransferred);

	if (context->mWsaBuf.len != dwTransferred)
	{
		printf_s("Partial SendCompletion requested [%d], sent [%d]\n", context->mWsaBuf.len, dwTransferred) ;
		return false;
	}
	
	/// zero receive
	return client->PreRecv();
}


