#include "TlsInspector.h"
#include <cstring>

static uint16_t read16(const uint8_t* p) {
    return (p[0] << 8) | p[1];
}

bool TlsInspector::inspect(const uint8_t* data,
                           size_t len,
                           bool& isTls,
                           std::string& tlsVersion,
                           std::string& sni,
                           std::string& cipherSuite)
{
    isTls = false;
    tlsVersion.clear();
    sni.clear();
    cipherSuite.clear();

    // TLS record header minimum: 5 bytes
    if (len < 5) return false;

    // TLS handshake content type = 22
    if (data[0] != 22) return false;

    uint8_t major = data[1];
    uint8_t minor = data[2];

    // Check TLS versions
    if (!(major == 3)) return false;

    // Now try to parse ClientHello
    if (!parseClientHello(data, len, tlsVersion, sni, cipherSuite))
        return false;

    isTls = true;
    return true;
}

bool TlsInspector::parseClientHello(const uint8_t* data, size_t len,
                                    std::string& tlsVersion,
                                    std::string& sni,
                                    std::string& cipherSuite)
{
    if (len < 5) return false;

    size_t pos = 5;
    if (pos + 4 > len) return false;

    if (data[pos] != 1) return false;  // HandshakeType = ClientHello

    uint32_t hsLen =
        (data[pos + 1] << 16) |
        (data[pos + 2] << 8) |
         data[pos + 3];

    pos += 4;
    if (pos + hsLen > len) return false;

    size_t clientHelloStart = pos;

    // Skip: client version (2 bytes), random (32 bytes), session ID length+id
    pos += 2 + 32;
    if (pos >= len) return false;

    uint8_t sessionIdLen = data[pos];
    pos += 1 + sessionIdLen;
    if (pos >= len) return false;

    // Cipher suites
    if (pos + 2 > len) return false;
    uint16_t cipherLen = read16(&data[pos]);
    pos += 2;

    if (pos + cipherLen > len) return false;

    if (cipherLen >= 2) {
        uint16_t cs = read16(&data[pos]);
        cipherSuite = "0x" + std::to_string(cs);
    }

    pos += cipherLen;

    // Compression methods
    if (pos + 1 > len) return false;
    uint8_t compLen = data[pos];
    pos += 1 + compLen;
    if (pos >= len) return false;

    // Extensions
    if (pos + 2 > len) return false;
    uint16_t extLen = read16(&data[pos]);
    pos += 2;

    if (pos + extLen > len) return false;

    size_t extEnd = pos + extLen;

    while (pos + 4 <= extEnd) {
        uint16_t extType = read16(&data[pos]);
        uint16_t extSize = read16(&data[pos + 2]);
        pos += 4;

        if (pos + extSize > extEnd) break;

        // SNI extension
        if (extType == 0) {
            size_t sniPos = pos + 2;
            if (sniPos + 3 < pos + extSize) {
                uint8_t nameType = data[sniPos];
                uint16_t nameLen = read16(&data[sniPos + 1]);
                if (nameType == 0 && sniPos + 3 + nameLen <= pos + extSize) {
                    sni.assign((const char*)&data[sniPos + 3], nameLen);
                }
            }
        }

        pos += extSize;
    }

    tlsVersion = "TLS 1." + std::to_string(data[2] - 1);
    return true;
}
