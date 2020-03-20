//
// Created by ftfunjth on 2020/3/15.
//

#ifndef FTPSERVER_TLS_HANDSHAKE_H
#define FTPSERVER_TLS_HANDSHAKE_H

#include "../TLS_Record.h"

enum class TLS_HandshakeType : uint8_t {
  HELLO_REQUEST = 0,
  CLIENT_HELLO = 1,
  SERVER_HELLO = 2,
  CERTIFICATE = 11,
  SERVER_KEY_EXCHANGE = 12,
  CERTIFICATE_REQUEST = 13,
  SERVER_HELLO_DONE = 14,
  CERTIFICATE_VERIFY = 15,
  CLIENT_KEY_EXCHANGE = 16,
  FINISHED = 20
};
struct TLS_HandShake {
  TLS_HandshakeType type;
  uint8_t length[3];
  TLS_ProtocolVersion version;
  struct {
    uint32_t gmt_unix_time;
    uint8_t random_bytes[28];
  } random;
  uint8_t session_id_length;
  uint8_t *session_id;
  void get_session_id(uint8_t **result, uint8_t *len) {
    *len = session_id_length;
    *result = &session_id_length + 1;
  }
  uint32_t get_message_length() {
    return length[2] * 0x10000 + length[1] * 0x100 + length[0];
  }

  static TLS_HandShake *package(uint8_t *ptr, uint16_t len);
};

#endif // FTPSERVER_TLS_HANDSHAKE_H
