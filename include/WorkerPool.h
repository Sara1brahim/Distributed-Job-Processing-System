
#ifndef WORKER_POOL_H
#define WORKER_POOL_H

#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <chrono>
#include <memory>

#include "JobScheduler.h"
#include "Worker.h"
#include "Database.h"

struct DelayedJob
{
    std::shared_ptr<Job> job;
    std::chrono::steady_clock::time_point retryTime;
};

struct DelayedJobComparator
{
    bool operator()(const DelayedJob& first, const DelayedJob& second) const
    {
        return first.retryTime > second.retryTime;
    }
};

class WorkerPool
{
private:
    JobScheduler scheduler;
    Database& database;

    std::priority_queue<
            DelayedJob,
            std::vector<DelayedJob>,
            DelayedJobComparator
    > delayedRetries;

    std::vector<std::thread> workers;
    std::thread retryThread;

    std::mutex queueMutex;
    std::condition_variable condition;
    std::condition_variable retryCondition;

    bool stopping;
    int outstandingJobs;

    void workerLoop(int workerId);
    void retryLoop();
    void finishJob();

public:
    WorkerPool(int numberOfWorkers, Database& database);
    ~WorkerPool();

    WorkerPool(const WorkerPool&) = delete;
    WorkerPool& operator=(const WorkerPool&) = delete;

    void submitJob(const std::shared_ptr<Job>& job);
    void shutdown();
};

#endif
