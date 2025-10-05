#pragma once
#include <unordered_map>
#include <mutex>
#include <vector>
#include <optional>
#include "Connection.hpp"

namespace dist
{

    class ConnectionRegistry
    {
    public:
        using NodeId = socket_t; // key by socket fd/handle; replace with stable ID if you add auth.

        bool insert(NodeId id, const NodeInfo &info)
        {
            std::lock_guard<std::mutex> lk(mu_);
            if (nodes_.size() >= maxNodes_)
                return false;
            nodes_[id] = info;
            return true;
        }

        void erase(NodeId id)
        {
            std::lock_guard<std::mutex> lk(mu_);
            nodes_.erase(id);
        }

        void markDead(NodeId id)
        {
            std::lock_guard<std::mutex> lk(mu_);
            auto it = nodes_.find(id);
            if (it != nodes_.end())
                it->second.alive = false;
        }

        void update(NodeId id, const std::function<void(NodeInfo &)> &fn)
        {
            std::lock_guard<std::mutex> lk(mu_);
            auto it = nodes_.find(id);
            if (it != nodes_.end())
                fn(it->second);
        }

        std::optional<NodeInfo> get(NodeId id) const
        {
            std::lock_guard<std::mutex> lk(mu_);
            if (auto it = nodes_.find(id); it != nodes_.end())
                return it->second;
            return std::nullopt;
        }

        std::vector<std::pair<NodeId, NodeInfo>> snapshot() const
        {
            std::lock_guard<std::mutex> lk(mu_);
            std::vector<std::pair<NodeId, NodeInfo>> out;
            out.reserve(nodes_.size());
            for (auto &kv : nodes_)
                out.push_back(kv);
            return out;
        }

        size_t size() const
        {
            std::lock_guard<std::mutex> lk(mu_);
            return nodes_.size();
        }

        void setMax(size_t m) { maxNodes_ = m; }

    private:
        mutable std::mutex mu_;
        std::unordered_map<NodeId, NodeInfo> nodes_;
        size_t maxNodes_{8}; // default as requested
    };

} // namespace dist
