
#include "JobScheduler.h"

bool JobComparator::operator()(
        const std::shared_ptr<Job>& first,
        const std::shared_ptr<Job>& second
) const
{
    if (first->getPriority() != second->getPriority())
        return first->getPriority() < second->getPriority();

    return first->getId() > second->getId();
}

void JobScheduler::addJob(const std::shared_ptr<Job>& job)
{
    if (job == nullptr)
        return;

    jobs.push(job);
}

bool JobScheduler::tryAddJob(const std::shared_ptr<Job>& job)
{
    if (job == nullptr)
        return false;

    try
    {
        jobs.push(job);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

std::shared_ptr<Job> JobScheduler::getNextJob()
{
    if (jobs.empty())
        return nullptr;

    std::shared_ptr<Job> job = jobs.top();
    jobs.pop();

    return job;
}

bool JobScheduler::isEmpty() const
{
    return jobs.empty();
}

int JobScheduler::getJobCount() const
{
    return static_cast<int>(jobs.size());
}
