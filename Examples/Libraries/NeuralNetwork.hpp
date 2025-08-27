#ifndef NEURALNETWORK_HPP
#define NEURALNETWORK_HPP

#include <vector>

namespace NeuralNetwork
{
    struct Tensor
    {
        int dims;
        int *shape;
        int *strides;
        float *data;

        Tensor();
        Tensor(int dims_, const int *shape_);
        Tensor(const Tensor &original);
        Tensor &operator=(const Tensor &other);
        Tensor(Tensor &&other) noexcept;
        Tensor &operator=(Tensor &&other) noexcept;
        ~Tensor();

        float operator()(const int *coordinates) const;
        float &operator()(const int *coordinates);
        void set(const int *coordinates, float value);

        Tensor operator*(const Tensor &other);
        Tensor dot(const Tensor &other) const;
        Tensor operator%(const Tensor &other);
        Tensor operator+(const Tensor &other) const;
        Tensor operator-(const Tensor &other) const;

        int length() const;
        bool equalsSize(const Tensor &other) const;
        std::size_t toLinearIndex(const int *coordinates) const;
    };

    struct Layer
    {
        virtual Tensor forward(const Tensor &input) = 0;
        virtual Tensor backward(const Tensor &grad_output, float learning_rate) = 0;
        virtual ~Layer() = default;
    };

    struct Dense : public Layer
    {
        Tensor weights;
        Tensor bias;
        Tensor last_input;

        Dense(int input_size, int output_size);

        Tensor forward(const Tensor &input) override;
        Tensor backward(const Tensor &grad_output, float learning_rate) override;

    private:
        Tensor transpose(const Tensor &t);
    };

    struct ReLu : public Layer
    {
        Tensor last_input;
        Tensor forward(const Tensor &input) override;
        Tensor backward(const Tensor &grad_output, float) override;
    };

    struct Sigmoid : public Layer
    {
        Tensor last_output;
        Tensor forward(const Tensor &input) override;
        Tensor backward(const Tensor &grad_output, float /*lr*/) override;
    };

    struct LeakyReLU : public Layer
    {
        Tensor last_input;
        float alpha;
        LeakyReLU(float alpha_ = 0.01f);
        Tensor forward(const Tensor &input) override;
        Tensor backward(const Tensor &grad_output, float /*lr*/) override;
    };

    struct Sequential
    {
        std::vector<Layer *> layers;
        void add(Layer *layer);
        Tensor forward(const Tensor &input);
        void backward(const Tensor &grad_output, float lr);
        ~Sequential();
    };
}

#endif // NEURALNETWORK_HPP
