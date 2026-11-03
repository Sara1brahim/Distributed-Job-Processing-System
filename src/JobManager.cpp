#include "JobManager.h"

bool JobManager::addJob(const Job& job)
{
    int id = job.getId();

    if (jobs.find(id) != jobs.end())
        return false;

    jobs.emplace(id, std::make_shared<Job>(job));
    return true;
}

std::shared_ptr<Job> JobManager::getJob(int id)
{
    if (jobs.find(id) == jobs.end())
        return nullptr;

    return jobs[id];
}

int JobManager::getJobCount() const
{
    return jobs.size();
}