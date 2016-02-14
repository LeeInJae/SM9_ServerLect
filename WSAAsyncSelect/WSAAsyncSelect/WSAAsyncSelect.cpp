#pragma warning(disable: 4996)
#include <iostream>
#include <WinSock2.h>
#include "WSAAsyncSelect.h"

using namespace std;
#define BUFF_SIZE	4096
#define Listen_Size 5
#define WM_SOCKET (WM_USER +1)


int main( void )
{
	//Create Window Handle
	HWND Handle_Window = GetWindowHandle( );
	if(Handle_Window == nullptr)
	{
		cout << "Window Handle Error" << endl;
	}

	//Winsock Init
	WSAData wsaData;
	if(WSAStartup( MAKEWORD( 2 , 2 ) , &wsaData ) != 0)
	{
		cout << "Winsock error" << endl;
	}

	//ServerSocket(Listen Socket Setting)
	SOCKET ServerSocket = socket( AF_INET , SOCK_STREAM , IPPROTO_TCP );
	if(ServerSocket == INVALID_SOCKET)
	{
		cout << "Server Socket Create error" << endl;
	}

	if(WSAAsyncSelect( ServerSocket , Handle_Window , WM_SOCKET , FD_ACCEPT | FD_CLOSE ) == SOCKET_ERROR)
	{
		cout << "WSAAsyncSelect error" << endl;
	}
	SOCKADDR_IN ServerAddr;
	int SizeServerAddr = sizeof( ServerAddr );
	memset( &ServerAddr , 0 , SizeServerAddr );

	if(WSAStringToAddress( L"127.0.0.1:9001" , AF_INET , NULL , ( SOCKADDR* )&ServerAddr , &SizeServerAddr ) == SOCKET_ERROR)
	{
		cout << "Server Addr Error" << endl;
	}

	if(bind( ServerSocket , ( SOCKADDR* )&ServerAddr , SizeServerAddr ) == SOCKET_ERROR)
	{
		cout << "bind error" << endl;
	}

	if(listen( ServerSocket , Listen_Size ) == SOCKET_ERROR)
	{
		cout << "listen error" << endl;
	}

	//GetMessage from Window MessageQueue
	MSG Msg;
	while(GetMessage( &Msg , 0 , 0 , 0 ) > 0)
	{
		TranslateMessage( &Msg );
		DispatchMessage( &Msg );
	}

	closesocket( ServerSocket );
	WSACleanup( );

	return 0;
}

LRESULT CALLBACK WndProc( HWND hWnd , UINT uMsg , WPARAM wParam , LPARAM lParam )
{
	switch(uMsg)
	{
	case WM_SOCKET:
		ProcessSocketIO( hWnd , wParam , lParam );
		break;
	case WM_DESTROY:
		PostQuitMessage( 0 );
		break;
	default:
		break;
	}
	return DefWindowProc( hWnd , uMsg , wParam , lParam );
}

void ProcessSocketIO( HWND hWnd , WPARAM wParam , LPARAM lParam )
{
	SOCKET TargetSocket = static_cast< SOCKET >( wParam );

	switch(WSAGETSELECTEVENT( lParam ))
	{
	case FD_ACCEPT:
		ServerSocket_Accept( TargetSocket , hWnd );
		break;
	case FD_READ:
		StreamSocket_Read( TargetSocket );
		break;
	case FD_WRITE:
		StreamSocket_Write( TargetSocket );
		break;
	case FD_CLOSE:
		TargetSocket_Close( TargetSocket );
		break;
	default:
		break;
	}
}

