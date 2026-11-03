#include "JobStatistics.h"

#include <iostream>
#include <iomanip>

void JobStatistics::generateReport(
    const std::vector<std::shared_ptr<Job>>& jobs
) const
{
    int totalJobs = jobs.size();

    int completedJobs = 0;
    int failedJobs = 0;
    int pendingJobs = 0;
    int runningJobs = 0;

    double totalExecutionDuration = 0.0;
    double totalProcessingTime = 0.0;

    int finishedJobs = 0;

    for (int i = 0; i < totalJobs; i++)
    {
        std::shared_ptr<Job> job = jobs[i];

        if (job == nullptr)
            continue;

        switch (job->getStatus())
        {
            case JobStatus::Completed:
                completedJobs++;
                break;

            case JobStatus::Failed:
                failedJobs++;
                break;

            case JobStatus::Pending:
                pendingJobs++;
                break;

            case JobStatus::Running:
                runningJobs++;
                break;
        }

        if (job->getStatus() == JobStatus::Completed ||
            job->getStatus() == JobStatus::Failed)
        {
            totalExecutionDuration += job->getExecutionDuration();
            totalProcessingTime += job->getTotalProcessingTime();

            finishedJobs++;
        }
    }

    double successRate = 0.0;
    double failureRate = 0.0;

    if (finishedJobs > 0)
    {
        successRate =
            (static_cast<double>(completedJobs) / finishedJobs) * 100;

        failureRate =
            (static_cast<double>(failedJobs) / finishedJobs) * 100;
    }

    double averageExecutionDuration = 0.0;
    double averageProcessingTime = 0.0;

    if (finishedJobs > 0)
    {
        averageExecutionDuration =
            totalExecutionDuration / finishedJobs;

        averageProcessingTime =
            totalProcessingTime / finishedJobs;
    }

    std::cout << "\n========== SYSTEM STATISTICS ==========\n";

    std::cout << "Total Jobs: " << totalJobs << std::endl;
    std::cout << "Completed Jobs: " << completedJobs << std::endl;
    std::cout << "Failed Jobs: " << failedJobs << std::endl;
    std::cout << "Pending Jobs: " << pendingJobs << std::endl;
    std::cout << "Running Jobs: " << runningJobs << std::endl;

    std::cout << std::fixed << std::setprecision(2);

    std::cout << "\nSuccess Rate: "
              << successRate << "%" << std::endl;

    std::cout << "Failure Rate: "
              << failureRate << "%" << std::endl;

    std::cout << "\nAverage Execution Duration: "
              << averageExecutionDuration << " seconds"
              << std::endl;

    std::cout << "Average Processing Time: "
              << averageProcessingTime << " seconds"
              << std::endl;

    std::cout << "=======================================\n";
}