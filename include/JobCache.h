#ifndef JOB_CACHE_H
#define JOB_CACHE_H

#include <string>
#include <list>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <cstddef>
#include <cstdint>

#include "Job.h"

struct JobSnapshot
{
    int id;
    std::string type;
    int priority;
    std::string status;

    int retryCount;
    int maxRetries;
    int attemptCount;

    double executionDuration;
    double retryWaitingTime;
    double totalProcessingTime;

    static JobSnapshot fromJob(const Job& job);
};

struct CacheStatistics
{
    std::uint64_t hits = 0;
    std::uint64_t misses = 0;

    std::size_t getSize() const
    {
        return hits + misses;
    }

    double getHitRate() const
    {
        std::uint64_t total = hits + misses;

        if (total == 0)
            return 0.0;

        return static_cast<double>(hits) / total * 100.0;
    }
};

class JobCache
{
private:
    using Clock = std::chrono::steady_clock;

    struct CacheEntry
    {
        JobSnapshot data;
        Clock::time_point expirationTime;
        std::list<int>::iterator position;
    };

    std::size_t capacity;
    std::chrono::milliseconds ttl;

    std::list<int> recentlyUsed;

    std::unordered_map<int, CacheEntry> cache;

    mutable std::mutex cacheMutex;

    std::uint64_t hits;
    std::uint64_t misses;

    void moveToFront(
            std::unordered_map<int, CacheEntry>::iterator entry
    );

    void removeLeastRecentlyUsed();

public:
    explicit JobCache(
            std::size_t capacity = 100,
            std::chrono::milliseconds ttl = std::chrono::seconds(30)
    );

    JobCache(const JobCache&) = delete;
    JobCache& operator=(const JobCache&) = delete;

    bool get(int jobId, JobSnapshot& result);

    void put(const JobSnapshot& job);

    void put(const Job& job);

    void invalidate(int jobId);

    void clear();

    CacheStatistics getStatistics() const;

    std::size_t getCacheSize() const;
};

#endif