void ServerSocket_Accept( SOCKET ServerSocket , HWND hWnd )
{
	SOCKADDR_IN ClientAddr;
	int SizeClientAddr = sizeof( ClientAddr );
	memset( &ClientAddr , 0 , SizeClientAddr );

	SOCKET StreamSocket = accept( ServerSocket , ( SOCKADDR* )&ClientAddr , &SizeClientAddr );
	WCHAR AddressString[ BUFF_SIZE + 1 ];
	DWORD SizeAdressString = BUFF_SIZE;

	if(WSAAddressToString( ( SOCKADDR* )&ClientAddr , SizeClientAddr , NULL , AddressString , &SizeAdressString ) == SOCKET_ERROR)
	{
		cout << "ClientAddr Error" << endl;
	}
	else
	{
		//wchar 출력(wide 형)
		wcout << "Connected   =  " << AddressString << endl;
	}

	if(StreamSocket == INVALID_SOCKET)
	{
		cout << "Stream Socket Error" << endl;
		return;
	}

	if(WSAAsyncSelect( StreamSocket , hWnd , WM_SOCKET , FD_READ | FD_WRITE | FD_CLOSE ) == SOCKET_ERROR)
	{
		cout << "Stream Socket WSAAsyncSelect Error" << endl;
		return;
	}
}

void StreamSocket_Read( SOCKET StreamSocket )
{
	int TotalSizeRecv = 0 , TotalSizeSend = 0;
	char Buff[ BUFF_SIZE + 1 ];

	//Question : tcp 통신이므로 while로 수신버퍼가 빌 때까지 받아야 하는 것 아닌가?
	int SizeRecv = recv( StreamSocket , Buff , BUFF_SIZE , NULL );
	
	if(SizeRecv == SOCKET_ERROR)
	{
		if(WSAGetLastError( ) != WSAEWOULDBLOCK)
		{
			TargetSocket_Close( StreamSocket );
			return;
		}
	}
	fd_set
	if(SizeRecv == 0)
	{
		//Gracefuly Close
		cout << "정상 종료요~~" << endl;
		TargetSocket_Close( StreamSocket );
		return;
	}

	TotalSizeRecv += SizeRecv;
	select
	ioctlsocket
	//Question: 만약에 send가 실패해서 Data를 보낼 수 없다면, 이 에코데이터는 클라이언트에서 못받을 수 있는것아닌가?
	//성공할때까지 보내야 하는 것아닌가?
	//그러면 계속 성공할때까지 기다린다면, 부하로 작동할 것이고 어떠한 이유로 영영 보내지 못하면 서버는 더이상 제할일 못하고 죽는 것 아닌가?
	int SizeSend = send( StreamSocket , Buff , TotalSizeRecv , NULL );
	if(SizeSend == SOCKET_ERROR)
	{
		if(WSAGetLastError( ) != WSAEWOULDBLOCK)
		{
			TargetSocket_Close( StreamSocket );
			return;
		}
	}
	
	SOCKET Socket	= socket( AF_INET , SOCK_STREAM , IPPROTO_TCP );
	int Optval		= true;
	setsockopt( Socket , IPPROTO_TCP , TCP_NODELAY , ( const char )&Optval , sizeof(SizeOptVal) );
}

void StreamSocket_Write( SOCKET StreamSocket )
{
	//Question : 정령 Write에서는 아무것도 해줄 필요가없는가?
}

void TargetSocket_Close( SOCKET TargetSocket )
{
	closesocket( TargetSocket );
}
HWND GetWindowHandle( )
{
	WNDCLASS wndclass;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hbrBackground = ( HBRUSH )GetStockObject( WHITE_BRUSH );
	wndclass.hCursor = LoadCursor( NULL , IDC_ARROW );
	wndclass.hIcon = LoadIcon( NULL , IDI_APPLICATION );
	wndclass.hInstance = NULL;
	wndclass.lpfnWndProc = ( WNDPROC )WndProc;
	wndclass.lpszClassName = L"MyWindowClass";
	wndclass.lpszMenuName = NULL;
	wndclass.style = CS_HREDRAW | CS_VREDRAW;

	if(RegisterClass( &wndclass ) == 0)
		return ( HWND )( nullptr );

	HWND hWnd = CreateWindow( L"MyWindowClass" , L"TCP 서버" , WS_OVERLAPPEDWINDOW , 0 , 0 , 600 , 300 , NULL , ( HMENU )NULL , NULL , NULL );
	if(hWnd != NULL)
	{
		ShowWindow( hWnd , SW_SHOWNORMAL );
		UpdateWindow( hWnd );

		return hWnd;
	}

	return ( HWND )( nullptr );
}