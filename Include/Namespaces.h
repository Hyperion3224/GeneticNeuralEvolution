#pragma once
#include <stdexcept>

namespace NeuralNetwork
{
    struct Tensor
    {
        const int dims;
        int *shape;
        float *data;

        Tensor(int dims_, const int *shape_);
        ~Tensor();

        float operator()(const int *coordinates) const;
        float &operator()(const int *coordinates);

        int length() const;

        float get(const int *coordinates) const;
        void set(const int *coordinates, float value);

        Tensor operator*(const Tensor &other);
        Tensor operator+(const Tensor &other) const;
    };
}

namespace Distributed
{
    // Add declarations here as needed
}

namespace Serialization
{
    // Add declarations here as needed
}