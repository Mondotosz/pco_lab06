#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <iostream>
#include <map>
#include <memory>
#include <pcosynchro/pcohoaremonitor.h>
#include <pcosynchro/pcologger.h>
#include <pcosynchro/pcomanager.h>
#include <pcosynchro/pcosemaphore.h>
#include <pcosynchro/pcothread.h>
#include <set>
#include <stack>
#include <thread>
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
        , timer_thread(std::make_unique<PcoThread>(&ThreadPool::timer, this))
    {}

    ~ThreadPool()
    {
        timer_thread->requestStop();
        timer_thread->join();

        monitorIn();
        std::cout << "[~TheadPool] begin" << std::endl;

        for (auto it = threads.begin(); it != threads.end(); ++it) {
            it->second.thread->requestStop();
            if (it->second.waiting) {
                signal(*it->second.cond);
            }
        }

        std::cout << "[~TheadPool] middle" << std::endl;

        for (auto it = threads.begin(); it != threads.end(); ++it) {
            it->second.thread->join();
        }
        std::cout << "[~TheadPool] end" << std::endl;

        monitorOut();
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
        if (queue.size() >= maxNbWaiting) {
            // No place left
            monitorOut();
            return false;
        }

        queue.push(std::move(runnable));

        if (nbAvailable > queue.size()) {
            // NOTE: A worker is available
            for (auto it = threads.begin(); it != threads.end(); ++it) {
                if (it->second.waiting) {
                    signal(*it->second.cond);
                }
            }
        } else if (threads.size() < maxThreadCount) {
            // NOTE: We can still create more threads
            size_t id = next_thread_id++;
            threads.insert(std::pair{
                id,
                worker_t{
                    .thread = std::make_unique<PcoThread>(&ThreadPool::worker, this, id),
                    .cond = std::make_unique<Condition>(),
                    .waiting = false,
                    .timeout = {}}});
        }

        // NOTE: default action is just queuing since a worker will take the
        // job when available

        monitorOut();
        return true;
    }

    /* Returns the number of currently running threads. They do not need to be executing a task,
     * just to be alive.
     */
    size_t currentNbThreads() { return threads.size(); }

private:
    typedef typename std::chrono::steady_clock Clock;
    typedef typename std::chrono::time_point<Clock> TimePoint;
    typedef typename ::size_t Key;
    typedef typename std::pair<PcoThread *, std::pair<TimePoint, Condition *>> TimeOutNode;

    size_t maxThreadCount;
    size_t maxNbWaiting;
    size_t nbAvailable;
    size_t next_thread_id = 0;
    std::chrono::milliseconds idleTimeout;

    struct worker_t
    {
        std::unique_ptr<PcoThread> thread;
        std::unique_ptr<Condition> cond;
        bool waiting;
        TimePoint timeout;
    };

    /**
    * A map to store the threads, that way the workers can remove themselves from
    * the map before returning. We can't use a set since we need a unique_ptr to
    * automatically free the pointer and we can't use a unique_ptr as a key
    */
    std::map<Key, worker_t> threads;

    std::queue<std::unique_ptr<Runnable>> queue;

    std::unique_ptr<PcoThread> timer_thread;

    void worker(size_t id)
    {
        monitorIn();
        worker_t &wrkr = threads.at(id);
        monitorOut();

        while (true) {
            monitorIn();

            std::cout << "[worker" << id << "]" << "in" << std::endl;
            wrkr.timeout = Clock::now() + idleTimeout;

            while (queue.empty() && !PcoThread::thisThread()->stopRequested()) {
                wrkr.waiting = true;
                ++nbAvailable;
                wait(*wrkr.cond);
                --nbAvailable;
                wrkr.waiting = false;
            }

            if (PcoThread::thisThread()->stopRequested()) {
                break;
            }

            std::unique_ptr<Runnable> runnable = std::move(queue.front());
            queue.pop();

            std::cout << "[worker" << id << "]" << "out" << std::endl;
            monitorOut();

            runnable->run();
        }

        monitorOut();
    }

    void timer()
    {
        while (true) {
            monitorIn();

            std::cout << "[timer]" << "in" << std::endl;
            if (PcoThread::thisThread()->stopRequested()) {
                std::cout << "[timer]" << "break" << std::endl;
                break;
            }

            std::queue<size_t> deleted{};

            std::cout << "[timer]" << "iterating" << std::endl;
            for (auto it = threads.begin(); it != threads.end(); ++it) {
                if (it->second.waiting && it->second.timeout < Clock::now()) {
                    deleted.push(it->first);
                    it->second.thread->requestStop();

                    std::cout << "[timer]" << "<signal" << std::endl;
                    signal(*it->second.cond);
                    std::cout << "[timer]" << "signal>" << std::endl;
                }
            }

            std::cout << "[timer]" << "releasing" << std::endl;
            while (!deleted.empty()) {
                auto it = threads.find(deleted.front());
                it->second.thread->join();
                auto _ = it->second.thread.release();
                deleted.pop();
                threads.erase(it);
            }

            std::cout << "[timer]" << "out" << std::endl;
            monitorOut();

            // sleep for half a timing. Less precise but avoids issues for now
            std::this_thread::sleep_for(idleTimeout / 2);
        }
        monitorOut();
    }
};

#endif // THREADPOOL_H
