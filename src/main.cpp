

#include <iostream>
#include <vector>

#include "JobManager.h"
#include "WorkerPool.h"
#include "JobStatistics.h"
#include "Database.h"
#include <stdexcept>

using namespace std;

int main()
{
Database database;

if (!database.connect("jobs.db"))
{
return 1;
}

if (!database.initializeSchema())
{
cerr << "Failed to initialize database schema.\n";
return 1;
}

cout << "Database schema initialized successfully!\n";

// Clear previous test data
if (!database.execute("DELETE FROM Jobs;"))
{
cerr << "Failed to clear previous jobs.\n";
return 1;
}
    try
    {
        WorkerPool invalidPool(0, database);
    }
    catch (const std::invalid_argument& error)
    {
        cout << "Expected error: " << error.what() << endl;
    }
JobManager manager;

manager.addJob(Job(101, "ImageProcessing", 2));
manager.addJob(Job(102, "ReportGeneration", 8));
manager.addJob(Job(103, "FileProcessing", 5));
manager.addJob(Job(104, "EmailSending", 3));

manager.addJob(Job(105, "DataProcessing", 10, 2, 5));
manager.addJob(Job(106, "Backup", 1));

cout << "Total Jobs: " << manager.getJobCount() << endl;

// Save all jobs before execution
for (int id = 101; id <= 106; id++)
{
shared_ptr<Job> job = manager.getJob(id);

if (!database.insertJob(*job))
{
cerr << "Failed to insert Job " << id << endl;
return 1;
}
}

cout << "All jobs saved to database successfully!\n";
    cout << "\n========== CACHE TEST ==========\n";

    JobSnapshot cachedJob;

    cout << "\nFirst read:\n";

    if (database.getJobById(102, cachedJob))
    {
        cout << "Job ID: " << cachedJob.id
             << " | Status: " << cachedJob.status << '\n';
    }

    cout << "\nSecond read:\n";

    if (database.getJobById(102, cachedJob))
    {
        cout << "Job ID: " << cachedJob.id
             << " | Status: " << cachedJob.status << '\n';
    }

    CacheStatistics cacheStats = database.getCacheStatistics();

    cout << "\nCache Hits: " << cacheStats.hits << '\n';
    cout << "Cache Misses: " << cacheStats.misses << '\n';
    cout << "Cache Hit Rate: "
         << cacheStats.getHitRate() << "%\n";

    cout << "Cache Size: "
         << database.getCacheSize() << '\n';

    cout << "================================\n\n";

WorkerPool pool(3, database);

for (int id = 101; id <= 106; id++)
{
pool.submitJob(manager.getJob(id));
}

    pool.shutdown();

    cout << "First shutdown completed.\n";

    pool.shutdown();

    cout << "Second shutdown completed.\n";

cout << "\nFinal Job Status:\n";

for (int id = 101; id <= 106; id++)
{
shared_ptr<Job> job = manager.getJob(id);

cout << "Job " << job->getId()
<< " - " << job->getType()
<< " - Priority: " << job->getPriority()
<< " - Status: " << jobStatusToString(job->getStatus())
<< " - Attempts: " << job->getAttemptCount()
<< " - Retries: " << job->getRetryCount()
<< " - Execution Duration: "
<< job->getExecutionDuration() << " seconds"
<< " - Retry Waiting Time: "
<< job->getRetryWaitingTime() << " seconds"
<< " - Total Processing Time: "
<< job->getTotalProcessingTime() << " seconds"
<< endl;
}

vector<shared_ptr<Job>> allJobs;

for (int id = 101; id <= 106; id++)
{
allJobs.push_back(manager.getJob(id));
}

JobStatistics statistics;

statistics.generateReport(allJobs);
database.displayAllJobs();

return 0;
}
