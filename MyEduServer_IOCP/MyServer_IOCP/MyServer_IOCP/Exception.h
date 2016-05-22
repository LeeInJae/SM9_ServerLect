#pragma once
class Exception
{
public:
	Exception( );
	~Exception( );
};

inline void CRASH_ASSERT( bool isOk )
{
	if(isOk)
		return;

	int* crashVal = 0;
	*crashVal = 0xDEADBEEF;
}
