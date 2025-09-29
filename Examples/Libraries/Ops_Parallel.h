#pragma once
#include <cstdint>
#include <functional>
#include "./ThreadPool.hpp"
#include "./ParallelFor.h"

inline void ForEachRange(ThreadPool *pool, int64_t begin, int64_t end,
                         const std::function<void(int64_t, int64_t)> &fn)
{
    if (end <= begin)
        return;
    if (!pool)
    {
        fn(begin, end);
        return;
    }
    ParallelFor(*pool, begin, end, fn);
}

inline void unary_map(ThreadPool *pool, float *dest, const float *src, int64_t num,
                      const std::function<float(float)> &fn)
{
    ForEachRange(pool, 0, num, [&](int64_t start, int64_t end)
                 {
        for (int64_t i = start; i < end; ++i) dest[i] = fn(src[i]); });
}

inline void binary_map(ThreadPool *pool, float *dst, const float *a, const float *b, int64_t n,
                       const std::function<float(float, float)> &f)
{
    ForEachRange(pool, 0, n, [&](int64_t start, int64_t end)
                 {
        for (int64_t i = start; i < end; ++i) dst[i] = f(a[i], b[i]); });
}

inline void matmul_rows(ThreadPool *pool,
                        const float *A, int Ar, int Ac, int Astr0, int Astr1,
                        const float *B, int Bc, int Bstr0, int Bstr1,
                        float *C, int Cstr0, int Cstr1)
{
    ForEachRange(pool, 0, Ar, [&](int64_t s, int64_t e)
                 {
        for (int i = int(s); i < int(e); ++i) {
            const float* aRow = A + i * Astr0;
            for (int j = 0; j < Bc; ++j) {
                float sum = 0.f;
                for (int k = 0; k < Ac; ++k)
                    sum += aRow[k * Astr1] * B[k * Bstr0 + j * Bstr1];
                C[i * Cstr0 + j * Cstr1] = sum;
            }
        } });
}

inline void add_bias_broadcast(ThreadPool *pool,
                               float *Y, const float *b,
                               int B, int O, int Ystr0, int Ystr1)
{
    ForEachRange(pool, 0, B, [&](int64_t s, int64_t e)
                 {
        for (int i = int(s); i < int(e); ++i) {
            float* row = Y + i * Ystr0;
            for (int j = 0; j < O; ++j) row[j * Ystr1] += b[j];
        } });
}

inline void reduce_sum_rows(ThreadPool *pool,
                            const float *X, int B, int O, int Xstr0, int Xstr1,
                            float *out /*zeroed [O] */)
{
    if (!pool)
    {
        for (int i = 0; i < B; ++i)
        {
            const float *row = X + i * Xstr0;
            for (int j = 0; j < O; ++j)
                out[j] += row[j * Xstr1];
        }
        return;
    }
    const int tasks = std::max(1, int(pool->size()) * 4);
    std::vector<std::vector<float>> partial(tasks, std::vector<float>(O, 0.f));
    ForEachRange(pool, 0, tasks, [&](int64_t s, int64_t e)
                 {
        for (int t = int(s); t < int(e); ++t) {
            int i0 = int((int64_t(B) *  t    ) / tasks);
            int i1 = int((int64_t(B) * (t+1)) / tasks);
            auto& acc = partial[t];
            for (int i = i0; i < i1; ++i) {
                const float* row = X + i * Xstr0;
                for (int j = 0; j < O; ++j) acc[j] += row[j * Xstr1];
            }
        } });
    for (int t = 0; t < tasks; ++t)
        for (int j = 0; j < O; ++j)
            out[j] += partial[t][j];
}
