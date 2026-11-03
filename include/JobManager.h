#ifndef JOB_MANAGER_H
#define JOB_MANAGER_H

#include <unordered_map>
#include <memory>
#include "Job.h"

class JobManager
{
private:
    std::unordered_map<int, std::shared_ptr<Job>> jobs;

public:
    bool addJob(const Job& job);
    std::shared_ptr<Job> getJob(int id);
    int getJobCount() const;
};

#endif