#include <cstdlib>
#include <stdexcept>

#include <iostream>
#include <cstring>

namespace NeuralNetwork
{
    struct Tensor
    {
        int dims;
        int *shape;
        int *strides;
        float *data;

        Tensor(int dims_, const int *shape_)
            : dims(dims_)
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

        Tensor(const Tensor &original)
            : dims(original.dims)
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

            shape = new int[dims];
            strides = new int[dims];
            data = new float[other.length()];

            std::memcpy(shape, other.shape, dims * sizeof(int));
            std::memcpy(strides, other.strides, dims * sizeof(int));
            std::memcpy(data, other.data, other.length() * sizeof(float));

            return *this;
        }

        Tensor(Tensor &&other) noexcept
            : dims(other.dims), shape(other.shape), strides(other.strides), data(other.data)
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

        Tensor operator*(const Tensor &other)
        {
            if (dims != other.dims)
                throw std::invalid_argument("Dimension mismatch");
            for (int i = 0; i < dims; i++)
                if (shape[i] != other.shape[i])
                    throw std::invalid_argument("Shape mismatch");

            Tensor result(dims, shape);
            int len = length();
            for (int i = 0; i < len; i++)
                result.data[i] = data[i] * other.data[i];

            return result;
        }

        Tensor operator^(const Tensor &other)
        {
            // implament dot product
        }

        Tensor operator+(const Tensor &other) const
        {
            if (dims != other.dims)
                throw std::out_of_range("Dimension mismatch");
            for (int i = 0; i < dims; i++)
                if (shape[i] != other.shape[i])
                    throw std::out_of_range("Shape mismatch");

            Tensor res(dims, shape);
            int len = length();
            for (int i = 0; i < len; i++)
                res.data[i] = data[i] + other.data[i];

            return res;
        }

        int length() const
        {
            int length = 1;
            for (int i = 0; i < dims; i++)
                length *= shape[i];
            return length;
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
    };
}

namespace Distributed
{

}

namespace Serialization
{

}

int main()
{
    int test = 2;
    int shape[] = {1, 2};
    NeuralNetwork::Tensor t(test, shape);

    int coords[] = {0, 0};
    t.set(coords, 2);
    coords[1] = 1;
    t.set(coords, 1);

    std::cout << std::endl
              << t(coords);
    coords[1]--;
    std::cout << std::endl
              << t(coords);

    std::cout << std::endl
              << (t * t)(coords);

    return 0;
}