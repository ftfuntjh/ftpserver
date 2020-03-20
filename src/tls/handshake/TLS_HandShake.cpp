//
// Created by ftfunjth on 2020/3/15.
//

#include "TLS_HandShake.h"
#include <cstring>
#include <new>

TLS_HandShake *TLS_HandShake::package(uint8_t *ptr, uint16_t len) {
  const uint8_t *iter = ptr;
  TLS_HandShake *record = new TLS_HandShake;
  memset(record, 0, sizeof(TLS_HandShake));
  COPY_TO((uint8_t *)&record->type, iter, sizeof(record->type));
  COPY_TO((uint8_t *)&record->length, iter, sizeof(record->length));
  COPY_TO((uint8_t *)&record->version, iter, sizeof(record->version));
  COPY_TO((uint8_t *)&record->random, iter, sizeof(record->random));
  COPY_TO((uint8_t *)&record->session_id_length, iter,
          sizeof(record->session_id_length));
  record->session_id = new uint8_t[record->session_id_length];
  COPY_TO((uint8_t *)&record->session_id, iter, record->session_id_length);
  return record;
}
