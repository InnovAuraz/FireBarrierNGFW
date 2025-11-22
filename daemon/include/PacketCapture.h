#pragma once

#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <cstdint>

struct CapturedPacket
{
    const uint8_t* data = nullptr;
    std::size_t length = 0;
    uint64_t timestampMs = 0;

    // --- NEW FIELDS (but unused for now) ---
    uint32_t srcIp = 0;
    uint32_t dstIp = 0;
    uint16_t srcPort = 0;
    uint16_t dstPort = 0;
    uint8_t protocol = 0;   // 6 = TCP, 17 = UDP

    int direction = 0;      // +1 forward, -1 reverse

    // TLS identification flags (optional, for future steps)
    bool isTls = false;
    std::string tlsVersion;
    std::string sni;
    std::string cipherSuite;
};

class PacketCapture
{
public:
    PacketCapture();
    ~PacketCapture();

    // Type for packet callback
    using PacketHandler = std::function<void(const CapturedPacket&)>;

    // Set the callback that will receive packets
    void setPacketHandler(PacketHandler handler);

    // Start capturing on given device name (from Config::pcapDevice())
    // Returns false on failure (device open error, etc.)
    bool start(const std::string& deviceName);

    bool startAuto();

    // Stop capturing and join thread
    void stop();

    // Is capture currently running?
    bool isRunning() const;

private:
    void captureLoop(const std::string& deviceName);

private:
    std::thread m_thread;
    std::atomic<bool> m_running{false};

    PacketHandler m_handler = nullptr;
};
