
#ifndef JOB_SCHEDULER_H
#define JOB_SCHEDULER_H

#include <queue>
#include <vector>
#include <memory>
#include "Job.h"

struct JobComparator
{
    bool operator()(
            const std::shared_ptr<Job>& first,
            const std::shared_ptr<Job>& second
    ) const;
};

class JobScheduler
{
private:
    std::priority_queue<
            std::shared_ptr<Job>,
            std::vector<std::shared_ptr<Job>>,
            JobComparator
    > jobs;

public:
    void addJob(const std::shared_ptr<Job>& job);
    bool tryAddJob(const std::shared_ptr<Job>& job);

    std::shared_ptr<Job> getNextJob();

    bool isEmpty() const;
    int getJobCount() const;
};

#endif
