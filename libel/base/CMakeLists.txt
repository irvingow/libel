set(base_SRCS
        asynlogging.cpp
        countdown_latch.cpp
        condition.cpp
        fileutil.cpp
        logfile.cpp
        logging.cpp
        logstream.cpp
        num2string.cpp
        Thread.cpp
        waiter.cpp
        Mutex.cpp
        timestamp.cpp
        threadpool.cpp)

add_library(libel_base ${base_SRCS})
target_link_libraries(libel_base pthread rt)

file(GLOB HEADERS "*.h")

add_subdirectory(tests)