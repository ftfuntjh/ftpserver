//
// Created by ftfunjth on 2020/3/15.
//

#include "TLS_Layout.h"
#include "../FTPSession.h"
#include "./handshake/TLS_HandShake.h"

int TLS_Layout::with(TLS_Record *record, FTPSession *session) {
  packets.emplace_back(record);
  switch (record->type) {
  case TLS_RecordType::HANDSHAKE:
    TLS_HandShake *hand_shake_packet =
        TLS_HandShake::package(&record->body[0], record->length);
    uint32_t len = hand_shake_packet->get_message_length();
    break;
  }
  return 0;
}
