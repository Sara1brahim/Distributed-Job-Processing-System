cmake_minimum_required(VERSION 3.31)

project(Distributed_Job_Processing_System LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

add_executable(Distributed_Job_Processing_System
        src/main.cpp
        src/Job.cpp
        src/JobManager.cpp
        src/JobScheduler.cpp
        src/Worker.cpp
        src/WorkerPool.cpp
        src/JobStatistics.cpp
        src/Database.cpp
        src/JobCache.cpp
        third_party/sqlite/sqlite3.c
)

target_include_directories(Distributed_Job_Processing_System PRIVATE
        third_party/sqlite
)

add_executable(JobCacheTests
        tests/JobCacheTests.cpp
        src/JobCache.cpp
        src/Job.cpp
)

target_include_directories(JobCacheTests PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

# Coordinator Server
add_executable(Coordinator
        src/CoordinatorMain.cpp
        src/network/CoordinatorServer.cpp
)

target_include_directories(Coordinator PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(Coordinator PRIVATE
        ws2_32
)


# Independent Remote Worker
add_executable(RemoteWorker
        src/RemoteWorkerMain.cpp
        src/network/WorkerClient.cpp
)

target_include_directories(RemoteWorker PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(RemoteWorker PRIVATE
        ws2_32
)