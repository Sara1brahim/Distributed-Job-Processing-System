#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <mutex>

#include "Job.h"
#include "JobCache.h"

#include <sqlite3.h>

class Database
{
private:
    sqlite3* db;
    mutable std::mutex databaseMutex;

    JobCache jobCache;

public:
    Database();
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    bool connect(const std::string& databasePath);
    void disconnect();

    bool execute(const std::string& sql);
    bool initializeSchema();

    bool insertJob(const Job& job);
    bool updateJob(const Job& job);

    bool getJobById(int jobId, JobSnapshot& result);

    bool displayAllJobs();

    CacheStatistics getCacheStatistics() const;
    std::size_t getCacheSize() const;

    bool isConnected() const;
};

#endif