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
        , deleting(false)
        , timer_thread(std::make_unique<PcoThread>(&ThreadPool::timer, this))
        , timer_waiting(false)
    {}

    ~ThreadPool()
    {
        monitorIn();

        // TODO: Stop and join the timer first to ensure that there is no way
        // for the threads map to be updated

        deleting = true;

        timer_thread->requestStop();
        signal(timer_cond);
        timer_thread->join();

        // Request stop on all threads
        for (auto it = threads.begin(); it != threads.end(); ++it) {
            it->second.thread->requestStop();
        }

        for (auto it = threads.begin(); it != threads.end(); ++it) {
            if (it->second.waiting) {
                signal(*it->second.cond);
            }
        }

        // NOTE: we can safely leave the monitor since we are joining every
        // thread before interacting with shared data
        monitorOut();

        // Wait for the threads to stop properly
        for (auto it = threads.begin(); it != threads.end(); ++it) {
            it->second.thread->join();
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
            queue.push(std::move(runnable));
            for (auto it = threads.begin(); it != threads.end(); ++it) {
                if (it->second.waiting) {
                    signal(*it->second.cond);
                    break;
                }
            }
            // signal(*std::find_if(threads.begin(), threads.end(), [](auto val) {
            //             return val.second.waiting;
            //         })->second.cond);
        } else if (threads.size() < maxThreadCount) {
            // NOTE: A thread can be created to handle the runnable

            queue.push(std::move(runnable));
            worker_t
                tmp{std::make_unique<PcoThread>(&ThreadPool::worker, this),
                    std::make_unique<Condition>(),
                    false};
            threads.insert(std::pair{tmp.thread.get(), std::move(tmp)});

        } else if (queue.size() < maxNbWaiting) {
            // NOTE: We still have space in the queue
            queue.push(std::move(runnable));
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
    size_t currentNbThreads() { return threads.size(); }

private:
    typedef typename std::chrono::steady_clock Clock;
    typedef typename std::chrono::time_point<Clock> TimePoint;
    typedef typename ::PcoThread *Key;
    typedef typename std::pair<PcoThread *, std::pair<TimePoint, Condition *>> TimeOutNode;

    size_t maxThreadCount;
    size_t maxNbWaiting;
    size_t nbAvailable;
    Condition timer_cond;
    std::chrono::milliseconds idleTimeout;

    struct worker_t
    {
        std::unique_ptr<PcoThread> thread;
        std::unique_ptr<Condition> cond;
        bool waiting;
    };

    /**
    * A map to store the threads, that way the workers can remove themselves from
    * the map before returning. We can't use a set since we need a unique_ptr to
    * automatically free the pointer and we can't use a unique_ptr as a key
    */
    std::map<Key, worker_t> threads;

    std::map<Key, std::pair<TimePoint, Condition *>> timeouts;

    std::queue<std::unique_ptr<Runnable>> queue;

    std::unique_ptr<PcoThread> timer_thread;

    bool deleting;
    bool timer_waiting;

    void worker()
    {
        static std::atomic<size_t> snum = 0;
        size_t num = snum++;

        monitorIn();
        worker_t &wrkr = threads.at(PcoThread::thisThread());
        monitorOut();

        while (true) {
            std::cout << "[worker" << num << "] <monitorIn" << std::endl;
            monitorIn();
            std::cout << "[worker" << num << "] monitorIn>" << std::endl;

            // std::cout << "[worker" << num << "] In" << std::endl;
            // Set the timeout
            TimePoint timeout = Clock::now() + idleTimeout;
            timeouts.insert(std::pair{PcoThread::thisThread(), std::pair{timeout, wrkr.cond.get()}});

            while (queue.empty() && !PcoThread::thisThread()->stopRequested()) {
                if (num == 9) {
                    std::cout << "noop" << std::endl;
                }
                wrkr.waiting = true;
                std::cout << "[worker" << num << "] waiting" << std::endl;
                if (timer_waiting) {
                    std::cout << "[worker" << num << "] Signaling timer" << std::endl;
                    signal(timer_cond);
                }
                ++nbAvailable;
                wait(*wrkr.cond);
                wrkr.waiting = false;
                --nbAvailable;
                if (Clock::now() > timeout) {
                    std::cout << "[worker" << num << "] requesting self stop" << std::endl;
                    PcoThread::thisThread()->requestStop();
                }
            }

            // Erase the timeout now that we've got a task or got canceled
            timeouts.erase(PcoThread::thisThread());

            if (PcoThread::thisThread()->stopRequested()) {
                break;
            }

            std::unique_ptr<Runnable> runnable = std::move(queue.front());
            queue.pop();

            // std::cout << "[worker" << num << "] out" << std::endl;
            monitorOut();

            runnable->run();
            // std::cout << "[worker" << num << "] ran" << std::endl;
        }

        std::cout << "[worker" << num << "] exiting" << std::endl;
        if (!deleting) {
            std::cout << "[worker" << num << "] erasing" << std::endl;
            auto _ = threads.at(PcoThread::thisThread()).thread.release();
            threads.erase(PcoThread::thisThread());
        }
        monitorOut();
        std::cout << "[worker" << num << "] exited" << std::endl;
    }

    void timer()
    {
        std::atomic<size_t> u = 0;
        monitorIn();
        while (true) {
            std::cout << "[timer] loop: " << u++ << std::endl;
            while (timeouts.empty() && !PcoThread::thisThread()->stopRequested()) {
                timer_waiting = true;
                wait(timer_cond);
                timer_waiting = false;
                std::cout << "[timer] signaled" << std::endl;
            }
            std::cout << "[timer] passed cond" << std::endl;

            if (PcoThread::thisThread()->stopRequested()) {
                break;
            }
            std::cout << "[timer] passed stop requested" << std::endl;

            // Get the next timeout
            auto end = timeouts.end();
            auto m = std::min_element(timeouts.begin(), end, [](TimeOutNode lhs, TimeOutNode rhs) {
                return lhs.second.first < rhs.second.first;
            });

            std::cout << "[timer] first monitorOut" << std::endl;
            monitorOut();

            if (m == end) {
                // NOTE: Something went wrong I guess
                std::cerr << "something went wrong" << std::endl;
                continue;
            }

            // sleep_until next timeout
            // NOTE: not sure if we should check that the time is valid
            std::this_thread::sleep_until(m->second.first);
            std::cout << "[timer] slept" << std::endl;

            monitorIn();
            std::cout << "[timer] second monitorIn" << std::endl;
            signal(*m->second.second);
            std::cout << "[timer] second monitorOut" << std::endl;
        }
        monitorOut();
    }
};

#endif // THREADPOOL_H
