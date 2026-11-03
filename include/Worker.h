#ifndef WORKER_H
#define WORKER_H

#include <memory>

#include "Job.h"
#include "Database.h"

enum class ExecutionResult
{
    Success,
    Failed,
    InvalidState,
    DatabaseError
};

class Worker
{
private:
    int workerId;
    Database& database;

public:
    Worker(int workerId, Database& database);

    ExecutionResult execute(const std::shared_ptr<Job>& job);
};

#endif