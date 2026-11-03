
#include "WorkerPool.h"

#include <iostream>
#include <exception>
#include <stdexcept>

WorkerPool::WorkerPool(int numberOfWorkers, Database& database)
        : database(database), stopping(false), outstandingJobs(0)
{
    if (numberOfWorkers <= 0)
    {
        throw std::invalid_argument(
                "WorkerPool must have at least one worker."
        );
    }

    try
    {
        for (int i = 1; i <= numberOfWorkers; i++)
        {
            workers.emplace_back(
                    &WorkerPool::workerLoop,
                    this,
                    i
            );
        }

        retryThread = std::thread(
                &WorkerPool::retryLoop,
                this
        );
    }
    catch (...)
    {
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            stopping = true;
        }

        condition.notify_all();
        retryCondition.notify_all();

        for (std::thread& worker : workers)
        {
            if (worker.joinable())
                worker.join();
        }

        if (retryThread.joinable())
            retryThread.join();

        throw;
    }
}

WorkerPool::~WorkerPool()
{
    shutdown();
}

void WorkerPool::submitJob(const std::shared_ptr<Job>& job)
{
    if (job == nullptr)
    {
        std::cerr << "Cannot submit a null Job.\n";
        return;
    }

    {
        std::lock_guard<std::mutex> lock(queueMutex);

        if (stopping)
        {
            std::cerr << "Cannot submit Job: WorkerPool is stopping.\n";
            return;
        }

        if (!scheduler.tryAddJob(job))
        {
            std::cerr << "Failed to add Job "
                      << job->getId()
                      << " to Scheduler.\n";
            return;
        }

        outstandingJobs++;
    }

    condition.notify_one();
}

void WorkerPool::finishJob()
{
    {
        std::lock_guard<std::mutex> lock(queueMutex);

        if (outstandingJobs > 0)
            outstandingJobs--;
    }

    condition.notify_all();
    retryCondition.notify_all();
}

void WorkerPool::workerLoop(int workerId)
{
    Worker worker(workerId, database);

    while (true)
    {
        std::shared_ptr<Job> currentJob;

        {
            std::unique_lock<std::mutex> lock(queueMutex);

            condition.wait(lock, [this]()
            {
                return !scheduler.isEmpty() ||
                       (stopping && outstandingJobs == 0);
            });

            if (stopping && outstandingJobs == 0)
                return;

            currentJob = scheduler.getNextJob();
        }

        if (currentJob == nullptr)
            continue;

        ExecutionResult result = ExecutionResult::InvalidState;
        bool unexpectedException = false;

        try
        {
            result = worker.execute(currentJob);
        }
        catch (const std::exception& error)
        {
            std::cerr << "Unexpected exception in Worker "
                      << workerId
                      << " while executing Job "
                      << currentJob->getId()
                      << ": " << error.what()
                      << '\n';

            unexpectedException = true;
        }
        catch (...)
        {
            std::cerr << "Unknown exception in Worker "
                      << workerId
                      << " while executing Job "
                      << currentJob->getId()
                      << '\n';

            unexpectedException = true;
        }

        if (unexpectedException)
        {
            if (!currentJob->failUnexpected())
            {
                std::cerr << "Could not mark Job "
                          << currentJob->getId()
                          << " as failed.\n";

                finishJob();
                continue;
            }

            try
            {
                if (!database.updateJob(*currentJob))
                {
                    std::cerr << "Could not save unexpected failure for Job "
                              << currentJob->getId()
                              << ".\n";

                    finishJob();
                    continue;
                }
            }
            catch (...)
            {
                std::cerr << "Exception while saving unexpected failure for Job "
                          << currentJob->getId()
                          << ".\n";

                finishJob();
                continue;
            }

            result = ExecutionResult::Failed;
        }

        if (result == ExecutionResult::Failed)
        {
            if (!currentJob->scheduleRetry())
            {
                std::cout << "Job " << currentJob->getId()
                          << " exhausted all retries.\n";

                finishJob();
                continue;
            }

            bool saved = false;

            try
            {
                saved = database.updateJob(*currentJob);
            }
            catch (...)
            {
                saved = false;
            }

            if (!saved)
            {
                std::cerr << "Could not save retry state for Job "
                          << currentJob->getId()
                          << ". Job will not be retried.\n";

                finishJob();
                continue;
            }

            auto retryTime =
                    std::chrono::steady_clock::now() +
                    std::chrono::seconds(5);

            bool retryQueued = false;

            try
            {
                std::lock_guard<std::mutex> lock(queueMutex);

                delayedRetries.push({currentJob, retryTime});
                retryQueued = true;
            }
            catch (...)
            {
                std::cerr << "Could not add Job "
                          << currentJob->getId()
                          << " to the retry queue.\n";
            }

            if (!retryQueued)
            {
                currentJob->failUnexpected();

                try
                {
                    database.updateJob(*currentJob);
                }
                catch (...)
                {
                    std::cerr << "Could not save final failure state for Job "
                              << currentJob->getId()
                              << ".\n";
                }

                finishJob();
                continue;
            }

            std::cout << "Job " << currentJob->getId()
                      << " will be retried after 5 seconds."
                      << " Retry count: "
                      << currentJob->getRetryCount()
                      << '\n';

            retryCondition.notify_one();
            continue;
        }

        if (result == ExecutionResult::DatabaseError)
        {
            std::cerr << "Job " << currentJob->getId()
                      << " stopped because of a database error.\n";
        }
        else if (result == ExecutionResult::InvalidState)
        {
            std::cout << "Job " << currentJob->getId()
                      << " was rejected because of an invalid state.\n";
        }

        finishJob();
    }
}

void WorkerPool::retryLoop()
{
    while (true)
    {
        std::unique_lock<std::mutex> lock(queueMutex);

        retryCondition.wait(lock, [this]()
        {
            return !delayedRetries.empty() ||
                   (stopping && outstandingJobs == 0);
        });

        if (stopping && outstandingJobs == 0)
            return;

        if (delayedRetries.empty())
            continue;

        auto retryTime = delayedRetries.top().retryTime;

        if (std::chrono::steady_clock::now() < retryTime)
        {
            retryCondition.wait_until(lock, retryTime);
            continue;
        }

        DelayedJob delayedJob = delayedRetries.top();

        if (!scheduler.tryAddJob(delayedJob.job))
        {
            std::cerr << "Failed to move Job "
                      << delayedJob.job->getId()
                      << " from retry queue to Scheduler.\n";

            retryCondition.wait_for(
                    lock,
                    std::chrono::seconds(1)
            );

            continue;
        }

        delayedRetries.pop();

        std::cout << "Job " << delayedJob.job->getId()
                  << " is ready for retry!\n";

        lock.unlock();

        condition.notify_one();
    }
}

void WorkerPool::shutdown()
{
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        stopping = true;
    }

    condition.notify_all();
    retryCondition.notify_all();

    for (std::thread& worker : workers)
    {
        if (worker.joinable())
            worker.join();
    }

    if (retryThread.joinable())
        retryThread.join();
}
