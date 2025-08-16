#include <cstdlib>
#include <stdexcept>

namespace NeuralNetwork
{
    struct Tensor
    {
        const int dims;
        int *shape;
        float *data;

        Tensor(int dims_, const int *shape_)
            : dims(dims_)
        {
            shape = new int[dims];
            for (int i = 0; i < dims; i++)
                shape[i] = shape_[i];

            data = new float[length()];
        }

        ~Tensor()
        {
            delete[] data;
            delete[] shape;
        }

        float operator()(const int *coordinates) const { return get(coordinates); }

        float &operator()(const int *coordinates)
        {
            int index = 0, stride = 1;
            for (int i = dims - 1; i >= 0; i--)
            {
                index += coordinates[i] * stride;
                stride *= shape[i];
            }
            return data[index];
        }

        int length() const
        {
            int length = 1;
            for (int i = 0; i < dims; i++)
                length *= shape[i];
            return length;
        }

        float get(const int *coordinates) const
        {
            int index = 0, stride = 1;
            for (int i = dims - 1; i >= 0; i--)
            {
                if (coordinates[i] > 0 || coordinates[i] <= shape[i])
                {
                    index += coordinates[i] * stride;
                    stride *= shape[i];
                }
                else
                {
                    throw std::out_of_range("Coordinate is out of range");
                }
            }
            return data[index];
        }

        void set(const int *coordinates, float value)
        {
            int index = 0;
            int stride = 1;
            for (int i = dims - 1; i >= 0; i--)
            {
                index += coordinates[i] * stride;
                stride *= shape[i];
            }

            data[index] = value;
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

        // Tensor operator@(const Tensor &other)
        // {
        // }

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
    return 0;
}