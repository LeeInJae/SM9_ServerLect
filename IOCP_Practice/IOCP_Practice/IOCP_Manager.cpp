#include "stdafx.h"
#include "IOCP_Manager.h"
#include "ClientSession.h"
#include "SessionManager.h"
#include "IOCP_Practice.h"

#define GQCS_TIMEOUT	20

__declspec(thread) int LIoThreadID = 0;
IOCP_Manager* GIOCP_Manager = nullptr;

IOCP_Manager::IOCP_Manager()
{
}


IOCP_Manager::~IOCP_Manager()
{
}

bool IOCP_Manager::Initialize()
{
	SYSTEM_INFO SystemInfo;
	GetSystemInfo(&SystemInfo);
	mIoThreadCount = SystemInfo.dwNumberOfProcessors * 2;

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		printf_s("WSAStartup Error\n");
		return false;
	}

	mCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, mIoThreadCount);
	if (mCompletionPort == NULL)
	{
		printf_s("Create CompletionPort error\n");
		return false;
	}

	mListenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);
	if (mListenSocket == INVALID_SOCKET)
	{
		printf_s("Create ListenSocket Error\n");
		return false;
	}

	int opt = 1;
	setsockopt(mListenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(int));

	SOCKADDR_IN ServerAddr;
	int SizeServerAddr = sizeof(SOCKADDR_IN);
	memset(&ServerAddr, 0, SizeServerAddr);
	if (WSAStringToAddress(L"127.0.0.1:9001", AF_INET, NULL, (SOCKADDR*)&ServerAddr, &SizeServerAddr) == SOCKET_ERROR)
	{
		printf_s("ServerAddress Setting Error\n");
		return false;
	}

	if (bind(mListenSocket, (SOCKADDR*)&ServerAddr, SizeServerAddr) == SOCKET_ERROR)
	{
		printf_s("Bind Error\n");
		return false;
	}

	return true;
}

void IOCP_Manager::Finalize()
{
	CloseHandle(mCompletionPort);
	WSACleanup();
}

bool IOCP_Manager::StartIoThreads()
{
	for (int i = 0; i < mIoThreadCount; ++i)
	{
		DWORD dwThreadId;
		HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, IoWorkerThread, (void*)&dwThreadId, 0, (unsigned int*)&dwThreadId);
	}

	return true;
}

bool IOCP_Manager::StartAcceptLoop()
{
	if (listen(mListenSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		printf_s("Listen Error\n");
		return false;
	}

	while (true)
	{
		SOCKET acceptedSock = accept(mListenSocket, NULL, NULL);
		if (acceptedSock == SOCKET_ERROR)
		{
			printf_s("accept : invalid socket\n");
			continue;
		}

		SOCKADDR_IN clientaddr;
		int addrlen = sizeof(SOCKADDR_IN);
		getpeername(acceptedSock, (SOCKADDR*)&clientaddr, &addrlen);

		ClientSession* client = GSessionManager->CreateClientSession(acceptedSock);

		if (client->OnConnect(&clientaddr) == false)
		{
			client->Disconnect(DisconnectReason::DR_CONNECT_ERROR);
			GSessionManager->DeleteClientSession(client);
		}
	}
	return true;
}

unsigned int WINAPI	IOCP_Manager::IoWorkerThread(LPVOID lpParam)
{
	LThreadType					= THREAD_TYPE::THREAD_IO_WORKER;

	LIoThreadID					= reinterpret_cast<int>(lpParam);
	HANDLE hCompletionPort		= GIOCP_Manager->GetCompletionPort();
	while (true)
	{
		DWORD					dwTransferred	= 0;
		OverlappedIOContext*	context			= nullptr;
		ClientSession*			asCompletionKey = nullptr;

		int ret = GetQueuedCompletionStatus(hCompletionPort, &dwTransferred, (ULONG_PTR*)&asCompletionKey, (LPOVERLAPPED*)&context, GQCS_TIMEOUT);
		if (ret == 0 && GetLastError() == WAIT_TIMEOUT)
		{
			continue;
		}

		if (ret == 0 || dwTransferred == 0)
		{
			asCompletionKey->Disconnect(DisconnectReason::DR_RECV_ZERO);
			GSessionManager->DeleteClientSession(asCompletionKey);
			continue;
		}

		if (context == nullptr)
		{
			printf_s("not dequeue completion packet\n");
			continue;
		}
		
		bool completionOk = true;
		switch (context->mIoType)
		{
		case IOType::IO_SEND:
			completionOk = SendCompletion(asCompletionKey, context, dwTransferred);
			break;

		case IOType::IO_RECV:
			completionOk = ReceiveCompletion(asCompletionKey, context, dwTransferred);
			break;

		default:
			printf_s("Unkonw I/O type: %d\n", context->mIoType);
			break;
		}

		if (!completionOk)
		{
			asCompletionKey->Disconnect(DisconnectReason::DR_COMPLETION_ERROR);
			GSessionManager->DeleteClientSession(asCompletionKey);
		}
	}

	return 0;
}

bool IOCP_Manager::ReceiveCompletion(const ClientSession* client, OverlappedIOContext* context, DWORD dwTransferred)
{
	if (client == nullptr || context == nullptr)
		return false;

	client->PostSend(context->mWsaBuf.buf, dwTransferred);
	delete context;

	return client->PostRecv();
}

bool IOCP_Manager::SendCompletion(const ClientSession* client, OverlappedIOContext* context, DWORD dwTransferred)
{
	if (client == nullptr || context == nullptr)
		return false;

	if (context->mWsaBuf.len != dwTransferred)
		return false;

	delete context;

	return true;
}