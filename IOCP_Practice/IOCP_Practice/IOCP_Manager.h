#pragma once

class	ClientSession;
struct	OverlappedIOContext;

class IOCP_Manager
{
public:
	IOCP_Manager();
	~IOCP_Manager();

	bool Initialize();
	void Finalize();

	bool StartIoThreads();
	bool StartAcceptLoop();

	HANDLE	GetCompletionPort() { return mCompletionPort; }
	int		GetIoThreadCount()	{ return mIoThreadCount; }

private:
	static unsigned int WINAPI	IoWorkerThread(LPVOID lpParam);
	static bool					ReceiveCompletion(const ClientSession* client, OverlappedIOContext* context, DWORD dwTransferred);
	static bool					SendCompletion(const ClientSession* client, OverlappedIOContext* context, DWORD dwTransferred);

private:
	HANDLE	mCompletionPort;
	int		mIoThreadCount;
	SOCKET	mListenSocket;
};

extern __declspec(thread) int LIoThreadID;
extern IOCP_Manager* GIOCP_Manager;
