//
// Created by ftfunjth on 2020/3/15.
//

#ifndef FTPSERVER_TLSRECORD_H
#define FTPSERVER_TLSRECORD_H
#include <cstdint>
#include <cstring>
#define COPY_TO(dest, ptr, len)                                                \
  do {                                                                         \
    uint32_t len_ = (uint32_t)len;                                             \
    for (int i = len_ - 1; i >= 0; i--) {                                      \
      (dest)[i] = *ptr++;                                                      \
    }                                                                          \
  } while (0)

enum class TLS_RecordType : uint8_t {
  CHANGE_CIPHER_SPEC = 20,
  ALERT,
  HANDSHAKE,
  APPLICATION_DATA,
};

struct TLS_ProtocolVersion {
  uint8_t major;
  uint8_t minor;
};
struct TLS_Record {
  TLS_RecordType type;         // Record type
  TLS_ProtocolVersion version; // Protocol version
  uint16_t length;             // Layer packet length
  uint8_t *body;               // Record Body
  TLS_Record() = default;
  static TLS_Record *package(const uint8_t *buffer, uint16_t len);
};

#endif // FTPSERVER_TLSRECORD_H
