#include "stdafx.h"
#include "IocpManager.h"
#include "EduServer_IOCP.h"
#include "ClientSession.h"
#include "SessionManager.h"

#define GQCS_TIMEOUT	20

__declspec(thread) int LIoThreadId = 0;
IocpManager* GIocpManager = nullptr;

IocpManager::IocpManager() : mCompletionPort(NULL), mIoThreadCount(2), mListenSocket(NULL)
{
}


IocpManager::~IocpManager()
{
}

bool IocpManager::Initialize()
{
	//TODO: mIoThreadCount = ...;GetSystemInfo����ؼ� set num of I/O threads
	SYSTEM_INFO SystemInfo;
	GetSystemInfo(&SystemInfo);
	mIoThreadCount = SystemInfo.dwNumberOfProcessors * 2; //core ���� 2�� ��ŭ worker thread �� ����( ������ ? )

	/// winsock initializing
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return false;

	/// Create I/O Completion Port
	//TODO: mCompletionPort = CreateIoCompletionPort(...)
	mCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, mIoThreadCount);
	if (mCompletionPort == NULL)
	{
		printf_s("CompletionPort Create Error\n");
	}

	/// create TCP socket
	//TODO: mListenSocket = ...
	mListenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);
	if (mListenSocket == INVALID_SOCKET)
	{
		printf_s("ListenSocket Create Error\n");
	}

	int opt = 1;
	setsockopt(mListenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(int));

	SOCKADDR_IN ServerAddr;
	int SizeServerAddr = sizeof(ServerAddr);
	memset(&ServerAddr, 0, SizeServerAddr);
	if (WSAStringToAddress(L"127.0.0.1:9001", AF_INET, NULL, (SOCKADDR*)&ServerAddr, &SizeServerAddr) == SOCKET_ERROR)
	{
		printf_s("Server Addr Error\n");
	}

	//TODO:  bind
	if (SOCKET_ERROR == bind(mListenSocket, (SOCKADDR*)&ServerAddr, sizeof(ServerAddr)))
		return false;

	return true;
}


bool IocpManager::StartIoThreads()
{
	/// I/O Thread
	for (int i = 0; i < mIoThreadCount; ++i)
	{
		DWORD dwThreadId;
		//TODO: HANDLE hThread = (HANDLE)_beginthreadex...);
		HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, IoWorkerThread, (void *)&dwThreadId, 0, (unsigned int*)&dwThreadId);
	}

	return true;
}


bool IocpManager::StartAcceptLoop()
{
	/// listen
	if (SOCKET_ERROR == listen(mListenSocket, SOMAXCONN))
		return false;


	/// accept loop
	while (true)
	{
		SOCKET acceptedSock = accept(mListenSocket, NULL, NULL);
		if (acceptedSock == INVALID_SOCKET)
		{
			printf_s("accept: invalid socket\n");
			continue;
		}

		SOCKADDR_IN clientaddr;
		int addrlen = sizeof(clientaddr);
		getpeername(acceptedSock, (SOCKADDR*)&clientaddr, &addrlen);

		/// ���� ���� ����ü �Ҵ�� �ʱ�ȭ
		ClientSession* client = GSessionManager->CreateClientSession(acceptedSock);

		/// Ŭ�� ���� ó��
		if (false == client->OnConnect(&clientaddr))
		{
			client->Disconnect(DR_ONCONNECT_ERROR);
			GSessionManager->DeleteClientSession(client);
		}
	}

	return true;
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
	HANDLE hCompletionPort	= GIocpManager->GetComletionPort();
	
	while (true)
	{
		DWORD dwTransferred				= 0;
		OverlappedIOContext* context	= nullptr;
		ClientSession* asCompletionKey	= nullptr;

		int ret = GetQueuedCompletionStatus(hCompletionPort, &dwTransferred, (ULONG_PTR*)&asCompletionKey, (LPOVERLAPPED*)&context, GQCS_TIMEOUT)  ; ///<���⿡�� GetQueuedCompletionStatus(hComletionPort, ..., GQCS_TIMEOUT)�� ������ ������� ����

		/// check time out first 
		if (ret == 0 && GetLastError()==WAIT_TIMEOUT)
			continue;

		if (ret == 0 || dwTransferred == 0)
		{
			/// connection closing
			asCompletionKey->Disconnect(DR_RECV_ZERO);
			GSessionManager->DeleteClientSession(asCompletionKey);
			continue;
		}

		// if (nullptr == context) �� ��� ó��
		if (nullptr == context)
		{
			printf_s( "not dequeue completion packet\n" );
			continue;
		}

		bool completionOk = true;
		switch (context->mIoType)
		{
		case IO_SEND:
			completionOk = SendCompletion(asCompletionKey, context, dwTransferred);
			break;

		case IO_RECV:
			completionOk = ReceiveCompletion(asCompletionKey, context, dwTransferred);
			break;

		default:
			printf_s("Unknown I/O Type: %d\n", context->mIoType);
			break;
		}

		if ( !completionOk )
		{
			/// connection closing
			asCompletionKey->Disconnect(DR_COMPLETION_ERROR);
			GSessionManager->DeleteClientSession(asCompletionKey);
		}

	}

	return 0;
}

bool IocpManager::ReceiveCompletion(const ClientSession* client, OverlappedIOContext* context, DWORD dwTransferred)
{
	/// echo back ó�� client->PostSend()���.
	if (client != nullptr && context != nullptr)
		client->PostSend( context->mWsaBuf.buf , dwTransferred );

	//���ο� I/O �۾��� overlapoped�� ���� �����ϹǷ� ���⼭ ��������Ѵ�.
	delete context;

	return client->PostRecv();
}

bool IocpManager::SendCompletion(const ClientSession* client, OverlappedIOContext* context, DWORD dwTransferred)
{
	/// ���� �� �Ǿ����� Ȯ���ϴ� �� ó��..
	if (client != nullptr && context != nullptr && context->mWsaBuf.len != dwTransferred) 
	{
		return false;
	}
	
	//���ο� I/O �۾��� overlapoped�� ���� �����ϹǷ� ���⼭ ��������Ѵ�.
	delete context;
	return true;
}
