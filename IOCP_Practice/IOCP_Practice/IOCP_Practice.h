#pragma once

enum class THREAD_TYPE : int
{
	THREAD_NONE,
	THREAD_MAIN_ACCEPT,
	THREAD_IO_WORKER
};

extern __declspec(thread) THREAD_TYPE LThreadType;