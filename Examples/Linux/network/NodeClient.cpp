#include "NodeClient.hpp"
#include <stdexcept>
#include <sstream>

NodeClient::NodeClient(std::string masterHost, uint16_t masterPort)
    : masterHost_(std::move(masterHost)), masterPort_(masterPort) {}

bool NodeClient::connect()
{
    try
    {
        return conn_.connect(masterHost_.c_str(), masterPort_);
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("NodeClient connect failed: {}", e.what());
        return false;
    }
}

NodeSpecs NodeClient::gatherSpecs()
{
    NodeSpecs s;
    s.ip = sys_info::local_ip();
    s.ram_mb_free = sys_info::free_ram_mb();
    s.hardware_threads = sys_info::hardware_threads();
    return s;
}

json NodeClient::specsToJson(const NodeSpecs &s)
{
    json j;
    j["type"] = "RESOURCE_REPORT";
    j["ip"] = s.ip;
    j["ram_mb_free"] = s.ram_mb_free;
    j["hardware_threads"] = s.hardware_threads;
    return j;
}

NodeConfig NodeClient::configFromJson(const json &j)
{
    NodeConfig cfg;
    cfg.node_index = j.value("node_index", -1);
    cfg.is_first = j.value("is_first", false);
    cfg.is_last = j.value("is_last", false);
    cfg.array_size = j.value("array_size", 0ull);
    cfg.next_node_addr = j.value("next_node_addr", "");
    if (j.contains("layers") && j["layers"].is_array())
    {
        for (auto &x : j["layers"])
            cfg.layers.push_back(x.get<int>());
    }
    return cfg;
}

bool NodeClient::sendSpecsAndAwaitConfig(NodeConfig &outConfig)
{
    // 1) Send specs JSON
    auto specs = gatherSpecs();
    json j = specsToJson(specs);

    // Use your Protocol framing; fall back to simple length-prefixed if needed.
    // Here we assume Protocol::send_json / recv_json exist; otherwise replace with your own.
    try
    {
        Protocol::send_json(conn_, j);           // <-- ADAPT IF NEEDED
        json reply = Protocol::recv_json(conn_); // <-- ADAPT IF NEEDED
        if (!reply.contains("type") || reply["type"] != "CONFIG")
        {
            LOG_ERROR("Unexpected config reply: {}", reply.dump());
            return false;
        }
        outConfig = configFromJson(reply);
        LOG_INFO("Received config: node={} layers={} first={} last={}",
                 outConfig.node_index, outConfig.layers.size(), outConfig.is_first, outConfig.is_last);
        return true;
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("Handshake failed: {}", e.what());
        return false;
    }
}
