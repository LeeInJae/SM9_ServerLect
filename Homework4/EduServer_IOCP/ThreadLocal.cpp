#include "stdafx.h"
#include "ThreadLocal.h"
#include "Timer.h"
#include "LockOrderChecker.h"

//Threlad Locatl Storage를 이용하여 Thread별로 관리할 데이터를 정리
__declspec(thread) int LThreadType = -1;
__declspec(thread) int LIoThreadId = -1;
__declspec(thread) Timer* LTimer = nullptr;
__declspec(thread) int64_t LTickCount = 0;
__declspec(thread) LockOrderChecker* LLockOrderChecker = nullptr;

