#include <iostream>
#include <thread>
#include <chrono>
#include <cassert>

#include "../include/JobCache.h"

using namespace std;
using namespace std::chrono;

JobSnapshot createSnapshot(int id, const string& type)
{
    JobSnapshot job{};

    job.id = id;
    job.type = type;
    job.priority = 5;
    job.status = "Pending";
    job.retryCount = 0;
    job.maxRetries = 2;
    job.attemptCount = 0;
    job.executionDuration = 0;
    job.retryWaitingTime = 0;
    job.totalProcessingTime = 0;

    return job;
}

void testBasicPutAndGet()
{
    cout << "\n[Test 1] Basic Put and Get\n";

    JobCache cache(10, seconds(30));

    JobSnapshot original = createSnapshot(1, "ImageProcessing");

    cache.put(original);

    JobSnapshot result{};

    bool found = cache.get(1, result);

    assert(found);
    assert(result.id == 1);
    assert(result.type == "ImageProcessing");

    cout << "PASSED: Job was stored and retrieved.\n";
}

void testCacheMiss()
{
    cout << "\n[Test 2] Cache Miss\n";

    JobCache cache(10, seconds(30));

    JobSnapshot result{};

    bool found = cache.get(100, result);

    assert(!found);

    CacheStatistics statistics = cache.getStatistics();

    assert(statistics.misses == 1);
    assert(statistics.hits == 0);

    cout << "PASSED: Missing Job returned a Cache Miss.\n";
}

void testLRUEviction()
{
    cout << "\n[Test 3] LRU Eviction\n";

    JobCache cache(2, seconds(30));

    cache.put(createSnapshot(1, "Job1"));
    cache.put(createSnapshot(2, "Job2"));

    JobSnapshot result{};

    // Job 1 becomes the most recently used.
    assert(cache.get(1, result));

    // Capacity is full, so Job 2 should be removed.
    cache.put(createSnapshot(3, "Job3"));

    assert(cache.get(1, result));
    assert(!cache.get(2, result));
    assert(cache.get(3, result));

    assert(cache.getCacheSize() == 2);

    cout << "PASSED: Least recently used Job was evicted.\n";
}

void testTTLExpiration()
{
    cout << "\n[Test 4] TTL Expiration\n";

    JobCache cache(10, milliseconds(100));

    cache.put(createSnapshot(1, "TemporaryJob"));

    JobSnapshot result{};

    assert(cache.get(1, result));

    // Wait until the entry expires.
    this_thread::sleep_for(milliseconds(150));

    bool found = cache.get(1, result);

    assert(!found);
    assert(cache.getCacheSize() == 0);

    cout << "PASSED: Expired Job was removed.\n";
}

void testInvalidation()
{
    cout << "\n[Test 5] Cache Invalidation\n";

    JobCache cache(10, seconds(30));

    cache.put(createSnapshot(1, "OldJob"));

    JobSnapshot result{};

    assert(cache.get(1, result));

    cache.invalidate(1);

    assert(!cache.get(1, result));

    cout << "PASSED: Invalidated Job was no longer available.\n";
}

void testUpdatingExistingEntry()
{
    cout << "\n[Test 6] Updating Existing Entry\n";

    JobCache cache(10, seconds(30));

    cache.put(createSnapshot(1, "OldJob"));

    JobSnapshot updated = createSnapshot(1, "UpdatedJob");

    updated.status = "Completed";

    cache.put(updated);

    JobSnapshot result{};

    assert(cache.get(1, result));

    assert(result.type == "UpdatedJob");
    assert(result.status == "Completed");

    assert(cache.getCacheSize() == 1);

    cout << "PASSED: Existing entry was updated correctly.\n";
}

void testClear()
{
    cout << "\n[Test 7] Clear Cache\n";

    JobCache cache(10, seconds(30));

    cache.put(createSnapshot(1, "Job1"));
    cache.put(createSnapshot(2, "Job2"));

    cache.clear();

    assert(cache.getCacheSize() == 0);

    JobSnapshot result{};

    assert(!cache.get(1, result));
    assert(!cache.get(2, result));

    cout << "PASSED: All cache entries were removed.\n";
}

void testCacheStatistics()
{
    cout << "\n[Test 8] Cache Statistics\n";

    JobCache cache(10, seconds(30));

    cache.put(createSnapshot(1, "Job1"));

    JobSnapshot result{};

    assert(cache.get(1, result));
    assert(cache.get(1, result));

    assert(!cache.get(2, result));
    assert(!cache.get(3, result));

    CacheStatistics statistics = cache.getStatistics();

    assert(statistics.hits == 2);
    assert(statistics.misses == 2);

    assert(statistics.getHitRate() == 50.0);

    cout << "Hits: " << statistics.hits << '\n';
    cout << "Misses: " << statistics.misses << '\n';
    cout << "Hit Rate: " << statistics.getHitRate() << "%\n";

    cout << "PASSED: Cache statistics are correct.\n";
}

int main()
{
    cout << "====================================\n";
    cout << "       JOB CACHE TEST SUITE         \n";
    cout << "====================================\n";

    testBasicPutAndGet();
    testCacheMiss();
    testLRUEviction();
    testTTLExpiration();
    testInvalidation();
    testUpdatingExistingEntry();
    testClear();
    testCacheStatistics();

    cout << "\n====================================\n";
    cout << " ALL CACHE TESTS PASSED SUCCESSFULLY\n";
    cout << "====================================\n";

    return 0;
}