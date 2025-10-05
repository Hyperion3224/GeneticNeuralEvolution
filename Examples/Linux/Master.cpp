#include "./network/net/MasterServer.hpp"
#include "./network/net/Logger.hpp"
#include "./network/net/Protocol.hpp"
#include "../Libraries/Third_Party/json.hpp"
#include <unordered_map>
#include <vector>
#include <iomanip>
#include <sstream>

#include "../Libraries/ModelPartitioner.hpp"

using json = nlohmann::json;
using MasterServer = dist::MasterServer;
using Connection = dist::Connection;

struct PendingNode
{
    dist::Connection conn;
    std::string addr; // "ip:port"
    std::uint64_t ram_mb{0};
    unsigned threads{1};
};

static std::vector<PendingNode> g_nodes;
static const int EXPECTED_NODES = 8;

// Call when a new connection is accepted
void MasterServer::on_client_connected(Connection c)
{
    // Store connection, wait for its RESOURCE_REPORT
    PendingNode p;
    p.addr = c.peer_addr(); // adapt if you have different API
    p.conn = std::move(c);
    g_nodes.emplace_back(std::move(p));
    LOG_INFO("Accepted node connection from {}", g_nodes.back().addr);
}

// Call when a message arrives from a node
void MasterServer::on_message(Connection &c, const std::string &payload)
{
    // Expect JSON
    json j = json::parse(payload, nullptr, true);
    if (!j.contains("type"))
    {
        LOG_WARN("Unknown message (no type) from {}", c.peer_addr());
        return;
    }
    const std::string type = j["type"].get<std::string>();

    if (type == "RESOURCE_REPORT")
    {
        // Update that node's specs
        for (auto &n : g_nodes)
        {
            if (n.conn.id() == c.id())
            { // or compare by address
                n.ram_mb = j.value("ram_mb_free", 0ull);
                n.threads = j.value("hardware_threads", 1u);
                LOG_INFO("Node {} reported RAM={}MB threads={}", n.addr, n.ram_mb, n.threads);
                break;
            }
        }

        // If we have all 8 reports, print table and send configs
        int have = 0;
        for (auto &n : g_nodes)
            if (n.ram_mb > 0)
                ++have;
        if (have >= EXPECTED_NODES)
        {
            print_resource_table();
            compute_and_send_configs(/*total_layers=*/get_total_layers_from_model(),
                                     /*bytes_per_layer=*/get_bytes_per_layer());
        }
    }
    else
    {
        LOG_WARN("Unhandled message type '{}' from {}", type, c.peer_addr());
    }
}

// Helper: pretty resource table
void MasterServer::print_resource_table()
{
    LOG_INFO("All {} nodes reported. Resource summary:", EXPECTED_NODES);
    std::ostringstream oss;
    oss << "\n+----+---------------------+------------+---------+\n";
    oss << "| id | addr                | freeRAM(M) | thrds   |\n";
    oss << "+----+---------------------+------------+---------+\n";
    for (size_t i = 0; i < g_nodes.size(); ++i)
    {
        const auto &n = g_nodes[i];
        oss << "| " << std::setw(2) << i
            << " | " << std::setw(19) << n.addr
            << " | " << std::setw(10) << n.ram_mb
            << " | " << std::setw(7) << n.threads
            << " |\n";
    }
    oss << "+----+---------------------+------------+---------+\n";
    LOG_INFO("{}", oss.str());
}

// Placeholder adapters to your NN code
int MasterServer::get_total_layers_from_model() const
{
    // e.g., if NeuralNetwork exposes layer count
    // return neuralNet_.layerCount();
    return 120; // <- adjust or query NeuralNetwork.cpp
}
std::uint64_t MasterServer::get_bytes_per_layer() const
{
    // Heuristic / or query tensors for per-layer footprint
    return 1ull * 1024ull * 1024ull; // 1MB per layer as a simple default
}

void MasterServer::compute_and_send_configs(int total_layers, std::uint64_t bytes_per_layer)
{
    std::vector<NodeCompute> nodes;
    nodes.reserve(g_nodes.size());
    for (auto &n : g_nodes)
    {
        nodes.push_back(NodeCompute{n.addr, n.ram_mb, n.threads});
    }

    auto assigns = ModelPartitioner::partition_by_capacity(nodes, total_layers, bytes_per_layer);

    // Send each node its CONFIG as JSON
    for (size_t i = 0; i < g_nodes.size(); ++i)
    {
        auto jcfg = ModelPartitioner::to_config_json(assigns[i], static_cast<int>(i));
        Protocol::send_json(g_nodes[i].conn, jcfg); // <-- ADAPT IF NEEDED
        LOG_INFO("Sent CONFIG to node {}: layers={}, first={}, last={}",
                 g_nodes[i].addr, assigns[i].layers.size(), assigns[i].is_first, assigns[i].is_last);
    }
}
