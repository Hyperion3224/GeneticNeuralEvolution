#include <cstdlib>
#include <stdexcept>
#include <optional>
#include <vector>
#include <cmath>
#include <iostream>
#include <cstring>
#include <algorithm>
#include <random>

#include "./ThreadPool.hpp"
#include "./ParallelFor.h"
#include "./Ops_Parallel.h"

namespace NeuralNetwork
{
    struct Tensor
    {
        int dims;
        int *shape;
        int *strides;
        float *data;

        ThreadPool *pool = nullptr;

        Tensor()
            : dims(0), pool(nullptr), shape(nullptr), strides(nullptr), data(nullptr) {}

        Tensor(int dims_, ThreadPool *pool_, const int *shape_)
            : dims(dims_), pool(pool_)
        {
            shape = new int[dims];
            for (int i = 0; i < dims; i++)
                shape[i] = shape_[i];

            data = new float[length()]();

            strides = new int[dims];
            int lastStridesIndex = dims - 1;
            strides[lastStridesIndex] = 1;
            for (int i = lastStridesIndex - 1; i >= 0; i--)
            {
                strides[i] = strides[i + 1] * shape[i + 1];
            }
        }

        Tensor(int dims_, ThreadPool *pool_, std::initializer_list<int> shape_)
            : dims(dims_), pool(pool_)
        {
            if ((int)shape_.size() != dims)
                throw std::invalid_argument("dims != shape_.size()");
            shape = new int[dims];

            int i = 0;
            for (int s : shape_)
                shape[i++] = s;

            data = new float[length()]();

            strides = new int[dims];
            int lastStridesIndex = dims - 1;
            strides[lastStridesIndex] = 1;
            for (int i = lastStridesIndex - 1; i >= 0; i--)
            {
                strides[i] = strides[i + 1] * shape[i + 1];
            }
        }

        Tensor(const Tensor &original)
            : dims(original.dims), pool(original.pool)
        {
            shape = new int[dims];
            strides = new int[dims];
            data = new float[original.length()];

            std::memcpy(shape, original.shape, dims * sizeof(int));
            std::memcpy(strides, original.strides, dims * sizeof(int));
            std::memcpy(data, original.data, original.length() * sizeof(float));
        }

        Tensor &operator=(const Tensor &other)
        {
            if (this == &other)
                return *this;

            delete[] data;
            delete[] shape;
            delete[] strides;

            dims = other.dims;
            pool = other.pool;

            shape = new int[dims];
            strides = new int[dims];
            data = new float[other.length()];

            std::memcpy(shape, other.shape, dims * sizeof(int));
            std::memcpy(strides, other.strides, dims * sizeof(int));
            std::memcpy(data, other.data, other.length() * sizeof(float));

            return *this;
        }

        Tensor(Tensor &&other) noexcept
            : dims(other.dims), pool(other.pool), shape(other.shape), strides(other.strides), data(other.data)
        {
            other.shape = nullptr;
            other.strides = nullptr;
            other.data = nullptr;
            other.dims = 0;
        }

        Tensor &operator=(Tensor &&other) noexcept
        {
            if (this == &other)
                return *this;

            delete[] data;
            delete[] shape;
            delete[] strides;

            dims = other.dims;
            pool = other.pool;
            shape = other.shape;
            strides = other.strides;
            data = other.data;

            other.shape = nullptr;
            other.strides = nullptr;
            other.data = nullptr;
            other.dims = 0;

            return *this;
        }

        ~Tensor()
        {
            delete[] data;
            delete[] shape;
            delete[] strides;
        }

        float operator()(const int *coordinates) const
        {
            return data[toLinearIndex(coordinates)];
        }

        float &operator()(const int *coordinates)
        {
            return data[toLinearIndex(coordinates)];
        }

        void set(const int *coordinates, float value)
        {
            data[toLinearIndex(coordinates)] = value;
        }

        Tensor operator*(const Tensor &other) const
        {
            equalsSize(other);
            Tensor result(dims, pool, shape);
            int len = length();

            binary_map(pool, result.data, data, other.data, len, [](float a, float b)
                       { return a * b; });

            return result;
        }

