#include "Database.h"

#include <iostream>

Database::Database()
        : db(nullptr),
          jobCache(100, std::chrono::seconds(30))
{
}

Database::~Database()
{
    disconnect();
}

bool Database::connect(const std::string& databasePath)
{
    std::lock_guard<std::mutex> lock(databaseMutex);

    if (db != nullptr)
    {
        std::cerr << "Database is already connected.\n";
        return false;
    }

    int result = sqlite3_open(databasePath.c_str(), &db);

    if (result != SQLITE_OK)
    {
        std::cerr << "Database connection failed: "
                  << (db != nullptr ? sqlite3_errmsg(db) : "Unknown error")
                  << '\n';

        if (db != nullptr)
        {
            sqlite3_close_v2(db);
            db = nullptr;
        }

        return false;
    }

    std::cout << "Database connected successfully!\n";
    return true;
}

void Database::disconnect()
{
    std::lock_guard<std::mutex> lock(databaseMutex);

    if (db == nullptr)
        return;

    int result = sqlite3_close(db);

    if (result != SQLITE_OK)
    {
        std::cerr << "Database close failed: "
                  << sqlite3_errmsg(db) << '\n';
        return;
    }

    db = nullptr;

    jobCache.clear();

    std::cout << "Database connection closed.\n";
}

bool Database::execute(const std::string& sql)
{
    std::lock_guard<std::mutex> lock(databaseMutex);

    if (db == nullptr)
    {
        std::cerr << "Error: Database is not connected.\n";
        return false;
    }

    char* errorMessage = nullptr;

    int result = sqlite3_exec(
            db,
            sql.c_str(),
            nullptr,
            nullptr,
            &errorMessage
    );

    if (result != SQLITE_OK)
    {
        std::cerr << "SQL Error: "
                  << (errorMessage != nullptr
                      ? errorMessage
                      : sqlite3_errmsg(db))
                  << '\n';

        sqlite3_free(errorMessage);
        return false;
    }

    // Any raw SQL command may change the Jobs table.
    // Clear the cache to prevent stale data.
    jobCache.clear();

    return true;
}

bool Database::initializeSchema()
{
    std::string sql = R"(
        CREATE TABLE IF NOT EXISTS Jobs
        (
            id INTEGER PRIMARY KEY,
            type TEXT NOT NULL,
            priority INTEGER NOT NULL,
            status TEXT NOT NULL,
            retry_count INTEGER NOT NULL,
            max_retries INTEGER NOT NULL,
            attempt_count INTEGER NOT NULL,
            failures_before_success INTEGER NOT NULL,
            execution_duration REAL DEFAULT 0,
            retry_waiting_time REAL DEFAULT 0,
            total_processing_time REAL DEFAULT 0
        );
    )";

    return execute(sql);
}

bool Database::insertJob(const Job& job)
{
    std::lock_guard<std::mutex> lock(databaseMutex);

    if (db == nullptr)
    {
        std::cerr << "Error: Database is not connected.\n";
        return false;
    }

    const char* sql = R"(
        INSERT INTO Jobs
        (
            id, type, priority, status, retry_count,
            max_retries, attempt_count, failures_before_success,
            execution_duration, retry_waiting_time,
            total_processing_time
        )
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);
    )";

    sqlite3_stmt* statement = nullptr;

    int result = sqlite3_prepare_v2(
            db,
            sql,
            -1,
            &statement,
            nullptr
    );

    if (result != SQLITE_OK)
    {
        std::cerr << "Prepare Error: "
                  << sqlite3_errmsg(db) << '\n';
        return false;
    }

    std::string status = jobStatusToString(job.getStatus());
    std::string type = job.getType();

    result = sqlite3_bind_int(statement, 1, job.getId());

    if (result == SQLITE_OK)
        result = sqlite3_bind_text(
                statement, 2, type.c_str(), -1, SQLITE_TRANSIENT
        );

    if (result == SQLITE_OK)
        result = sqlite3_bind_int(statement, 3, job.getPriority());

    if (result == SQLITE_OK)
        result = sqlite3_bind_text(
                statement, 4, status.c_str(), -1, SQLITE_TRANSIENT
        );

    if (result == SQLITE_OK)
        result = sqlite3_bind_int(statement, 5, job.getRetryCount());

    if (result == SQLITE_OK)
        result = sqlite3_bind_int(statement, 6, job.getMaxRetries());

    if (result == SQLITE_OK)
        result = sqlite3_bind_int(statement, 7, job.getAttemptCount());

    if (result == SQLITE_OK)
        result = sqlite3_bind_int(
                statement, 8, job.getFailuresBeforeSuccess()
        );

    if (result == SQLITE_OK)
        result = sqlite3_bind_double(
                statement, 9, job.getExecutionDuration()
        );

    if (result == SQLITE_OK)
        result = sqlite3_bind_double(
                statement, 10, job.getRetryWaitingTime()
        );

    if (result == SQLITE_OK)
        result = sqlite3_bind_double(
                statement, 11, job.getTotalProcessingTime()
        );

    if (result == SQLITE_OK)
        result = sqlite3_step(statement);

    sqlite3_finalize(statement);

    if (result != SQLITE_DONE)
    {
        std::cerr << "Insert Error: "
                  << sqlite3_errmsg(db) << '\n';
        return false;
    }

    jobCache.invalidate(job.getId());

    return true;
}

