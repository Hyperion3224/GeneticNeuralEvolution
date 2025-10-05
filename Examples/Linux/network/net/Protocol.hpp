#pragma once
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <stdexcept>

namespace dist
{

    // Wire is: [u32 length][u8 type][payload bytes...], length = sizeof(type)+payloadLen, network byte order for u32.

    enum class MsgType : uint8_t
    {
        RESOURCE_REPORT = 1,
        PING = 2,
        PONG = 3,
        SHUTDOWN = 4,
    };

    struct ResourceReportPayload
    {
        uint64_t ramBytes; // total RAM on worker
        uint32_t threads;  // hardware threads
        // IP is inferred from socket's peer address; no need to send it.
    };

    // Helpers for host/network endian conversions (portable).
    inline uint32_t hostToNet32(uint32_t v)
    {
        unsigned char b[4] = {
            static_cast<unsigned char>((v >> 24) & 0xFF),
            static_cast<unsigned char>((v >> 16) & 0xFF),
            static_cast<unsigned char>((v >> 8) & 0xFF),
            static_cast<unsigned char>((v) & 0xFF),
        };
        uint32_t out;
        std::memcpy(&out, b, 4);
        return out;
    }
    inline uint32_t netToHost32(uint32_t v)
    {
        unsigned char b[4];
        std::memcpy(b, &v, 4);
        return (uint32_t(b[0]) << 24) | (uint32_t(b[1]) << 16) | (uint32_t(b[2]) << 8) | uint32_t(b[3]);
    }
    inline uint64_t hostToNet64(uint64_t v)
    {
        uint64_t r = 0;
        for (int i = 0; i < 8; ++i)
            r |= ((v >> (56 - 8 * i)) & 0xFFull) << (8 * (7 - i));
        return r;
    }
    inline uint64_t netToHost64(uint64_t v) { return hostToNet64(v); }

    // Serialize payloads
    inline std::vector<uint8_t> encodeResourceReport(const ResourceReportPayload &p)
    {
        std::vector<uint8_t> buf(12);
        uint64_t ramN = hostToNet64(p.ramBytes);
        uint32_t thrN = hostToNet32(p.threads);
        std::memcpy(buf.data(), &ramN, 8);
        std::memcpy(buf.data() + 8, &thrN, 4);
        return buf;
    }
    inline ResourceReportPayload decodeResourceReport(const std::vector<uint8_t> &buf)
    {
        if (buf.size() != 12)
            throw std::runtime_error("Bad ResourceReport size");
        ResourceReportPayload p{};
        uint64_t ramN;
        uint32_t thrN;
        std::memcpy(&ramN, buf.data(), 8);
        std::memcpy(&thrN, buf.data() + 8, 4);
        p.ramBytes = netToHost64(ramN);
        p.threads = netToHost32(thrN);
        return p;
    }

} // namespace dist
