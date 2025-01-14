#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <cassert>
#include <chrono>
#include <map>
#include <memory>
#include <pcosynchro/pcohoaremonitor.h>
#include <pcosynchro/pcologger.h>
#include <pcosynchro/pcomanager.h>
#include <pcosynchro/pcosemaphore.h>
#include <pcosynchro/pcothread.h>
#include <thread>
#include <utility>
#include <queue>

// NOTE: could wrap this in #ifdef DEBUG
#define LOG_TIMER 0
#define LOG_DEL 0
#define LOG_WORK 0
#define LOG_IN_OUT 0
#define LOG_TASKS 0

#if LOG_IN_OUT
#include <atomic>
#endif

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
        // NOTE: we cannot call join within a monitor and we don't need to be
        // within the monitor to stop the timer.
        timer_thread->requestStop();
        timer_thread->join();

        monitorIn();
#if LOG_DEL > 1
        PcoLogger() << "[~TheadPool] begin" << std::endl;
#endif

        for (auto it = threads.begin(); it != threads.end(); ++it) {
#if LOG_DEL > 2
            PcoLogger() << "[~TheadPool] requestStop on: " << it->first << std::endl;
#endif
            it->second.thread->requestStop();
            // doesn't matter if the worker is waiting or not, signal() will
            // check by itself.
            signal(*it->second.cond);
        }

#if LOG_DEL > 1
        PcoLogger() << "[~TheadPool] middle" << std::endl;
        PcoLogger() << "[~TheadPool] nb threads: " << threads.size() << std::endl;

        PcoLogger() << "[~TheadPool] queue.size(): " << queue.size() << std::endl;
        PcoLogger() << "[~TheadPool] nbAvailable: " << nbAvailable << std::endl;
#endif

        monitorOut();

#if LOG_IN_OUT
        PcoLogger() << "[~TheadPool] nb in/out: " << in << "/" << out << std::endl;
#endif

        for (auto it = threads.begin(); it != threads.end(); ++it) {
#if LOG_DEL > 2
            PcoLogger() << "[~TheadPool] joining thread: " << it->first << std::endl;
#endif
            it->second.thread->join();
        }
#if LOG_TASKS
        PcoLogger() << "[~TheadPool] tasks accepted/refused/executed: " << accepted << "/"
                    << refused << "/" << executed << std::endl;
#endif
#if LOG_DEL
        PcoLogger() << "[~TheadPool] end" << std::endl;
#endif
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
#if LOG_TASKS
            ++refused;
#endif
            monitorOut();
            runnable->cancelRun();
            return false;
        }

#if LOG_TASKS
        ++accepted;
#endif
        // Even if we create a new thread the queue is still the way that's used
        // to give the task to the worker.
        queue.push(std::move(runnable));

        if (nbAvailable) {
            // NOTE: A worker is available
            for (auto it = threads.begin(); it != threads.end(); ++it) {
                if (it->second.waiting) {
                    signal(*it->second.cond);
                    break;
                }
            }
        } else if (threads.size() < maxThreadCount) {
            // NOTE: We can still create more threads
            size_t id = next_thread_id++;
            threads.emplace(
                id,
                worker_t{
                    .thread = std::make_shared<PcoThread>(&ThreadPool::worker, this, id),
                    .cond = std::make_shared<Condition>(),
                    .waiting = false,
                    .timeout = {},
                    .timed_out = false});
        }

        // NOTE: default action is just queuing since a worker will take the
        // job when available

        // NOTE: the reason why we fail the 4th test might be due to the fact
        // that workers that finished their task are in competition with whichever
        // thread tries to add a new task when it comes to securing the mutex

        monitorOut();
        return true;
    }

    /* Returns the number of currently running threads. They do not need to be executing a task,
     * just to be alive. (The watchdog isn't accounted for)
     */
    size_t currentNbThreads() { return threads.size(); }

private:
    typedef typename std::chrono::steady_clock Clock;
    typedef typename std::chrono::time_point<Clock> TimePoint;
    typedef typename ::size_t Key;
    typedef typename std::pair<PcoThread *, std::pair<TimePoint, Condition *>> TimeOutNode;

    // The maximum number of worker threads
    size_t maxThreadCount;
    // The max number of tasks that can be stored in the queue
    size_t maxNbWaiting;
    // The number of threads that are waiting for a task
    size_t nbAvailable;
    // The next thread id to use in the map.
    // NOTE: looking back, a circular buffer should have worked
    size_t next_thread_id = 0;
    // The time before a waiting worker should be timed out
    std::chrono::milliseconds idleTimeout;

#if LOG_IN_OUT
    // The number of times monitorIn was called
    std::atomic<size_t> in{0};
    // The number of times monitorOut was called
    std::atomic<size_t> out{0};
