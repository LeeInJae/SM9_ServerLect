#pragma once
#define SERVER_ADDRESS L"127.0.0.1:9001"

enum class EThreadType : int
{
	THREAD_NONE,
	THREAD_MAIN_ACCEPT,
	THREAD_IO_WORKER,
};

extern __declspec( thread ) EThreadType LThreadType;