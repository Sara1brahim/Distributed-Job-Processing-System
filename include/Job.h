#ifndef JOB_H
#define JOB_H

#include <string>
#include <chrono>

enum class JobStatus
{
    Pending,
    Running,
    Completed,
    Failed
};

std::string jobStatusToString(JobStatus status);

class Job
{
private:
    int id;
    std::string type;
    int priority;
    JobStatus status;

    int retryCount;
    int maxRetries;

    int attemptCount;
    int failuresBeforeSuccess;

    using Clock = std::chrono::steady_clock;

    Clock::time_point createdAt;
    Clock::time_point startedAt;
    Clock::time_point attemptStartedAt;
    Clock::time_point attemptEndedAt;
    Clock::time_point retryScheduledAt;
    Clock::time_point finishedAt;

    std::chrono::milliseconds executionDuration;
    std::chrono::milliseconds retryWaitingTime;

    bool hasStarted;
    bool hasFinished;
    bool waitingForRetry;

public:
    Job(
            int id,
            std::string type,
            int priority,
            int maxRetries = 2,
            int failuresBeforeSuccess = 0
    );

    int getId() const;
    std::string getType() const;
    int getPriority() const;
    JobStatus getStatus() const;

    int getRetryCount() const;
    int getMaxRetries() const;
    int getAttemptCount() const;
    int getFailuresBeforeSuccess() const;

    bool canRetry() const;
    bool shouldFail();

    bool start();
    bool complete();
    bool fail();
    bool failUnexpected();
    bool scheduleRetry();

    double getExecutionDuration() const;
    double getRetryWaitingTime() const;
    double getTotalProcessingTime() const;
};

#endif