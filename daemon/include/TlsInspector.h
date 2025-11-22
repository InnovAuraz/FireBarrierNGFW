#pragma once
#include <cstdint>
#include <string>

class TlsInspector
{
public:
    // Inspect packet bytes for TLS ClientHello
    // Returns true if TLS-like handshake detected
    static bool inspect(const uint8_t* data,
                        size_t len,
                        bool& isTls,
                        std::string& tlsVersion,
                        std::string& sni,
                        std::string& cipherSuite);

private:
    static bool parseClientHello(const uint8_t* data, size_t len,
                                 std::string& tlsVersion,
                                 std::string& sni,
                                 std::string& cipherSuite);
};