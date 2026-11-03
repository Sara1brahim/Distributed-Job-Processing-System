
#include "Worker.h"

#include <iostream>
#include <thread>
#include <chrono>

Worker::Worker(int workerId, Database& database)
        : workerId(workerId), database(database)
{
}

ExecutionResult Worker::execute(const std::shared_ptr<Job>& job)
{
    if (job == nullptr)
        return ExecutionResult::InvalidState;

    if (!job->start())
    {
        std::cout << "Worker " << workerId
                  << " rejected Job " << job->getId()
                  << " because it is not Pending."
                  << std::endl;

        return ExecutionResult::InvalidState;
    }

    if (!database.updateJob(*job))
    {
        std::cerr << "Failed to save Job "
                  << job->getId()
                  << " after starting."
                  << std::endl;

        job->failUnexpected();
        database.updateJob(*job);

        return ExecutionResult::DatabaseError;
    }

    std::cout << "Worker " << workerId
              << " started Job " << job->getId()
              << " | Attempt: " << job->getAttemptCount()
              << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(2));

    if (job->shouldFail())
    {
        if (!job->fail())
            return ExecutionResult::InvalidState;

        if (!database.updateJob(*job))
        {
            std::cerr << "Failed to save failure state for Job "
                      << job->getId() << std::endl;

            return ExecutionResult::DatabaseError;
        }

        std::cout << "Job " << job->getId()
                  << " failed!" << std::endl;

        return ExecutionResult::Failed;
    }

    if (!job->complete())
        return ExecutionResult::InvalidState;

    if (!database.updateJob(*job))
    {
        std::cerr << "Failed to save completion state for Job "
                  << job->getId() << std::endl;

        return ExecutionResult::DatabaseError;
    }

    std::cout << "Worker " << workerId
              << " completed Job " << job->getId()
              << std::endl;

    return ExecutionResult::Success;
}
