// MyServer_IOCP.cpp : �ܼ� ���� ���α׷��� ���� �������� �����մϴ�.
//

#include "stdafx.h"
#include "MyServer_IOCP.h"
#include "SessionManager.h"
#include "IOCPManager.h"

//thread local storage
__declspec( thread )EThreadType LThreadType = EThreadType::THREAD_NONE;

int main()
{
	LThreadType = EThreadType::THREAD_MAIN_ACCEPT;

	//Global Manager
	GSessionManager		= new SessionManager;
	GIOCPManager		= new IOCPManager;


    return 0;
}

