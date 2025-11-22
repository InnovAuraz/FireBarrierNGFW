#include "FlowBuilder.h"
#include <algorithm>

static const uint64_t FLOW_TIMEOUT_MS = 3000;  // finalize if inactive for 3s

FlowKey FlowBuilder::parsePacketKey(const CapturedPacket& pkt, bool& valid)
{
    valid = true;

    if (pkt.srcIp == 0 || pkt.dstIp == 0 || pkt.length == 0)
    {
        valid = false;
        return {};
    }

    FlowKey k;
    k.srcIp   = pkt.srcIp;
    k.dstIp   = pkt.dstIp;
    k.srcPort = pkt.srcPort;
    k.dstPort = pkt.dstPort;
    k.protocol = pkt.protocol;

    return k;
}

void FlowBuilder::processPacket(const CapturedPacket& pkt)
{
    bool valid = false;
    FlowKey key = parsePacketKey(pkt, valid);
    if (!valid) return;

    std::lock_guard<std::mutex> lock(m_mutex);

    auto& flow = m_flows[key];

    // First packet of a flow
    if (flow.firstSeenMs == 0)
    {
        flow.key = key;
        flow.firstSeenMs = pkt.timestampMs;
        flow.lastSeenMs  = pkt.timestampMs;
    }

    // Inter-arrival time
    uint64_t prevTs = flow.lastSeenMs;
    flow.lastSeenMs = pkt.timestampMs;

    if (prevTs != 0)
    {
        double delta = double(flow.lastSeenMs - prevTs);
        if (delta < 5000.0)
            flow.interArrivalMs.push_back(delta);
    }

    // Packet sizes
    flow.packetSizes.push_back((uint32_t)pkt.length);

    // Direction: PacketCapture fills this correctly
    flow.directions.push_back(pkt.direction);

    // Byte counters
    if (pkt.direction >= 0)
    {
        flow.bytesForward += pkt.length;
        flow.packetCountForward++;
    }
    else
    {
        flow.bytesReverse += pkt.length;
        flow.packetCountReverse++;
    }

    // TLS metadata
    flow.isTls       = pkt.isTls;
    flow.tlsVersion  = pkt.tlsVersion;
    flow.sni         = pkt.sni;
    flow.cipherSuite = pkt.cipherSuite;

    // Derived: duration
    flow.durationMs = flow.lastSeenMs - flow.firstSeenMs;

    // Derived: mean packet size
    if (!flow.packetSizes.empty())
    {
        double sum = 0;
        for (auto s : flow.packetSizes) sum += s;
        flow.meanPacketSize = sum / flow.packetSizes.size();
    }

    // Derived: entropy (very lightweight approximation)
    {
        std::vector<int> freq(10, 0);
        for (auto sz : flow.packetSizes)
        {
            int bucket = std::min<int>(sz / 150, 9);
            freq[bucket]++;
        }

        double total = double(flow.packetSizes.size());
        double H = 0.0;
        for (int f : freq)
        {
            if (f == 0) continue;
            double p = f / total;
            H -= p * std::log2(p);
        }
        flow.entropy = H;
    }
}

void FlowBuilder::checkForExpiredFlows(uint64_t nowMs)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<FlowKey> toDelete;

    for (auto& [key, flow] : m_flows)
    {
        uint64_t delta = nowMs - flow.lastSeenMs;

        if (delta > FLOW_TIMEOUT_MS)
        {
            if (m_flowReadyCallback)
                m_flowReadyCallback(flow);

            toDelete.push_back(key);
        }
    }

    for (const auto& k : toDelete)
        m_flows.erase(k);
}

void FlowBuilder::finalizeFlow(const FlowKey& key, FlowRecord& flow)
{
    if (m_flowReadyCallback)
        m_flowReadyCallback(flow);

    m_flows.erase(key);
}

std::vector<FlowRecord> FlowBuilder::getFlowsSnapshot()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<FlowRecord> out;
    out.reserve(m_flows.size());

    for (auto& [k, f] : m_flows)
        out.push_back(f);

    return out;
}

void FlowBuilder::reset()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_flows.clear();
}

void FlowBuilder::setFlowReadyCallback(std::function<void(const FlowRecord&)> cb)
{
    m_flowReadyCallback = cb;
}
