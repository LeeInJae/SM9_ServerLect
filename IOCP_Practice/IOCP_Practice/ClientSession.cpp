#include "stdafx.h"
#include "Exception.h"
#include "IOCP_Practice.h"
#include "IOCP_Manager.h"
#include "SessionManager.h"
#include "ClientSession.h"
#include <Ws2tcpip.h>

ClientSession::ClientSession()
{
}


ClientSession::~ClientSession()
{
}

bool ClientSession::OnConnect(SOCKADDR_IN* addr)
{
	FastSpinLockGuard CustomSpinLock(mLock);
	{
		CRASH_ASSERT(LThreadType == THREAD_TYPE::THREAD_MAIN_ACCEPT);

		u_long arg = 1;
		ioctlsocket(mSocket, FIONBIO, &arg);

		int opt = 1;
		setsockopt(mSocket, IPPROTO_TCP, TCP_NODELAY, (const char*)&opt, sizeof(int));

		opt = 0;
		if (setsockopt(mSocket, SOL_SOCKET, SO_RCVBUF, (const char*)&opt, sizeof(int)) == SOCKET_ERROR)
		{
			printf_s("[DEBUG] SO_RECVBUF change error : %d\n", GetLastError());
			return false;
		}

		HANDLE handle = CreateIoCompletionPort((HANDLE)mSocket, GIOCP_Manager->GetCompletionPort(), (ULONG_PTR)this, 0); 
		if (handle != GIOCP_Manager->GetCompletionPort())
		{
			printf_s("[DEBUG] CreatIoCompletionPort error : %d\n", GetLastError());
			return false;
		}

		memcpy(&mClientAddr, addr, sizeof(SOCKADDR_IN));
		mConnected = true;

		CHAR AddrStr[30];
		int SizeAddrStr = sizeof(AddrStr);
		inet_ntop(AF_INET, (void*)&mClientAddr.sin_addr, AddrStr, SizeAddrStr);
		printf_s("Client Connected !!!!!!!!!!!!!!!!  : IP : %s, PORT = %d \n", AddrStr, ntohs(mClientAddr.sin_port));

		//printf_s("[DEBUG] Client Connected: IP=%s, PORT=%d\n", inet_ntoa(mClientAddr.sin_addr), ntohs(mClientAddr.sin_port));

		GSessionManager->IncreaseConnectionCount();

		return PostRecv();
	}
}

bool ClientSession::PostRecv() const
{
	if(!IsConnected())
		return false;

	OverlappedIOContext* recvContext	= new OverlappedIOContext(this, IOType::IO_RECV);
	recvContext->mSessionObject			= this;
	recvContext->mWsaBuf.buf			= recvContext->mBuffer;
	recvContext->mWsaBuf.len			= sizeof(recvContext->mBuffer);

	DWORD recvBytes = 0, recvFlag = 0;
	if (WSARecv(mSocket, &(recvContext->mWsaBuf), 1, &recvBytes, &recvFlag, &(recvContext->mOverlapped), NULL) == SOCKET_ERROR)
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			printf_s("PostRecv Error\n");
			return false;
		}
	}

	return true;
}
bool ClientSession::PostSend(const char* buf, int len) const
{
	if (!IsConnected())
		return false;

	OverlappedIOContext* sendContext = new OverlappedIOContext(this, IOType::IO_SEND);
	if (sendContext == nullptr)
		return false;

	memcpy_s(sendContext->mBuffer, BUFSIZE, buf, len);
	sendContext->mWsaBuf.buf		= sendContext->mBuffer;
	sendContext->mWsaBuf.len		= len;
	sendContext->mSessionObject		= this;

	DWORD sendBytes = 0, sendFlag = 0;
	if (WSASend(mSocket, &(sendContext->mWsaBuf), 1, &sendBytes, sendFlag, &(sendContext->mOverlapped), NULL) == SOCKET_ERROR)
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			printf_s("WSASend Error\n");
			return false;
		}
	}

	return true;
}
void ClientSession::Disconnect(DisconnectReason reason)
{
	FastSpinLockGuard CustomSpinLock(mLock);
	{
		if (!IsConnected())
			return;

		LINGER lingerOption;
		lingerOption.l_onoff	= 1;
		lingerOption.l_linger	= 0;

		if (setsockopt(mSocket, SOL_SOCKET, SO_LINGER, (char*)&lingerOption, sizeof(LINGER)) == SOCKET_ERROR)
		{
			printf_s("[DEBUG] setsockopt linger option error: %d\n", GetLastError());
		}

		//printf_s("[DEBUG] Client Disconnected: Reason=%d IP=%s, PORT=%d \n", reason, inet_ntoa(mClientAddr.sin_addr), ntohs(mClientAddr.sin_port));

		GSessionManager->DecreaseConnectionCount();
		closesocket(mSocket);
		mConnected = false;
	}
}