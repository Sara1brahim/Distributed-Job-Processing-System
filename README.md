# Distributed Job Processing System

A C++20 project exploring distributed job processing through a Coordinator–Worker architecture, TCP communication, multithreading, job scheduling, persistent storage, and caching.

## Overview

The system is designed around a Coordinator that manages incoming jobs and distributes them to independent Worker processes. Workers communicate with the Coordinator over TCP, receive jobs, execute them, and report their results.

The project also includes a local job processing engine with scheduling, retry handling, SQLite persistence, execution statistics, and an in-memory cache.

## Architecture

```text
                 Coordinator
                      |
                TCP Communication
                 /          \
            Worker 1       Worker 2
                 \          /
                  Job Results
```

### Main Components

- **Coordinator Server:** Accepts worker connections, handles registration, assigns available jobs, and receives execution results.
- **Remote Worker Client:** Connects to the Coordinator, requests jobs, executes them, and reports results.
- **Job Scheduler:** Uses a priority queue to organize jobs based on priority.
- **Worker Pool:** Manages local worker threads and job execution.
- **Job Management:** Tracks job states, execution attempts, retries, and timing information.
- **Database:** Uses SQLite for persistent job storage.
- **Job Cache:** Implements an LRU cache with TTL expiration to reduce repeated database reads.
- **Job Statistics:** Reports job counts, success/failure rates, and execution-time metrics.

## Implemented Features

### Distributed Communication Prototype
- TCP client-server communication using Windows Winsock.
- Worker registration with the Coordinator.
- Multiple independent Worker processes can connect to the Coordinator.
- Basic job dispatching from the Coordinator to connected Workers.
- Workers execute assigned jobs and report their results.
- Thread-safe access to the Coordinator's pending job queue.

### Local Job Processing Engine
- Priority-based job scheduling.
- Multithreaded job execution.
- Job lifecycle management: Pending, Running, Completed, and Failed.
- Retry handling for failed jobs.
- SQLite database integration.
- LRU caching with configurable capacity and TTL.
- Cache hit/miss statistics.
- Job execution and processing statistics.

## Technologies Used

- C++20
- Object-Oriented Programming
- Data Structures: Priority Queue, List, Hash Map
- Multithreading and Synchronization
- TCP/IP Networking
- Windows Winsock API
- SQLite
- CMake
- CLion

## Project Structure

```text
Distributed-Job-Processing-System/
│
├── include/
│   ├── network/
│   ├── Job.h
│   ├── JobManager.h
│   ├── JobScheduler.h
│   ├── Worker.h
│   ├── WorkerPool.h
│   ├── Database.h
│   ├── JobCache.h
│   └── JobStatistics.h
│
├── src/
│   ├── network/
│   ├── Job.cpp
│   ├── JobManager.cpp
│   ├── JobScheduler.cpp
│   ├── Worker.cpp
│   ├── WorkerPool.cpp
│   ├── Database.cpp
│   ├── JobCache.cpp
│   ├── JobStatistics.cpp
│   ├── main.cpp
│   ├── CoordinatorMain.cpp
│   └── RemoteWorkerMain.cpp
│
├── tests/
│   └── JobCacheTests.cpp
│
├── third_party/
│   └── sqlite/
│
└── CMakeLists.txt
```

## Build Requirements

- Windows
- C++20-compatible compiler
- CMake 3.31 or later
- Ninja or another supported CMake generator

SQLite amalgamation source files are included in `third_party/sqlite`.

## Build

From the project root:

```bash
cmake -S . -B build
cmake --build build
```

## Running the Distributed Communication Prototype

Start the Coordinator first:

```bash
./build/Coordinator
```

Then start one or more Worker processes:

```bash
./build/RemoteWorker 1
./build/RemoteWorker 2
```

Each Worker connects to the Coordinator on port `54000`, registers its ID, requests available jobs, executes them, and sends execution results back.

> Run the Coordinator and each Worker in separate terminal windows. The current network prototype uses `127.0.0.1`, so the processes communicate on the same machine.

## Running Cache Tests

Build and run the `JobCacheTests` target:

```bash
cmake --build build --target JobCacheTests
```

Run the generated test executable from the build directory.

The test suite covers:
- Basic cache insertion and retrieval.
- Cache misses.
- LRU eviction.
- TTL expiration.
- Cache invalidation.
- Updating existing entries.
- Clearing the cache.
- Cache statistics.

## Current Development Status

The local job processing engine, SQLite-backed storage, LRU/TTL cache, and initial TCP-based Coordinator–Worker communication prototype have been implemented.

The next development phase focuses on integrating distributed execution with the existing job scheduler and database, followed by worker failure recovery, reliability improvements, and end-to-end testing.

## Author

**Sara Ibrahim**

- GitHub: [@Sara1brahim](https://github.com/Sara1brahim)
- Project: [Distributed Job Processing System](https://github.com/Sara1brahim/Distributed-Job-Processing-System)