        Tensor dot(const Tensor &other) const
        {
            if (dims == 1 && other.dims == 1)
            {
                if (length() != other.length())
                    throw std::out_of_range("Vector length mismatch");
                Tensor res(1, pool, {1}); // scalar
                res.pool = pool ? pool : other.pool;
                float sum = 0.f;

                if (res.pool && res.pool->size() > 1)
                {
                    const int n = length();
                    const int tasks = std::max<int>(1, int(res.pool->size()) * 4);
                    std::vector<float> partial(tasks, 0.f);
                    ForEachRange(res.pool, 0, tasks, [&](int64_t s, int64_t e)
                                 {
                for (int t=int(s); t<int(e); ++t) {
                    int i0 = int((int64_t(n) *  t    ) / tasks);
                    int i1 = int((int64_t(n) * (t+1)) / tasks);
                    float acc = 0.f;
                    for (int i=i0; i<i1; ++i) acc += data[i] * other.data[i];
                    partial[t] += acc;
                } });
                    for (float v : partial)
                        sum += v;
                }
                else
                {
                    for (int i = 0; i < length(); ++i)
                        sum += data[i] * other.data[i];
                }
                res.data[0] = sum;
                return res;
            }
            else if (dims == 2 && other.dims == 2)
            {
                if (shape[1] != other.shape[0])
                    throw std::out_of_range("Matrix shapes are incompatible");
                int thisRows = shape[0], thisCol = shape[1], otherCols = other.shape[1];
                int newShape[2] = {thisRows, otherCols};
                Tensor res(2, pool, newShape);

                // Use parallel row kernel
                matmul_rows(res.pool,
                            /*A*/ data, thisRows, thisCol, strides[0], strides[1],
                            /*B*/ other.data, otherCols, other.strides[0], other.strides[1],
                            /*C*/ res.data, res.strides[0], res.strides[1]);
                return res;
            }
            else
            {
                throw std::out_of_range("Dot product not implemented for these dimensions");
            }
        }

        // dot product
        Tensor operator%(const Tensor &other) const
        {
            return dot(other);
        }

        Tensor operator+(const Tensor &other) const
        {
            equalsSize(other);
            Tensor res(dims, pool, shape);
            int len = length();
            binary_map(pool, res.data, data, other.data, len, [](float a, float b)
                       { return a + b; });

            return res;
        }

        Tensor operator-(const Tensor &other) const
        {
            equalsSize(other);
            Tensor res(dims, pool, shape);
            int len = length();
            binary_map(pool, res.data, data, other.data, len, [](float a, float b)
                       { return a - b; });

            return res;
        }

        uint64_t length() const
        {
            uint64_t length = 1;
            for (int i = 0; i < dims; i++)
                length *= shape[i];
            return length;
        }

        bool equalsSize(const Tensor &other) const
        {
            if (dims != other.dims)
            {
                throw std::out_of_range("Dimension mismatch");
            }
            for (int i = 0; i < dims; i++)
                if (shape[i] != other.shape[i])
                {
                    throw std::out_of_range("Shape mismatch");
                }
            return true;
        }

        std::size_t toLinearIndex(const int *coordinates) const
        {
            std::size_t index = 0;
            for (int i = dims - 1; i >= 0; i--)
            {
                if (coordinates[i] >= 0 && coordinates[i] < shape[i])
                {
                    index += coordinates[i] * strides[i];
                }
                else
                {
                    throw std::out_of_range("Coordinate is out of range");
                }
            }
            return index;
        }

        void setPool(ThreadPool *_pool)
        {
            pool = _pool;
        }
    };

    struct Layer
    {
        virtual Tensor forward(const Tensor &input) = 0;
        virtual Tensor backward(const Tensor &grad_output, float learning_rate) = 0;
        virtual ~Layer() = default;

        virtual void SetPool(ThreadPool *p) { pool = p; }

    protected:
        ThreadPool *pool = nullptr;
    };

    struct Dense : public Layer
    {
        Tensor weights;
        Tensor bias;
        Tensor last_input;

        Dense(int input_size, int output_size)
            : weights(2, pool, {input_size, output_size}), bias(1, pool, {output_size})
        {
            std::mt19937 rng(123);
            std::uniform_real_distribution<float> dist(-0.05f, 0.05f);
            unary_map(pool, weights.data, weights.data, weights.length(),
                      [&](float)
                      { return dist(rng); });
            unary_map(pool, bias.data, bias.data, bias.length(), [](float a)
                      { return 0.f; });
        }

        void SetPool(ThreadPool *pool_) override
        {
            pool = pool_;
            weights.setPool(pool_);
            bias.setPool(pool_);
            last_input.setPool(pool_);
        }

        Tensor forward(const Tensor &input) override
        {
            last_input = input; // store for backward
            Tensor output = input.dot(weights);
            if (output.dims == 2 && bias.dims == 1)
            {
                add_bias_broadcast(pool,
                                   /*Y*/ output.data, /*b*/ bias.data,
                                   /*B*/ output.shape[0], /*O*/ output.shape[1],
                                   /*Ystr0*/ output.strides[0], /*Ystr1*/ output.strides[1]);
                return output;
            }

            return output + bias;
        }

