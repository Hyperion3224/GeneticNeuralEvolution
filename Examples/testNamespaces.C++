#include "../Include/Namespaces.h"
#include <iostream>

int main()
{
    int test = 2;
    int shape[] = {1, 2};
    NeuralNetwork::Tensor t(test, shape);

    int coords[] = {0, 0};
    t.set(coords, 1);
    coords[1] = 1;
    t.set(coords, 2);

    std::cout << t.get(coords);
    coords[1]--;
    std::cout << t.get(coords);
}