#pragma once

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <algorithm>
#include <utility>
#include <functional>
#include <stdexcept>


//class ThreadPool;



class ThreadPool {
public:
    typedef std::vector<std::thread>::size_type size_type;

    ThreadPool()  : stop(false) {}
    virtual ~ThreadPool();

    void Init(size_type);

    template<class F>
    auto enqueue(F&& f, int priority = 0) -> std::future<decltype(std::forward<F>(f)())>;

private:
    class Worker {
    public:
        Worker(ThreadPool& s) : pool(s) { }

        void operator()() {
            while(true) {
                std::unique_lock<std::mutex> lock(pool.queue_mutex);

                while(!pool.stop && pool.tasks.empty())
                    pool.condition.wait(lock);

                if(pool.stop && pool.tasks.empty())
                    return;

                std::function<void()> task(pool.tasks.top().second);
                pool.tasks.pop();

                lock.unlock();

                task();
            }
        }

    private:
        ThreadPool& pool;
    };

    // need to keep track of threads so we can join them
    std::vector<std::thread> workers;

    typedef std::pair<int, std::function<void()>> priority_task;

    // emulate 'nice'
    struct task_comp {
        bool operator()(const priority_task& lhs, const priority_task& rhs) const {
            return lhs.first > rhs.first;
        }
    };

    // the prioritized task queue
    std::priority_queue<priority_task, std::vector<priority_task>, task_comp> tasks;

    // synchronization
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
};

// the constructor just launches some amount of workers
void ThreadPool::Init(ThreadPool::size_type threads) {
    workers.reserve(threads);

    for(ThreadPool::size_type i = 0; i < threads; ++i)
        workers.emplace_back(Worker(*this));
}

// add new work item to the pool
template<class F>
auto ThreadPool::enqueue(F&& f, int priority) -> std::future<decltype(std::forward<F>(f)())> {
    typedef decltype(std::forward<F>(f)()) R;

    if(stop)
        throw std::runtime_error("enqueue on stopped threadpool");

    auto task = std::make_shared<std::packaged_task<R()>>(std::forward<F>(f));
    std::future<R> res = task->get_future();

    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        tasks.emplace(priority, [task]{ (*task)(); });
    }

    condition.notify_one();

    return res;
}

// the destructor joins all threads
ThreadPool::~ThreadPool() {
    stop = true;
    condition.notify_all();
    Print(1, "workers.size() %d\n", workers.size());
    for(ThreadPool::size_type i = 0; i < workers.size(); ++i) {
        Print(1, "Gone here ffff\n");
        workers[i].join();
    }
}