        Tensor backward(const Tensor &grad_output, float lr) override
        {
            // dX
            Tensor grad_input = grad_output.dot(transpose(weights));

            // dW = X^T · dY
            Tensor Xt = transpose(last_input);
            Tensor grad_weights = Xt.dot(grad_output);

            // db = sum over rows
            Tensor grad_bias(1, pool, bias.shape);
            std::fill(grad_bias.data, grad_bias.data + grad_bias.length(), 0.f);
            if (grad_output.dims == 2)
            {
                reduce_sum_rows(pool, grad_output.data,
                                /*B*/ grad_output.shape[0], /*O*/ grad_output.shape[1],
                                /*Xstr0*/ grad_output.strides[0], /*Xstr1*/ grad_output.strides[1],
                                /*out*/ grad_bias.data);
            }
            else
            {
                // no batch
                std::memcpy(grad_bias.data, grad_output.data, sizeof(float) * grad_bias.length());
            }

            // SGD update
            binary_map(pool, weights.data, weights.data, grad_weights.data, weights.length(),
                       [=](float w, float gw)
                       { return w - lr * gw; });
            binary_map(pool, bias.data, bias.data, grad_bias.data, bias.length(),
                       [=](float b, float gb)
                       { return b - lr * gb; });

            return grad_input;
        }

    private:
        Tensor transpose(const Tensor &t)
        {
            if (t.dims != 2)
                throw std::runtime_error("Only 2D transpose supported");
            int newShape[2] = {t.shape[1], t.shape[0]};
            Tensor result(2, pool, newShape);
            for (int i = 0; i < t.shape[0]; i++)
                for (int j = 0; j < t.shape[1]; j++)
                    result.data[j * result.strides[0] + i * result.strides[1]] =
                        t.data[i * t.strides[0] + j * t.strides[1]];
            return result;
        }
    };

    struct ReLu : public Layer
    {
        Tensor last_input;

        void SetPool(ThreadPool *pool_) override
        {
            pool = pool_;
            last_input.setPool(pool_);
        }

        Tensor forward(const Tensor &input) override
        {
            last_input = input;
            Tensor output = Tensor(input.dims, pool, input.shape);
            unary_map(pool, output.data, last_input.data, last_input.length(), [](float a)
                      { return std::max(0.0f, a); });
            return output;
        }

        Tensor backward(const Tensor &grad_output, float) override
        {
            Tensor grad_input = grad_output;
            binary_map(pool, grad_input.data, last_input.data, grad_output.data, grad_output.length(), [](float a, float b)
                       { return a > 0.f ? b : 0.f; });

            return grad_input;
        }
    };

    struct Sigmoid : public Layer
    {
        Tensor last_output; // store for backward (since σ'(x) = σ(x)(1-σ(x)))

        void SetPool(ThreadPool *pool_) override
        {
            pool = pool_;
            last_output.setPool(pool_);
        }

        Tensor forward(const Tensor &input) override
        {
            last_output = Tensor(input.dims, pool, input.shape);
            unary_map(pool, last_output.data, input.data, input.length(), [](float a)
                      {
                if (a >= 0.f) { float z = std::exp(-a); return 1.f / (1.f + z); }
                else          { float z = std::exp(a);  return z / (1.f + z); } });
            return last_output;
        }

        Tensor backward(const Tensor &grad_output, float /*lr*/) override
        {
            Tensor grad_input = grad_output; // same shape

            binary_map(pool, grad_input.data, grad_input.data, last_output.data, grad_output.length(), [](float a, float b)
                       { return a * (b * (1.f - b)); });
            return grad_input;
        }
    };

    struct LeakyReLU : public Layer
    {
        Tensor last_input;
        float alpha;

        void SetPool(ThreadPool *pool_) override
        {
            pool = pool_;
            last_input.setPool(pool_);
        }

        LeakyReLU(float alpha_ = 0.01f) : alpha(alpha_) {}

        Tensor forward(const Tensor &input) override
        {
            last_input = input;
            Tensor output = Tensor(input.dims, pool, input.shape);

            unary_map(pool, output.data, input.data, input.length(), [alpha = this->alpha](float a)
                      { return (a > 0) ? a : alpha * a; });

            return output;
        }

        Tensor backward(const Tensor &grad_output, float /*lr*/) override
        {
            Tensor grad_input = grad_output;

            binary_map(pool, grad_input.data, grad_input.data, last_input.data, grad_input.length(), [alpha = this->alpha](float a, float b)
                       { return a * ((b > 0) ? 1.f : alpha); });
            return grad_input;
        }
    };

    struct Sequential
    {
        std::vector<Layer *> layers;
        ThreadPool &pool;

        Sequential(ThreadPool &p) : pool(p) {}

        void add(Layer *layer)
        {
            layer->SetPool(&pool);
            layers.push_back(layer);
        }

        Tensor forward(const Tensor &input)
        {
            Tensor x = input;
            for (auto layer : layers)
                x = layer->forward(x);
            return x;
        }

        void backward(const Tensor &grad_output, float lr)
        {
            Tensor grad = grad_output;
            if (layers.size() == 0)
            {
                return;
            }
            for (int i = layers.size() - 1; i >= 0; i--)
                grad = layers[i]->backward(grad, lr);
        }

        ~Sequential()
        {
            for (auto l : layers)
                delete l;
        }
    };
}