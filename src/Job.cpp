#include "Job.h"

Job::Job(
    int id,
    std::string type,
    int priority,
    int maxRetries,
    int failuresBeforeSuccess
)
{
    this->id = id;
    this->type = type;
    this->priority = priority;
    this->status = JobStatus::Pending;

    this->retryCount = 0;
    this->maxRetries = maxRetries;

    this->attemptCount = 0;
    this->failuresBeforeSuccess = failuresBeforeSuccess;

    createdAt = Clock::now();

    executionDuration = std::chrono::milliseconds(0);
    retryWaitingTime = std::chrono::milliseconds(0);

    hasStarted = false;
    hasFinished = false;
    waitingForRetry = false;
}

int Job::getId() const { return id; }
std::string Job::getType() const { return type; }
int Job::getPriority() const { return priority; }
JobStatus Job::getStatus() const { return status; }

int Job::getRetryCount() const { return retryCount; }
int Job::getMaxRetries() const { return maxRetries; }
int Job::getAttemptCount() const { return attemptCount; }
int Job::getFailuresBeforeSuccess() const { return failuresBeforeSuccess; }

bool Job::canRetry() const
{
    return retryCount < maxRetries;
}

bool Job::shouldFail()
{
    return attemptCount <= failuresBeforeSuccess;
}

bool Job::start()
{
    if (status != JobStatus::Pending)
        return false;

    auto now = Clock::now();

    if (!hasStarted)
    {
        startedAt = now;
        hasStarted = true;
    }

    if (waitingForRetry)
    {
        retryWaitingTime +=
            std::chrono::duration_cast<std::chrono::milliseconds>(
                now - retryScheduledAt
            );

        waitingForRetry = false;
    }

    attemptCount++;
    attemptStartedAt = now;

    status = JobStatus::Running;

    return true;
}

bool Job::complete()
{
    if (status != JobStatus::Running)
        return false;

    auto now = Clock::now();

    executionDuration +=
        std::chrono::duration_cast<std::chrono::milliseconds>(
            now - attemptStartedAt
        );

    attemptEndedAt = now;
    finishedAt = now;

    hasFinished = true;
    status = JobStatus::Completed;

    return true;
}

bool Job::fail()
{
    if (status != JobStatus::Running)
        return false;

    auto now = Clock::now();

    executionDuration +=
        std::chrono::duration_cast<std::chrono::milliseconds>(
            now - attemptStartedAt
        );

    attemptEndedAt = now;
    finishedAt = now;

    hasFinished = true;
    status = JobStatus::Failed;

    return true;
}

bool Job::failUnexpected()
{
    if (status != JobStatus::Pending &&
        status != JobStatus::Running)
    {
        return false;
    }

    auto now = Clock::now();

    if (status == JobStatus::Running)
    {
        executionDuration +=
                std::chrono::duration_cast<std::chrono::milliseconds>(
                        now - attemptStartedAt
                );

        attemptEndedAt = now;
    }

    finishedAt = now;
    hasFinished = true;
    status = JobStatus::Failed;

    return true;
}

bool Job::scheduleRetry()
{
    if (status != JobStatus::Failed || !canRetry())
        return false;

    retryCount++;

    retryScheduledAt = Clock::now();
    waitingForRetry = true;

    hasFinished = false;
    status = JobStatus::Pending;

    return true;
}

double Job::getExecutionDuration() const
{
    return executionDuration.count() / 1000.0;
}

double Job::getRetryWaitingTime() const
{
    return retryWaitingTime.count() / 1000.0;
}

double Job::getTotalProcessingTime() const
{
    if (!hasStarted)
        return 0.0;

    Clock::time_point endTime = hasFinished
        ? finishedAt
        : Clock::now();

    auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            endTime - startedAt
        );

    return duration.count() / 1000.0;
}

std::string jobStatusToString(JobStatus status)
{
    switch (status)
    {
        case JobStatus::Pending: return "Pending";
        case JobStatus::Running: return "Running";
        case JobStatus::Completed: return "Completed";
        case JobStatus::Failed: return "Failed";
    }

    return "Unknown";
}