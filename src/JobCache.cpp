#include "JobCache.h"

JobSnapshot JobSnapshot::fromJob(const Job& job)
{
    return {
            job.getId(),
            job.getType(),
            job.getPriority(),
            jobStatusToString(job.getStatus()),
            job.getRetryCount(),
            job.getMaxRetries(),
            job.getAttemptCount(),
            job.getExecutionDuration(),
            job.getRetryWaitingTime(),
            job.getTotalProcessingTime()
    };
}

JobCache::JobCache(
        std::size_t capacity,
        std::chrono::milliseconds ttl
)
        : capacity(capacity),
          ttl(ttl),
          hits(0),
          misses(0)
{
}

void JobCache::moveToFront(
        std::unordered_map<int, CacheEntry>::iterator entry
)
{
    recentlyUsed.erase(entry->second.position);

    recentlyUsed.push_front(entry->first);

    entry->second.position = recentlyUsed.begin();
}

void JobCache::removeLeastRecentlyUsed()
{
    if (recentlyUsed.empty())
        return;

    int jobId = recentlyUsed.back();

    recentlyUsed.pop_back();

    cache.erase(jobId);
}

bool JobCache::get(int jobId, JobSnapshot& result)
{
    std::lock_guard<std::mutex> lock(cacheMutex);

    auto entry = cache.find(jobId);

    if (entry == cache.end())
    {
        misses++;
        return false;
    }

    if (Clock::now() >= entry->second.expirationTime)
    {
        recentlyUsed.erase(entry->second.position);
        cache.erase(entry);

        misses++;
        return false;
    }

    result = entry->second.data;

    moveToFront(entry);

    hits++;

    return true;
}

void JobCache::put(const JobSnapshot& job)
{
    std::lock_guard<std::mutex> lock(cacheMutex);

    if (capacity == 0 || ttl.count() <= 0)
        return;

    auto entry = cache.find(job.id);

    if (entry != cache.end())
    {
        entry->second.data = job;
        entry->second.expirationTime = Clock::now() + ttl;

        moveToFront(entry);

        return;
    }

    while (cache.size() >= capacity)
    {
        removeLeastRecentlyUsed();
    }

    recentlyUsed.push_front(job.id);

    CacheEntry newEntry;

    newEntry.data = job;
    newEntry.expirationTime = Clock::now() + ttl;
    newEntry.position = recentlyUsed.begin();

    cache.emplace(job.id, std::move(newEntry));
}

void JobCache::put(const Job& job)
{
    put(JobSnapshot::fromJob(job));
}

void JobCache::invalidate(int jobId)
{
    std::lock_guard<std::mutex> lock(cacheMutex);

    auto entry = cache.find(jobId);

    if (entry == cache.end())
        return;

    recentlyUsed.erase(entry->second.position);

    cache.erase(entry);
}

void JobCache::clear()
{
    std::lock_guard<std::mutex> lock(cacheMutex);

    cache.clear();
    recentlyUsed.clear();
}

CacheStatistics JobCache::getStatistics() const
{
    std::lock_guard<std::mutex> lock(cacheMutex);

    CacheStatistics statistics;

    statistics.hits = hits;
    statistics.misses = misses;

    return statistics;
}

std::size_t JobCache::getCacheSize() const
{
    std::lock_guard<std::mutex> lock(cacheMutex);

    return cache.size();
}