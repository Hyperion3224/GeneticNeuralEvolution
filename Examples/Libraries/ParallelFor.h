#include <future>
#include <algorithm>
#include <vector>
#include <functional>
#include <./ThreadPool.hpp>

inline void ParallelFor(ThreadPool &pool, int64_t begin, int64_t end,
                        std::function<void(int64_t, int64_t)> fn,
                        int desiredTasks = -1)
{
    const int64_t N = end - begin;
    if (N <= 0)
        return;

    int threads = std::max((size_t)1, pool.size());
    int numTasks = (desiredTasks > 0) ? desiredTasks : threads * 4;
    numTasks = std::max<int64_t>(1, std::min<int64_t>(numTasks, N));

    const int64_t minChunk = 8192; // tweak if needed
    const int maxTasksByGrain = int((N + minChunk - 1) / minChunk);
    if (maxTasksByGrain > 0)
        numTasks = std::min(numTasks, maxTasksByGrain);

    int64_t chunk = (N + numTasks - 1) / numTasks;

    std::vector<std::future<void>> futures;
    futures.reserve(numTasks);

    for (int t = 0; t < numTasks; ++t)
    {
        int64_t s = begin + t * chunk;
        int64_t e = std::min<int64_t>(s + chunk, end);
        if (s >= e)
            break;

        futures.push_back(pool.enqueue([=]
                                       { fn(s, e); }));
    }
    for (auto &future : futures)
        future.get();
}
