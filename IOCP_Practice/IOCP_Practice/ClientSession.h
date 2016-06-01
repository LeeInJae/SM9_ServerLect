#pragma once
#include "FastSpinLock.h"

#define BUFSIZE 4096

class ClientSession;
class Sessionmanager;
class FastSpinLock;

enum class IOType : short int
{
	IO_NONE,
	IO_SEND,
	IO_RECV,
	IO_RECV_ZERO,
	IO_ACCEPT
};

enum class DisconnectReason : short int
{
	DR_NONE,
	DR_RECV_ZERO,
	DR_ACTIVE,
	DR_CONNECT_ERROR,
	DR_COMPLETION_ERROR,
};

struct OverlappedIOContext
{
	OverlappedIOContext(const ClientSession* owner, IOType ioType) : mSessionObject(owner), mIoType(ioType)
	{
		memset(&mOverlapped, 0, sizeof(OVERLAPPED));
		memset(&mWsaBuf, 0, sizeof(mWsaBuf));
		memset(mBuffer, 0, BUFSIZE);
	}

	OVERLAPPED				mOverlapped;
	const ClientSession*	mSessionObject;
	IOType					mIoType;
	WSABUF					mWsaBuf;
	char					mBuffer[BUFSIZE];

};
class ClientSession
{
public:
	ClientSession();

	ClientSession(SOCKET sock) : mSocket(sock), mConnected(false)
	{
		memset(&mClientAddr, 0, sizeof(SOCKADDR_IN));
	}
	~ClientSession();

	bool OnConnect(SOCKADDR_IN* addr);
	bool IsConnected() const { return mConnected; }

	bool PostRecv() const;
	bool PostSend(const char* buf, int len) const;
	void Disconnect(DisconnectReason reason);

private:
	bool			mConnected;
	SOCKET			mSocket;
	SOCKADDR_IN		mClientAddr;
	FastSpinLock	mLock;

	friend class SessionManager;
};

