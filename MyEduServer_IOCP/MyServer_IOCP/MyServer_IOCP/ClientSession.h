#pragma once
#define BUF_SIZE 4096

enum class EIOType : int 
{
	IO_NONE,
	IO_RECV,
	IO_RECV_ZERO,
	IO_SEND,
	IO_ACCEPT,
};

enum class EDisconnectReason : int
{
	DR_NONE,
	DR_ACTIVE,
	DR_RECV_ZERO,
	DR_CONNECT_ERROR,
	DR_COMPLETION_ERROR,
};

class ClientSession;
class FastSpinLock;

struct OverlappedIOContext
{
	OverlappedIOContext( ) = default;

	OverlappedIOContext(const ClientSession* InClientSession, EIOType InIoType ) : mSessionObject(InClientSession), IOType(InIoType )
	{
		memset( &mOverlapped , 0 , sizeof( OVERLAPPED ) );
		memset( &mWsaBuf , 0 , sizeof( WSABUF ) );
		memset( mBuf , 0 , sizeof( BUF_SIZE ) );
	}

	OVERLAPPED				mOverlapped;
	const ClientSession*	mSessionObject = nullptr;
	WSABUF					mWsaBuf;
	char					mBuf[ BUF_SIZE ];
	EIOType					IOType = EIOType::IO_NONE;
};

class ClientSession
{
public:
	ClientSession( );
	ClientSession( SOCKET InStreamSocket ) : mStreamSocket( InStreamSocket ) , mConnected( false ) 
	{
		memset( &mClientAddr , 0 , sizeof( SOCKADDR_IN ) );
	}

	~ClientSession( );

	//연결을 담당하는 method
	bool			OnConnect( SOCKADDR_IN* Addr );
	void			OnDisconnect( EDisconnectReason InReason );

	//연결이 되었는지 확인하는 method
	bool			IsConnected( ) const { return mConnected; }

	//데이터 통신
	bool			PostRecv( ) const;
	bool			PostSend( const char*buff , int len ) const;
	
	//Getter
	SOCKET			GetStreamSocket( ) { return mStreamSocket; }
private:
	bool			mConnected = false;
	SOCKET			mStreamSocket;

	SOCKADDR_IN		mClientAddr;

	//todo : lock.
	FastSpinLock	mLock;
};

