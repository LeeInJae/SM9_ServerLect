#pragma once

class ClientSession;
struct OverlappedIOContext;

class IOCPManager
{
public:
	IOCPManager( ) : mCompletionPort( NULL ) , mIOThreadCount( 0 ) , mListenSocket( NULL )
	{
		//
	}
	~IOCPManager( );

	bool Initialize( );
	void Finalize( );

	bool StartIoThreads( );
	bool StartAcceptLoop( );

	HANDLE GetCompletionPort( ) { return mCompletionPort; }
	int GetIOThreadCount( ) { return mIOThreadCount; }
private:
	static unsigned int WINAPI IOWorkerThread( LPVOID lpParam );
	static bool ReceiveCompletion( const ClientSession* InClientSession , OverlappedIOContext* InOverlappedIOContext , DWORD dwTransferred );
	static bool SendCompletion( const ClientSession* InClientSession , OverlappedIOContext* InOverlappedIOContext , DWORD dwTransferred );
private:
	HANDLE	mCompletionPort;
	int		mIOThreadCount = 0;

	SOCKET	mListenSocket;
};
extern	__declspec( thread ) int LThreadID;
extern	IOCPManager* GIOCPManager;