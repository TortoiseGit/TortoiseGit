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

// �ɹ���Ļ�����
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

	// ��ȡ�������Ȩ
#ifdef DEBUG
	bool AcquireShared(DWORD waitTime = INFINITE);
#else
	bool AcquireShared(DWORD waitTime = 500);
#endif
	// �ͷŹ������Ȩ
	void ReleaseShared();

	// ��ȡ��ռ����Ȩ
#ifdef DEBUG
	bool AcquireExclusive(DWORD waitTime = INFINITE);
#else
	bool AcquireExclusive(DWORD waitTime = 500);
#endif
	// �ͷŶ�ռ����Ȩ
	void ReleaseExclusive();
};

#endif // SHARED_MUTEX_H_INCLUDED
