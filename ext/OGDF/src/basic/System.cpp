/*
 * $Revision: 2633 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-18 09:09:28 +0200 (Mi, 18. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of System class.
 *
 * \author Carsten Gutwenger
 *
 * \par License:
 * This file is part of the Open Graph Drawing Framework (OGDF).
 *
 * \par
 * Copyright (C)<br>
 * See README.txt in the root directory of the OGDF installation for details.
 *
 * \par
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * Version 2 or 3 as published by the Free Software Foundation;
 * see the file LICENSE.txt included in the packaging of this file
 * for details.
 *
 * \par
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * \par
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * \see  http://www.gnu.org/copyleft/gpl.html
 ***************************************************************/


#include <ogdf/basic/basic.h>

#ifdef __APPLE__
#include <stdlib.h>
#include <malloc/malloc.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/utsname.h>
#include <mach/vm_statistics.h>
#include <mach/mach.h>
#include <mach/machine.h>
#elif defined(OGDF_SYSTEM_UNIX)
#include <malloc.h>
#endif

#if defined(OGDF_SYSTEM_WINDOWS) || defined(__CYGWIN__)
#include <Psapi.h>
#endif

#ifdef _MSC_VER
#include <intrin.h>

#elif defined(OGDF_SYSTEM_UNIX) || (defined(__MINGW32__) && !defined(__MINGW64__))
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>

static void __cpuid(int CPUInfo[4], int infoType)
{
	uint32_t a = CPUInfo[0];
	uint32_t b = CPUInfo[1];
	uint32_t c = CPUInfo[2];
	uint32_t d = CPUInfo[3];

#if defined(__i386__) || defined(__x86_64__) && !defined(__APPLE__)
	__asm__ __volatile__ ("xchgl	%%ebx,%0\n\t"
						"cpuid	\n\t"
						"xchgl	%%ebx,%0\n\t"
						: "+r" (b), "=a" (a), "=c" (c), "=d" (d)
						: "1" (infoType), "2" (c));
#else
	// not supported on other systems!
	a = b = c = d = 0;
#endif

	CPUInfo[0] = a;
	CPUInfo[1] = b;
	CPUInfo[2] = c;
	CPUInfo[3] = d;
}
#endif


