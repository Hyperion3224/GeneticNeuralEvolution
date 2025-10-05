#pragma once
#include <string>
#include <vector>
#include <atomic>
#include <chrono>
#include <functional>
#include <thread>
#include "Protocol.hpp"
#include "Logger.hpp"

#if defined(_WIN32)
  #include <winsock2.h>
  #include <ws2tcpip.h>
  using socket_t = SOCKET;
  #ifndef SHUT_RDWR
    #define SHUT_RDWR SD_BOTH
  #endif
#else
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netinet/tcp.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  using socket_t = int;
  #define INVALID_SOCKET (-1)
  #define closesocket ::close
#endif

namespace dist {

struct NodeInfo {
    std::string ip;
    uint64_t ramBytes = 0;
    uint32_t threads  = 0;
    std::chrono::steady_clock::time_point lastSeen{};
    bool alive = true;
};

class Connection {
public:
    Connection(socket_t s, std::string peerIp)
        : sock_(s), peerIp_(std::move(peerIp)), lastSeen_(std::chrono::steady_clock::now()) {}

    ~Connection() { close(); }

    const std::string& peerIp() const { return peerIp_; }

    void updateLastSeen() { lastSeen_ = std::chrono::steady_clock::now(); }
    std::chrono::steady_clock::time_point lastSeen() const { return lastSeen_; }

    bool sendMessage(MsgType type, const std::vector<uint8_t>& payload) {
        uint32_t len = static_cast<uint32_t>(1 + payload.size());
        uint32_t lenN = hostToNet32(len);

        std::vector<uint8_t> frame(4 + len);
        std::memcpy(frame.data(), &lenN, 4);
        frame[4] = static_cast<uint8_t>(type);
        if (!payload.empty()) std::memcpy(frame.data()+5, payload.data(), payload.size());
        return sendAll(frame.data(), frame.size());
    }

    // Returns false on peer disconnect or fatal error; fills out parameters on success.
    bool recvMessage(MsgType& typeOut, std::vector<uint8_t>& payloadOut) {
        payloadOut.clear();

        uint32_t lenN;
        if (!recvAll(&lenN, 4)) return false;
        uint32_t len = netToHost32(lenN);
        if (len == 0) { LOG_WARN("Received zero-length frame from %s", peerIp_.c_str()); return false; }
        std::vector<uint8_t> buf(len);
        if (!recvAll(buf.data(), len)) return false;

        typeOut = static_cast<MsgType>(buf[0]);
        if (len > 1) payloadOut.assign(buf.begin()+1, buf.end());
        else payloadOut.clear();

        updateLastSeen();
        return true;
    }

    void close() {
        if (sock_ != INVALID_SOCKET) {
            ::shutdown(sock_, SHUT_RDWR);
            ::close(sock_);
            sock_ = INVALID_SOCKET;
        }
    }

    socket_t raw() const { return sock_; }

private:
    bool sendAll(const void* data, size_t len) {
        const uint8_t* p = static_cast<const uint8_t*>(data);
        size_t sent = 0;
        while (sent < len) {
#if defined(_WIN32)
            int n = ::send(sock_, reinterpret_cast<const char*>(p + sent), int(len - sent), 0);
#else
            ssize_t n = ::send(sock_, p + sent, len - sent, MSG_NOSIGNAL);
#endif
            if (n <= 0) { return false; }
            sent += size_t(n);
        }
        return true;
    }

    bool recvAll(void* out, size_t len) {
        uint8_t* p = static_cast<uint8_t*>(out);
        size_t recvd = 0;
        while (recvd < len) {
#if defined(_WIN32)
            int n = ::recv(sock_, reinterpret_cast<char*>(p + recvd), int(len - recvd), 0);
#else
            ssize_t n = ::recv(sock_, p + recvd, len - recvd, 0);
#endif
            if (n == 0) return false; // peer closed
            if (n < 0)  return false;
            recvd += size_t(n);
        }
        return true;
    }

    socket_t sock_{INVALID_SOCKET};
    std::string peerIp_;
    std::chrono::steady_clock::time_point lastSeen_;
};

} // namespace dist
