#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "./Third_Party/json.hpp"

struct NodeCompute
{
    std::string addr;     // "ip:port"
    std::uint64_t ram_mb; // free RAM
    unsigned threads;     // hardware threads
};

struct NodeAssignment
{
    int node_index = -1;
    std::vector<int> layers; // layer indices
    std::uint64_t array_bytes = 0;
    std::string next_addr; // empty if last
    bool is_first = false;
    bool is_last = false;
};

class ModelPartitioner
{
public:
    // total_layers: e.g. 120
    // safety_mem_per_thread_mb: reserve headroom per thread so we don't overcommit
    static std::vector<NodeAssignment>
    partition_by_capacity(const std::vector<NodeCompute> &nodes,
                          int total_layers,
                          std::uint64_t bytes_per_layer,
                          std::uint64_t safety_mem_per_thread_mb = 128);

    // Serialize one node's assignment to a JSON config to send over the wire
    static nlohmann::json to_config_json(const NodeAssignment &a, int node_index);

    // Convenience: build a chain of addresses in node order
    static std::vector<std::string> chain_addrs(const std::vector<NodeCompute> &nodes);
};