namespace ogdf {

unsigned int System::s_cpuFeatures;
int          System::s_cacheSize;
int          System::s_cacheLine;
int          System::s_pageSize;
int          System::s_numberOfProcessors;


#if defined(OGDF_SYSTEM_WINDOWS) || defined(__CYGWIN__)
LARGE_INTEGER System::s_HPCounterFrequency;
#endif


void System::init()
{
	s_cpuFeatures = 0;
	s_cacheSize   = 0;
	s_cacheLine   = 0;

	// currently not working for shared libs on Linux
#if !defined(OGDF_DLL) || !defined(OGDF_SYSTEM_UNIX)

	int CPUInfo[4] = {-1};
	__cpuid(CPUInfo, 0);

	unsigned int nIds = CPUInfo[0];
	if(nIds >= 1)
	{
		__cpuid(CPUInfo, 1);

		int featureInfoECX = CPUInfo[2];
		int featureInfoEDX = CPUInfo[3];

		if(featureInfoEDX & (1 << 23)) s_cpuFeatures |= cpufmMMX;
		if(featureInfoEDX & (1 << 25)) s_cpuFeatures |= cpufmSSE;
		if(featureInfoEDX & (1 << 26)) s_cpuFeatures |= cpufmSSE2;
		if(featureInfoECX & (1 <<  0)) s_cpuFeatures |= cpufmSSE3;
		if(featureInfoECX & (1 <<  9)) s_cpuFeatures |= cpufmSSSE3;
		if(featureInfoECX & (1 << 19)) s_cpuFeatures |= cpufmSSE4_1;
		if(featureInfoECX & (1 << 20)) s_cpuFeatures |= cpufmSSE4_2;
		if(featureInfoECX & (1 <<  5)) s_cpuFeatures |= cpufmVMX;
		if(featureInfoECX & (1 <<  6)) s_cpuFeatures |= cpufmSMX;
		if(featureInfoECX & (1 <<  7)) s_cpuFeatures |= cpufmEST;
		if(featureInfoECX & (1 <<  3)) s_cpuFeatures |= cpufmMONITOR;
	}

	__cpuid(CPUInfo, 0x80000000);
	unsigned int nExIds = CPUInfo[0];

	if(nExIds >= 0x80000006) {
		__cpuid(CPUInfo, 0x80000006);
		s_cacheLine = CPUInfo[2] & 0xff;
		s_cacheSize = (CPUInfo[2] >> 16) & 0xffff;
	}

#if defined(OGDF_SYSTEM_WINDOWS) || defined(__CYGWIN__)
	QueryPerformanceFrequency(&s_HPCounterFrequency);

	SYSTEM_INFO siSysInfo;
	GetSystemInfo(&siSysInfo);
	s_pageSize = siSysInfo.dwPageSize;
	s_numberOfProcessors = siSysInfo.dwNumberOfProcessors;

#elif defined(OGDF_SYSTEM_UNIX) && defined(__APPLE__)
	unsigned long long value;
	size_t  size = sizeof( value );
		if (sysctlbyname("hw.pagesize", &value, &size, NULL, 0) !=-1)
		s_pageSize = (int)value;
	else
		s_pageSize = 0;

	if (sysctlbyname("hw.ncpu", &value, &size, NULL, 0) !=-1)
		s_numberOfProcessors = (int)value;
	else
		s_numberOfProcessors = 1;

#elif defined(OGDF_SYSTEM_UNIX)
	s_pageSize = sysconf(_SC_PAGESIZE);
	s_numberOfProcessors = (int)sysconf(_SC_NPROCESSORS_CONF);

#else
	s_pageSize = 0; // just a placeholder!!!
	s_numberOfProcessors = 1; // just a placeholder!!!
#endif

#endif
}


#if defined(OGDF_SYSTEM_WINDOWS) || defined(__CYGWIN__)
void System::getHPCounter(LARGE_INTEGER &counter)
{
	QueryPerformanceCounter(&counter);
}


double System::elapsedSeconds(
	const LARGE_INTEGER &startCounter,
	const LARGE_INTEGER &endCounter)
{
	return double(endCounter.QuadPart - startCounter.QuadPart)
					/ s_HPCounterFrequency.QuadPart;
}


__int64 System::usedRealTime(__int64 &t)
{
	__int64 tStart = t;
	t = GetTickCount();
	return t - tStart;
}


long long System::physicalMemory()
{
#if !defined(__CYGWIN__) || (_WIN32_WINNT >= 0x0500)
	MEMORYSTATUSEX statex;
	statex.dwLength = sizeof (statex);

	GlobalMemoryStatusEx (&statex);
	return statex.ullTotalPhys;
#else
	MEMORYSTATUS stat;
	stat.dwLength = sizeof (stat);

	GlobalMemoryStatus (&stat);
	return stat.dwTotalPhys;
#endif
}

long long System::availablePhysicalMemory()
{
#if !defined(__CYGWIN__) || (_WIN32_WINNT >= 0x0500)
	MEMORYSTATUSEX statex;
	statex.dwLength = sizeof (statex);

	GlobalMemoryStatusEx (&statex);
	return statex.ullAvailPhys;
#else
	MEMORYSTATUS stat;
	stat.dwLength = sizeof (stat);

	GlobalMemoryStatus (&stat);
	return stat.dwAvailPhys;
#endif
}

size_t System::memoryUsedByProcess()
{
	PROCESS_MEMORY_COUNTERS pmc;
	GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));

	return pmc.WorkingSetSize;
}

size_t System::peakMemoryUsedByProcess()
{
	PROCESS_MEMORY_COUNTERS pmc;
	GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));

	return pmc.PeakWorkingSetSize;
}

#elif __APPLE__

long long System::physicalMemory()
{
	unsigned long long value;
	size_t  size = sizeof( value );
	if (sysctlbyname("hw.memsize", &value, &size, NULL, 0) !=-1)
		return value;
	else
		return 0;
}

long long System::availablePhysicalMemory()
{
	unsigned long long pageSize;
	long long result;
	size_t  size = sizeof( pageSize );
	sysctlbyname("hw.pagesize", &pageSize, &size, NULL, 0);

	vm_statistics_data_t vm_stat;
	int count = ((mach_msg_type_number_t) (sizeof(vm_statistics_data_t)/sizeof(integer_t)));
	host_statistics(mach_host_self(), HOST_VM_INFO, (integer_t*)&vm_stat, (mach_msg_type_number_t*)&count);
	result = (unsigned long long)(vm_stat.free_count + vm_stat.inactive_count) * pageSize;
	return result;
}