#endif

    /**
     * Contains everything that's necessary to know the status of a worker and
     * interact with it
     */
    struct worker_t
    {
        // A pointer to the thread
        std::shared_ptr<PcoThread> thread;
        // A pointer to the condition that the thread will wait on and should be
        // used to wake it up
        std::shared_ptr<Condition> cond;
        // technically the Condition knows if a thread is waiting but it's
        // private and we can't modify the class so we need to manage this info
        // here
        bool waiting;
        // The time at which point it should be considered as timed out if still
        // waiting
        TimePoint timeout;
        // used to distinguish between timeout and stop request
        bool timed_out;
    };

    /**
    * A map to store the threads, that way the workers can remove themselves from
    * the map before returning. We can't use a set since we need a unique_ptr to
    * automatically free the pointer and we can't use a unique_ptr as a key
    */
    std::map<Key, worker_t> threads;

    // The queue of tasks that cannot be executed straight away
    std::queue<std::unique_ptr<Runnable>> queue;

    std::unique_ptr<PcoThread> timer_thread;

#if LOG_TASKS
    std::size_t accepted = 0;
    std::size_t refused = 0;
    std::size_t executed = 0;
#endif

#if LOG_IN_OUT
    void monitorIn()
    {
        PcoHoareMonitor::monitorIn();
        ++in;
    }

    void monitorOut()
    {
        ++out;
        PcoHoareMonitor::monitorOut();
    }
#endif

    void worker(size_t id)
    {
        monitorIn();
        worker_t &wrkr = threads.at(id);
        monitorOut();

        while (true) {
            monitorIn();

#if LOG_WORK > 2
            PcoLogger() << "[worker" << id << "]" << "in" << std::endl;
#endif

#if LOG_WORK
            if (PcoThread::thisThread()->stopRequested()) {
                PcoLogger() << "[worker" << id << "]" << "stop requested before" << std::endl;
            }
#endif

            if (queue.empty() && !wrkr.timed_out && !PcoThread::thisThread()->stopRequested()) {
                wrkr.timeout = Clock::now() + idleTimeout;
                wrkr.waiting = true;
                ++nbAvailable;
                wait(*wrkr.cond);
                --nbAvailable;
                wrkr.waiting = false;

#if LOG_WORK
                if (PcoThread::thisThread()->stopRequested()) {
                    PcoLogger() << "[worker" << id << "]" << "stop in before" << std::endl;
                }

                if (wrkr.timed_out) {
                    PcoLogger() << "[worker" << id << "]" << "timed out" << std::endl;
                }
#endif
            }

            // NOTE: we still check if the queue is empty since we probably
            // shouldn't leave jobs that we promised to treat
            if ((PcoThread::thisThread()->stopRequested() && queue.empty()) || wrkr.timed_out) {
                break;
            }

            std::unique_ptr<Runnable> runnable = std::move(queue.front());
            queue.pop();

#if LOG_WORK > 2
            PcoLogger() << "[worker" << id << "]" << "out" << std::endl;
#endif
            monitorOut();

            runnable->run();
#if LOG_TASKS
            ++executed;
#endif
        }

        monitorOut();
    }

    void timer()
    {
        while (true) {
            monitorIn();

#if LOG_TIMER > 2
            PcoLogger() << "[timer]" << "in" << std::endl;
#endif
            if (PcoThread::thisThread()->stopRequested()) {
#if LOG_TIMER
                PcoLogger() << "[timer]" << "break" << std::endl;
#endif
                break;
            }

            std::queue<size_t> deleted{};

#if LOG_TIMER > 1
            PcoLogger() << "[timer]" << "iterating" << std::endl;
#endif
            for (auto it = threads.begin(); it != threads.end(); ++it) {
                if (it->second.waiting && it->second.timeout < Clock::now()) {
                    deleted.push(it->first);
                    it->second.timed_out = true;

#if LOG_TIMER
                    PcoLogger() << "[timer]" << "<signal" << std::endl;
#endif
                    signal(*it->second.cond);
#if LOG_TIMER
                    PcoLogger() << "[timer]" << "signal>" << std::endl;
#endif
                }
            }

#if LOG_TIMER > 1
            PcoLogger() << "[timer]" << "releasing" << std::endl;
#endif
            while (!deleted.empty()) {
                auto it = threads.find(deleted.front());
                // TODO: Check if there is issues with the  code below
                it->second.thread->join();
                deleted.pop();
                threads.erase(it);
            }

            // NOTE: finding the next timing to wakeup
            TimePoint until = Clock::now() + idleTimeout;
            for (auto it = threads.begin(); it != threads.end(); ++it) {
                if (!it->second.waiting || it->second.timed_out) {
                    continue;
                }

                if (until > it->second.timeout) {
                    until = it->second.timeout;
                }
            }

#if LOG_TIMER > 2
            PcoLogger() << "[timer]" << "out" << std::endl;
#endif
            monitorOut();

            std::this_thread::sleep_until(until);
        }

        monitorOut();
    }
};

#endif // THREADPOOL_H
