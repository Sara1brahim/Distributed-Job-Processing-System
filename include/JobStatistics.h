#ifndef JOB_STATISTICS_H
#define JOB_STATISTICS_H

#include <vector>
#include <memory>

#include "Job.h"

class JobStatistics
{
public:
    void generateReport(
        const std::vector<std::shared_ptr<Job>>& jobs
    ) const;
};

#endif