size_t System::memoryUsedByProcess()
{
	/*int pid = getpid();
	static char filename[32];
	sprintf(filename, 32, "/proc/%d/statm", pid);

	int fd = open(filename, O_RDONLY, 0);
	if(fd==-1) OGDF_THROW(Exception);

	static char sbuf[256];
	sbuf[read(fd, sbuf, sizeof(sbuf) - 1)] = 0;
	close(fd);

	long size, resident, share, trs, lrs, drs, dt;
	sscanf(sbuf, "%ld %ld %ld %ld %ld %ld %ld",
		&size,      // total program size (in pages)
		&resident,  // number of resident set (non-swapped) pages (4k)
		&share,     // number of pages of shared (mmap'd) memory
		&trs,       // text resident set size
		&lrs,       // shared-lib resident set size
		&drs,       // data resident set size
		&dt);       // dirty pages

	return resident*4*1024;*/
	return 0;
}

#else
// LINUX, NOT MAC OS
long long System::physicalMemory()
{
	return (long long)(sysconf(_SC_PHYS_PAGES)) * sysconf(_SC_PAGESIZE);
}

long long System::availablePhysicalMemory()
{
	return (long long)(sysconf(_SC_AVPHYS_PAGES)) * sysconf(_SC_PAGESIZE);
}

size_t System::memoryUsedByProcess()
{
	int pid = getpid();
	static char filename[32];
	sprintf(filename, 32, "/proc/%d/statm", pid);

	int fd = open(filename, O_RDONLY, 0);
	if(fd==-1) OGDF_THROW(Exception);

	static char sbuf[256];
	sbuf[read(fd, sbuf, sizeof(sbuf) - 1)] = 0;
	close(fd);

	long size, resident, share, trs, lrs, drs, dt;
	sscanf(sbuf, "%ld %ld %ld %ld %ld %ld %ld",
		&size,      // total program size (in pages)
		&resident,  // number of resident set (non-swapped) pages (4k)
		&share,     // number of pages of shared (mmap'd) memory
		&trs,       // text resident set size
		&lrs,       // shared-lib resident set size
		&drs,       // data resident set size
		&dt);       // dirty pages

	return resident*4*1024;
}

#endif


#ifdef OGDF_SYSTEM_WINDOWS

size_t System::memoryAllocatedByMalloc()
{
	_HEAPINFO hinfo;
	int heapstatus;
	hinfo._pentry = NULL;

	size_t allocMem = 0;
	while((heapstatus = _heapwalk(&hinfo)) == _HEAPOK)
	{
		if(hinfo._useflag == _USEDENTRY)
			allocMem += hinfo._size;
	}

	return allocMem;
}

size_t System::memoryInFreelistOfMalloc()
{
	_HEAPINFO hinfo;
	int heapstatus;
	hinfo._pentry = NULL;

	size_t allocMem = 0;
	while((heapstatus = _heapwalk(&hinfo)) == _HEAPOK)
	{
		if(hinfo._useflag == _FREEENTRY)
			allocMem += hinfo._size;
	}

	return allocMem;
}

#elif __APPLE__

size_t System::memoryAllocatedByMalloc()
{
	return mstats().chunks_used;
}

size_t System::memoryInFreelistOfMalloc()
{
	return mstats().chunks_free;
}
#else

size_t System::memoryAllocatedByMalloc()
{
	return mallinfo().uordblks;
}

size_t System::memoryInFreelistOfMalloc()
{
	return mallinfo().fordblks;
}

#endif

#if !defined(OGDF_SYSTEM_WINDOWS) && !defined(__CYGWIN__)
__int64 System::usedRealTime(__int64 &t)
{
	__int64 tStart = t;
	timeval tv;
	gettimeofday(&tv, 0);
	t = __int64(tv.tv_sec) * 1000 + tv.tv_usec/1000;
	return t - tStart;
}
#endif


size_t System::memoryAllocatedByMemoryManager()
{
	return PoolMemoryAllocator::memoryAllocatedInBlocks();
}

size_t System::memoryInGlobalFreeListOfMemoryManager()
{
	return PoolMemoryAllocator::memoryInGlobalFreeList();
}

size_t System::memoryInThreadFreeListOfMemoryManager()
{
	return PoolMemoryAllocator::memoryInThreadFreeList();
}


} // namespace ogdf
