#pragma once
#include <string>
#include <cstdint>
#include <vector>
#include <optional>

#include "./net/Connection.hpp" // your existing transport
#include "./net/Protocol.hpp"   // your existing framing/tags
#include "./net/Logger.hpp"

#include "../../Libraries/System_Info.hpp"

#include "../../Libraries/Third_Party/json.hpp"
using json = nlohmann::json;

struct NodeSpecs
{
    std::string ip;
    std::uint64_t ram_mb_free{0};
    unsigned hardware_threads{1};
};

struct NodeConfig
{
    int node_index = -1; // 0..N-1
    bool is_first = false;
    bool is_last = false;
    std::vector<int> layers;      // layer indices assigned to this node
    std::uint64_t array_size = 0; // bytes to allocate for temp buffers
    std::string next_node_addr;   // "ip:port", empty if last
};

class NodeClient
{
public:
    NodeClient(std::string masterHost, uint16_t masterPort);
    bool connect();
    bool sendSpecsAndAwaitConfig(NodeConfig &outConfig);

private:
    std::string masterHost_;
    uint16_t masterPort_;
    dist::Connection conn_; // assumes default-constructible; adapt if needed

    static NodeSpecs gatherSpecs();
    static json specsToJson(const NodeSpecs &s);
    static NodeConfig configFromJson(const json &j);
};
