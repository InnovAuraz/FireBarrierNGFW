#include "PacketCapture.h"
#include "Logger.h"
#include "TlsInspector.h"

#include <pcap.h>
#include <chrono>

// Link against Npcap / WinPcap libs
#pragma comment(lib, "wpcap.lib")
#pragma comment(lib, "Packet.lib")

PacketCapture::PacketCapture() = default;

PacketCapture::~PacketCapture()
{
    stop();
}

void PacketCapture::setPacketHandler(PacketHandler handler)
{
    m_handler = std::move(handler);
}

bool PacketCapture::start(const std::string& deviceName)
{
    if (m_running.load())
        return false; // already running

    m_running.store(true);
    m_thread = std::thread(&PacketCapture::captureLoop, this, deviceName);
    return true;
}

bool PacketCapture::startAuto()
{
    if (m_running.load())
    {
        LOG_WARN("PacketCapture::startAuto called but capture is already running.");
        return true;
    }

    pcap_if_t* alldevs = nullptr;
    char errbuf[PCAP_ERRBUF_SIZE] = {0};

    if (pcap_findalldevs(&alldevs, errbuf) == -1)
    {
        LOG_ERROR(std::string("pcap_findalldevs failed: ") + errbuf);
        return false;
    }

    pcap_if_t* chosen = nullptr;

    for (pcap_if_t* d = alldevs; d != nullptr; d = d->next)
    {
        // Skip loopback interfaces
        if (d->flags & PCAP_IF_LOOPBACK)
            continue;

        // Optionally skip interfaces with no addresses
        if (d->addresses == nullptr)
            continue;

        LOG_INFO(std::string("PacketCapture: candidate device: ") + d->name);

        chosen = d;
        break;
    }

    if (!chosen)
    {
        LOG_ERROR("PacketCapture::startAuto - no suitable non-loopback device found.");
        pcap_freealldevs(alldevs);
        return false;
    }

    std::string devName = chosen->name;
    LOG_INFO(std::string("PacketCapture::startAuto - selected device: ") + devName);

    pcap_freealldevs(alldevs);

    // Reuse existing start(deviceName) logic
    return start(devName);
}

void PacketCapture::stop()
{
    if (!m_running.load())
        return;

    m_running.store(false);

    if (m_thread.joinable())
        m_thread.join();
}

bool PacketCapture::isRunning() const
{
    return m_running.load();
}

void PacketCapture::captureLoop(const std::string& deviceName)
{
    char errbuf[PCAP_ERRBUF_SIZE] = {0};

    LOG_INFO("Starting packet capture on device: " + deviceName);

    pcap_t* handle = pcap_open_live(
        deviceName.c_str(),
        65536,       // max capture size
        1,           // promiscuous mode
        1000,        // read timeout (ms)
        errbuf
    );

    if (!handle)
    {
        LOG_ERROR(std::string("pcap_open_live failed: ") + errbuf);
        m_running.store(false);
        return;
    }

    while (m_running.load())
    {
        struct pcap_pkthdr* header = nullptr;
        const u_char* data = nullptr;

        int res = pcap_next_ex(handle, &header, &data);

        if (res == 0)
            continue; // timeout
        else if (res == -1)
        {
            LOG_ERROR(std::string("pcap_next_ex error: ") + pcap_geterr(handle));
            break;
        }
        else if (res == -2)
            break; // EOF or breakloop

        if (!header || !data)
            continue;

        if (!m_handler)
            continue;

        // ----------------------------
        // Build CapturedPacket struct
        // ----------------------------
        CapturedPacket pkt;
        pkt.data   = reinterpret_cast<const uint8_t*>(data);
        pkt.length = header->caplen;

        pkt.timestampMs =
            static_cast<uint64_t>(header->ts.tv_sec) * 1000ULL +
            static_cast<uint64_t>(header->ts.tv_usec) / 1000ULL;

        // ----------------------------
        // Parse Ethernet header
        // ----------------------------
        if (pkt.length < 14)
        {
            m_handler(pkt);
            continue;
        }

        const uint8_t* eth = pkt.data;
        uint16_t ethType = (eth[12] << 8) | eth[13];

        if (ethType != 0x0800) // Not IPv4
        {
            m_handler(pkt);
            continue;
        }

        // ----------------------------
        // Parse IPv4 header
        // ----------------------------
        const uint8_t* ip = eth + 14;
        if (pkt.length < 14 + 20)
        {
            m_handler(pkt);
            continue;
        }

        uint8_t ihl = (ip[0] & 0x0F) * 4;
        if (ihl < 20 || pkt.length < 14 + ihl)
        {
            m_handler(pkt);
            continue;
        }

        pkt.protocol = ip[9];

        pkt.srcIp =
            (ip[12] << 24) | (ip[13] << 16) |
            (ip[14] << 8) | ip[15];

        pkt.dstIp =
            (ip[16] << 24) | (ip[17] << 16) |
            (ip[18] << 8) | ip[19];

        // ----------------------------
        // Parse TCP or UDP header
        // ----------------------------
        const uint8_t* l4 = ip + ihl;

        if (pkt.protocol == 6) // TCP
        {
            if (pkt.length >= (size_t)(14 + ihl + 4))
            {
                pkt.srcPort = (l4[0] << 8) | l4[1];
                pkt.dstPort = (l4[2] << 8) | l4[3];
            }
        }
        else if (pkt.protocol == 17) // UDP
        {
            if (pkt.length >= (size_t)(14 + ihl + 4))
            {
                pkt.srcPort = (l4[0] << 8) | l4[1];
                pkt.dstPort = (l4[2] << 8) | l4[3];
            }
        }
        else
        {
            // Unsupported protocol – still allow packet through
        }

        // ----------------------------
        // Compute direction (forward = +1, reverse = -1)
        // ----------------------------
        // Simple rule: Forward = srcIp < dstIp, otherwise reverse
        if (pkt.srcIp < pkt.dstIp)
            pkt.direction = +1;
        else if (pkt.srcIp > pkt.dstIp)
            pkt.direction = -1;
        else
            pkt.direction = 0; // same IP (rare)

        // ----------------------------
        // TLS inspection
        // ----------------------------
        TlsInspector::inspect(
            pkt.data,
            pkt.length,
            pkt.isTls,
            pkt.tlsVersion,
            pkt.sni,
            pkt.cipherSuite
        );

        // ----------------------------
        // Deliver annotated packet
        // ----------------------------
        m_handler(pkt);
    }

    pcap_close(handle);
    LOG_INFO("Packet capture stopped on device: " + deviceName);

    m_running.store(false);
}
