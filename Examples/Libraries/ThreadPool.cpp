#pragma once
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <future>

class ThreadPool
{
public:
    explicit ThreadPool(size_t numThreads)
    {
        start(numThreads);
    }

    ~ThreadPool()
    {
        stop();
    }

    size_t size()
    {
        return mThreads.size();
    }

    void enqueue(std::function<void()> task)
    {
        {
            std::unique_lock<std::mutex> lock(mEventMutex);
            mTasks.push(std::move(task));
        }
        mEventVar.notify_one();
    }

    template <class F, class... Args>
    auto enqueue(F &&f, Args &&...args) -> std::future<std::invoke_result_t<F, Args...>>
    {
        using RetType = std::invoke_result_t<F, Args...>;

        auto task = std::make_shared<std::packaged_task<RetType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));

        std::future<RetType> future = task->get_future();
        {
            std::unique_lock<std::mutex> lock(mEventMutex);
            mTasks.emplace([task]()
                           { (*task)(); });
        }
        mEventVar.notify_one();
        return future;
    }

private:
    std::vector<std::thread> mThreads;
    std::condition_variable mEventVar;
    std::mutex mEventMutex;
    bool mStopping = false;

    std::queue<std::function<void()>> mTasks;

    void start(size_t numThreads)
    {
        for (size_t i = 0; i < numThreads; ++i)
        {
            mThreads.emplace_back([=]
                                  {
                while (true) {
                    std::function<void()> task;

                    {
                        std::unique_lock<std::mutex> lock(mEventMutex);
                        mEventVar.wait(lock, [=] { return mStopping || !mTasks.empty(); });

                        if (mStopping && mTasks.empty())
                            break;

                        task = std::move(mTasks.front());
                        mTasks.pop();
                    }

                    task(); 
                } });
        }
    }

    void stop() noexcept
    {
        {
            std::unique_lock<std::mutex> lock(mEventMutex);
            mStopping = true;
        }

        mEventVar.notify_all();

        for (std::thread &thread : mThreads)
            thread.join();
    }
};
