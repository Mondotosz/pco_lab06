#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <cassert>
#include <chrono>
#include <iostream>
#include <map>
#include <memory>
#include <pcosynchro/pcohoaremonitor.h>
#include <pcosynchro/pcologger.h>
#include <pcosynchro/pcomanager.h>
#include <pcosynchro/pcothread.h>
#include <set>
#include <stack>
#include <utility>
#include <vector>
#include <queue>

class Runnable
{
public:
    virtual ~Runnable() = default;
    virtual void run() = 0;
    virtual void cancelRun() = 0;
    virtual std::string id() = 0;
};

class ThreadPool : public PcoHoareMonitor
{
public:
    ThreadPool(int maxThreadCount, int maxNbWaiting, std::chrono::milliseconds idleTimeout)
        : maxThreadCount(maxThreadCount)
        , maxNbWaiting(maxNbWaiting)
        , idleTimeout(idleTimeout)
        , threads()
        , queue()
        , nbAvailable(0)
    {}

    ~ThreadPool()
    {
        monitorIn();

        // Request stop on all threads
        for (auto it = threads.begin(); it != threads.end(); ++it) {
            it->second->requestStop();
            signal(cond);
        }

        monitorOut();

        // Wait for the threads to stop properly
        for (auto it = threads.begin(); it != threads.end(); ++it) {
            it->second->join();
        }

        // TODO: rest of the cleanup
    }

    /*
     * Start a runnable. If a thread in the pool is avaible, assign the
     * runnable to it. If no thread is available but the pool can grow, create a new
     * pool thread and assign the runnable to it. If no thread is available and the
     * pool is at max capacity and there are less than maxNbWaiting threads waiting,
     * block the caller until a thread becomes available again, and else do not run the runnable.
     * If the runnable has been started, returns true, and else (the last case), return false.
     */
    bool start(std::unique_ptr<Runnable> runnable)
    {
        monitorIn();
        if (nbAvailable > queue.size()) {
            // NOTE: A thread can handle the runnable

            // TODO: add to queue and signal
        } else if (threads.size() < maxThreadCount) {
            // NOTE: A thread can be created to handle the runnable

            std::unique_ptr<PcoThread> thread
                = std::make_unique<PcoThread>(&ThreadPool::worker, this, runnable);
            threads.insert(std::pair{thread.get(), std::move(thread)});

        } else if (queue.size() < maxNbWaiting) {
            // NOTE: We still have space in the queue

            // TODO: add to the queue and signal
        } else {
            monitorOut();
            // If none of the above actions are available, default to refusing the
            // task
            return false;
        }

        monitorOut();
        return true;
    }

    /* Returns the number of currently running threads. They do not need to be executing a task,
     * just to be alive.
     */
    size_t currentNbThreads()
    {
        // TODO:
        return 0;
    }

private:
    size_t maxThreadCount;
    size_t maxNbWaiting;
    size_t nbAvailable;
    Condition cond;
    std::chrono::milliseconds idleTimeout;

    /**
    * A map to store the threads, that way the workers can remove themselves from
    * the map before returning. We can't use a set since we need a unique_ptr to
    * automatically free the pointer and we can't use a unique_ptr as a key
    */
    std::map<PcoThread *, std::unique_ptr<PcoThread>> threads;

    std::queue<std::unique_ptr<Runnable>> queue;

    void worker(std::unique_ptr<Runnable> runnable)
    {
        runnable->run();
        auto _ = runnable.release();

        while (true) {
        }
    }
};

#endif // THREADPOOL_H