bool Database::updateJob(const Job& job)
{
    std::lock_guard<std::mutex> lock(databaseMutex);

    if (db == nullptr)
    {
        std::cerr << "Error: Database is not connected.\n";
        return false;
    }

    const char* sql = R"(
        UPDATE Jobs
        SET
            type = ?,
            priority = ?,
            status = ?,
            retry_count = ?,
            max_retries = ?,
            attempt_count = ?,
            failures_before_success = ?,
            execution_duration = ?,
            retry_waiting_time = ?,
            total_processing_time = ?
        WHERE id = ?;
    )";

    sqlite3_stmt* statement = nullptr;

    int result = sqlite3_prepare_v2(
            db,
            sql,
            -1,
            &statement,
            nullptr
    );

    if (result != SQLITE_OK)
    {
        std::cerr << "Prepare Error: "
                  << sqlite3_errmsg(db) << '\n';
        return false;
    }

    std::string status = jobStatusToString(job.getStatus());
    std::string type = job.getType();

    result = sqlite3_bind_text(
            statement, 1, type.c_str(), -1, SQLITE_TRANSIENT
    );

    if (result == SQLITE_OK)
        result = sqlite3_bind_int(statement, 2, job.getPriority());

    if (result == SQLITE_OK)
        result = sqlite3_bind_text(
                statement, 3, status.c_str(), -1, SQLITE_TRANSIENT
        );

    if (result == SQLITE_OK)
        result = sqlite3_bind_int(statement, 4, job.getRetryCount());

    if (result == SQLITE_OK)
        result = sqlite3_bind_int(statement, 5, job.getMaxRetries());

    if (result == SQLITE_OK)
        result = sqlite3_bind_int(statement, 6, job.getAttemptCount());

    if (result == SQLITE_OK)
        result = sqlite3_bind_int(
                statement, 7, job.getFailuresBeforeSuccess()
        );

    if (result == SQLITE_OK)
        result = sqlite3_bind_double(
                statement, 8, job.getExecutionDuration()
        );

    if (result == SQLITE_OK)
        result = sqlite3_bind_double(
                statement, 9, job.getRetryWaitingTime()
        );

    if (result == SQLITE_OK)
        result = sqlite3_bind_double(
                statement, 10, job.getTotalProcessingTime()
        );

    if (result == SQLITE_OK)
        result = sqlite3_bind_int(statement, 11, job.getId());

    if (result == SQLITE_OK)
        result = sqlite3_step(statement);

    int affectedRows = 0;

    if (result == SQLITE_DONE)
        affectedRows = sqlite3_changes(db);

    sqlite3_finalize(statement);

    if (result != SQLITE_DONE)
    {
        std::cerr << "Update Error: "
                  << sqlite3_errmsg(db) << '\n';
        return false;
    }

    if (affectedRows == 0)
    {
        std::cerr << "Update Warning: Job "
                  << job.getId() << " was not found.\n";
        return false;
    }

    jobCache.put(job);

    return true;
}

