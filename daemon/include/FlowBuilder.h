#pragma once
#include <cstdint>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <string>
#include <functional>
#include <cmath>
#include "PacketCapture.h"

struct FlowKey
{
    uint32_t srcIp;
    uint32_t dstIp;
    uint16_t srcPort;
    uint16_t dstPort;
    uint8_t  protocol;

    bool operator==(const FlowKey& other) const
    {
        return srcIp == other.srcIp &&
               dstIp == other.dstIp &&
               srcPort == other.srcPort &&
               dstPort == other.dstPort &&
               protocol == other.protocol;
    }
};

struct FlowKeyHash
{
    std::size_t operator()(const FlowKey& k) const
    {
        std::size_t h1 = std::hash<uint32_t>()(k.srcIp);
        std::size_t h2 = std::hash<uint32_t>()(k.dstIp);
        std::size_t h3 = std::hash<uint16_t>()(k.srcPort);
        std::size_t h4 = std::hash<uint16_t>()(k.dstPort);
        std::size_t h5 = std::hash<uint8_t>()(k.protocol);

        return (((h1 ^ (h2 << 1)) ^ (h3 << 2)) ^ (h4 << 3)) ^ (h5 << 4);
    }
};

struct FlowRecord
{
    FlowKey key;

    uint64_t firstSeenMs = 0;
    uint64_t lastSeenMs  = 0;

    uint64_t bytesForward = 0;
    uint64_t bytesReverse = 0;

    uint32_t packetCountForward = 0;
    uint32_t packetCountReverse = 0;

    // ML input features
    std::vector<uint32_t> packetSizes;
    std::vector<int>      directions;
    std::vector<double>   interArrivalMs;

    // TLS metadata
    bool        isTls       = false;
    std::string tlsVersion;
    std::string sni;
    std::string cipherSuite;

    // Derived ML stats
    double   entropy        = 0.0;
    double   meanPacketSize = 0.0;
    uint64_t durationMs     = 0;
};

class FlowBuilder
{
public:
    FlowBuilder() = default;
    ~FlowBuilder() = default;

    void processPacket(const CapturedPacket& pkt);

    std::vector<FlowRecord> getFlowsSnapshot();

    void checkForExpiredFlows(uint64_t nowMs);

    void reset();

    void setFlowReadyCallback(std::function<void(const FlowRecord&)> cb);

private:
    FlowKey parsePacketKey(const CapturedPacket& pkt, bool& valid);
    void finalizeFlow(const FlowKey& key, FlowRecord& flow);

private:
    std::unordered_map<FlowKey, FlowRecord, FlowKeyHash> m_flows;
    std::mutex m_mutex;

    std::function<void(const FlowRecord&)> m_flowReadyCallback;
};
