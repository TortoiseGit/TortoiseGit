
#include "stdafx.h"
#include "StatusCache.h"
#include "RootFolderCache.h"
#include "IntegritySession.h"
#include "IntegrityActions.h"
#include "ShellExt.h"

#undef min
#undef max

class SmallFixedInProcessCache : public ICache {
public:
	SmallFixedInProcessCache();

	virtual FileStatusFlags getStatus(std::wstring fileName);
	virtual bool isPathControlled(std::wstring path);
	virtual void refresh(std::wstring path);

	virtual IntegritySession& getIntegritySession() { return integritySession; };
	
private:
	RootFolderCache rootFolderCache;
	IntegritySession integritySession;

	struct CachedEntry {
		std::wstring path;
		FileStatusFlags fileStatus;
		std::chrono::time_point<std::chrono::system_clock> entryCreationTime;
		int age;

		CachedEntry() : age(0) {}
	};

	struct CachedResult {
		FileStatusFlags fileStatus;
		bool valid;

		CachedResult(FileStatusFlags fileStatus) : fileStatus(fileStatus), valid(true) {};
		CachedResult() : valid(false) {};
	};

	static const int cacheSize = 10;
	static const std::chrono::seconds entryExpiryTime;
	int lastAge;
	CachedEntry cachedStatus[cacheSize];
	std::mutex cacheLockObject;

	CachedResult findCachedStatus(const std::wstring& path);
	void addCachedStatus(const std::wstring& path, FileStatusFlags fileStatus);
};

SmallFixedInProcessCache* cacheInstance = NULL;
const std::chrono::seconds SmallFixedInProcessCache::entryExpiryTime = std::chrono::seconds(30);

SmallFixedInProcessCache::SmallFixedInProcessCache() : integritySession(), rootFolderCache(integritySession)
{
	rootFolderCache.forceRefresh();
	lastAge = 0;
}

SmallFixedInProcessCache::CachedResult SmallFixedInProcessCache::findCachedStatus(const std::wstring& path) {
	std::lock_guard<std::mutex> lock(cacheLockObject);

	for (CachedEntry& entry : cachedStatus) {
		if (entry.path == path && (entry.entryCreationTime - std::chrono::system_clock::now()) < entryExpiryTime) {
			entry.age = ++lastAge;
			entry.entryCreationTime = std::chrono::system_clock::now();
			return CachedResult(entry.fileStatus);
		}
	}
	return CachedResult();
}

void SmallFixedInProcessCache::addCachedStatus(const std::wstring& path, FileStatusFlags fileStatus) {
	std::lock_guard<std::mutex> lock(cacheLockObject);

	// find oldest
	int oldestEntryAge = std::numeric_limits<int>::max();
	int oldestEntryIndex = -1;
	for (int i = 0; i < cacheSize; i++) {
		if (cachedStatus[i].age < oldestEntryAge || 
			(cachedStatus[i].entryCreationTime - std::chrono::system_clock::now()) < entryExpiryTime) {
			oldestEntryIndex = i;
			oldestEntryAge = cachedStatus[i].age;
		}
	}

	cachedStatus[oldestEntryIndex].path = path;
	cachedStatus[oldestEntryIndex].fileStatus = fileStatus;
	cachedStatus[oldestEntryIndex].age = ++lastAge;
}

FileStatusFlags SmallFixedInProcessCache::getStatus(std::wstring fileName)
{
	rootFolderCache.refreshIfStale();

	CachedResult result = findCachedStatus(fileName);
	if (result.valid) {
		return result.fileStatus;
	}

	FileStatusFlags status = IntegrityActions::getFileStatus(integritySession, fileName);
	if (status != (FileStatusFlags)FileStatus::TimeoutError) {
		addCachedStatus(fileName, status);
	}
	return status;
}

bool SmallFixedInProcessCache::isPathControlled(std::wstring path)
{
	return rootFolderCache.isPathControlled(path);
}

void SmallFixedInProcessCache::refresh(std::wstring path) {
	rootFolderCache.forceRefresh();

	for (CachedEntry entry : cachedStatus) {
		entry.path = L"";
	}

	// TODO? flush wf cache for folders?
}

////////////////////////////////////////////////////////////////////////////////
// ICache

ICache& ICache::getInstance() {
	AutoLocker lock(g_csGlobalCOMGuard);
	if(cacheInstance == NULL) {
		cacheInstance = new SmallFixedInProcessCache();
	}

	return *cacheInstance;
}