bool Database::getJobById(int jobId, JobSnapshot& result)
{
    std::lock_guard<std::mutex> lock(databaseMutex);

    if (db == nullptr)
    {
        std::cerr << "Error: Database is not connected.\n";
        return false;
    }

    // First, try to retrieve the Job from the cache.
    if (jobCache.get(jobId, result))
    {
        std::cout << "Cache HIT for Job "
                  << jobId << '\n';

        return true;
    }

    std::cout << "Cache MISS for Job "
              << jobId << ". Reading from database...\n";

    const char* sql = R"(
        SELECT
            id,
            type,
            priority,
            status,
            retry_count,
            max_retries,
            attempt_count,
            execution_duration,
            retry_waiting_time,
            total_processing_time
        FROM Jobs
        WHERE id = ?;
    )";

    sqlite3_stmt* statement = nullptr;

    int queryResult = sqlite3_prepare_v2(
            db,
            sql,
            -1,
            &statement,
            nullptr
    );

    if (queryResult != SQLITE_OK)
    {
        std::cerr << "Select Prepare Error: "
                  << sqlite3_errmsg(db) << '\n';
        return false;
    }

    queryResult = sqlite3_bind_int(statement, 1, jobId);

    if (queryResult == SQLITE_OK)
        queryResult = sqlite3_step(statement);

    if (queryResult != SQLITE_ROW)
    {
        if (queryResult != SQLITE_DONE)
        {
            std::cerr << "Select Error: "
                      << sqlite3_errmsg(db) << '\n';
        }

        sqlite3_finalize(statement);
        return false;
    }

    const unsigned char* typeText =
            sqlite3_column_text(statement, 1);

    const unsigned char* statusText =
            sqlite3_column_text(statement, 3);

    result.id = sqlite3_column_int(statement, 0);

    result.type = typeText != nullptr
                  ? reinterpret_cast<const char*>(typeText)
                  : "";

    result.priority = sqlite3_column_int(statement, 2);

    result.status = statusText != nullptr
                    ? reinterpret_cast<const char*>(statusText)
                    : "";

    result.retryCount = sqlite3_column_int(statement, 4);
    result.maxRetries = sqlite3_column_int(statement, 5);
    result.attemptCount = sqlite3_column_int(statement, 6);

    result.executionDuration =
            sqlite3_column_double(statement, 7);

    result.retryWaitingTime =
            sqlite3_column_double(statement, 8);

    result.totalProcessingTime =
            sqlite3_column_double(statement, 9);

    sqlite3_finalize(statement);

    // Store the database result in the cache for future reads.
    jobCache.put(result);

    return true;
}

bool Database::displayAllJobs()
{
    std::lock_guard<std::mutex> lock(databaseMutex);

    if (db == nullptr)
    {
        std::cerr << "Error: Database is not connected.\n";
        return false;
    }

    const char* sql = R"(
        SELECT
            id,
            type,
            priority,
            status,
            retry_count,
            max_retries,
            attempt_count,
            execution_duration,
            retry_waiting_time,
            total_processing_time
        FROM Jobs
        ORDER BY id;
    )";

    sqlite3_stmt* statement = nullptr;

    int result = sqlite3_prepare_v2(
            db,
            sql,
            -1,
            &statement,
            nullptr
    );

    if (result != SQLITE_OK)
    {
        std::cerr << "Select Error: "
                  << sqlite3_errmsg(db) << '\n';
        return false;
    }

    std::cout << "\n========== DATABASE JOBS ==========\n";

    while ((result = sqlite3_step(statement)) == SQLITE_ROW)
    {
        std::cout
                << "ID: " << sqlite3_column_int(statement, 0)
                << " | Type: " << sqlite3_column_text(statement, 1)
                << " | Priority: " << sqlite3_column_int(statement, 2)
                << " | Status: " << sqlite3_column_text(statement, 3)
                << " | Retries: " << sqlite3_column_int(statement, 4)
                << " | Max Retries: " << sqlite3_column_int(statement, 5)
                << " | Attempts: " << sqlite3_column_int(statement, 6)
                << " | Execution: " << sqlite3_column_double(statement, 7)
                << " sec | Retry Waiting: "
                << sqlite3_column_double(statement, 8)
                << " sec | Total: "
                << sqlite3_column_double(statement, 9)
                << " sec\n";
    }

    sqlite3_finalize(statement);

    if (result != SQLITE_DONE)
    {
        std::cerr << "Read Error: "
                  << sqlite3_errmsg(db) << '\n';
        return false;
    }

    std::cout << "===================================\n";

    return true;
}

CacheStatistics Database::getCacheStatistics() const
{
    return jobCache.getStatistics();
}

std::size_t Database::getCacheSize() const
{
    return jobCache.getCacheSize();
}

bool Database::isConnected() const
{
    std::lock_guard<std::mutex> lock(databaseMutex);

    return db != nullptr;
}
