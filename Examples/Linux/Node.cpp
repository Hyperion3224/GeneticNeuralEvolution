#include <iostream>
#include "./network/NodeClient.hpp"
#include "./network/net/Logger.hpp"

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        std::cerr << "usage: node <master_host> <master_port>\n";
        return 1;
    }
    const std::string host = argv[1];
    const uint16_t port = static_cast<uint16_t>(std::stoi(argv[2]));

    NodeClient client(host, port);
    if (!client.connect())
        return 2;

    NodeConfig cfg;
    if (!client.sendSpecsAndAwaitConfig(cfg))
        return 3;

    // Demo: allocate arrays according to config (replace with your NN allocs)
    // ... e.g., std::vector<float> activations(cfg.array_size / sizeof(float));

    // Demo output:
    std::cout << "Node index: " << cfg.node_index
              << " first=" << cfg.is_first
              << " last=" << cfg.is_last
              << " next=" << cfg.next_node_addr
              << " layers=[";
    for (size_t i = 0; i < cfg.layers.size(); ++i)
    {
        std::cout << cfg.layers[i] << (i + 1 < cfg.layers.size() ? "," : "");
    }
    std::cout << "] array_bytes=" << cfg.array_size << "\n";

    // ... proceed to initialize local NeuralNetwork slice based on layers ...
    return 0;
}
