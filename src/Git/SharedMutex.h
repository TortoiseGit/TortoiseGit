/**
* SharedMutex.h
* @Author   Tu Yongce <yongce (at) 126 (dot) com>
* @Created  2008-11-17
* @Modified 2008-11-17
* @Version  0.1
*/

#ifndef SHARED_MUTEX_H_INCLUDED
#define SHARED_MUTEX_H_INCLUDED

#ifndef _WIN32
#error "works only on Windows"
#endif

#include <windows.h>

// 可共享的互斥量
class SharedMutex
{
private:
	HANDLE m_mutex;
	HANDLE m_sharedEvent;
	HANDLE m_exclusiveEvent;

	volatile int m_sharedNum;
	volatile int m_exclusiveNum;
	volatile int m_lockType;

	static const int LOCK_NONE = 0;
	static const int LOCK_SHARED = 1;
	static const int LOCK_EXCLUSIVE = 2;

public:
	SharedMutex();
	void Init();
	void Release();
//	SharedMutex(SharedMutex &mutex);
//	SharedMutex & operator = (SharedMutex &mutex);

	~SharedMutex();

	// 获取共享访问权
#ifdef DEBUG
	bool AcquireShared(DWORD waitTime = INFINITE);
#else
	bool AcquireShared(DWORD waitTime = 500);
#endif
	// 释放共享访问权
	void ReleaseShared();

	// 获取独占访问权
#ifdef DEBUG
	bool AcquireExclusive(DWORD waitTime = INFINITE);
#else
	bool AcquireExclusive(DWORD waitTime = 500);
#endif
	// 释放独占访问权
	void ReleaseExclusive();
};

#endif // SHARED_MUTEX_H_INCLUDED
