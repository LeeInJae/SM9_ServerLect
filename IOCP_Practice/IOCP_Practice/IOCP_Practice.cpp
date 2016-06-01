// IOCP_Practice.cpp : 콘솔 응용 프로그램에 대한 진입점을 정의합니다.
//

#include "stdafx.h"
#include "IOCP_Manager.h"
#include "SessionManager.h"
#include "IOCP_Practice.h"

__declspec(thread) THREAD_TYPE LThreadType = THREAD_TYPE::THREAD_NONE;

int main()
{
	LThreadType = THREAD_TYPE::THREAD_MAIN_ACCEPT;
	GSessionManager = new SessionManager;
	GIOCP_Manager	= new IOCP_Manager;

	if (GIOCP_Manager->Initialize() == false)
		return -1;

	if (GIOCP_Manager->StartIoThreads() == false)
		return -1;

	printf_s("Start Server======================================\n");
	if (GIOCP_Manager->StartAcceptLoop() == false)
		return -1;

	GIOCP_Manager->Finalize();

	printf_s("End Server======================================\n");

	delete GIOCP_Manager;
	delete GSessionManager;
    return 0;
}