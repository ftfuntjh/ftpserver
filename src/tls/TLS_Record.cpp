//
// Created by ftfunjth on 2020/3/15.
//

#include "TLS_Record.h"
#include <cstring>
#include <new>
using namespace std;

TLS_Record *TLS_Record::package(const uint8_t *buffer, uint16_t len) {
  TLS_Record *record = new TLS_Record;
  const uint8_t *iter = buffer;
  memset(record, 0, sizeof(TLS_Record));
  COPY_TO((uint8_t *)&record->type, iter, sizeof(record->type));
  COPY_TO((uint8_t *)&record->version, iter, sizeof(record->version));
  COPY_TO((uint8_t *)&record->length, iter, sizeof(record->length));
  record->body = new uint8_t[record->length];
  memcpy(record->body, iter, record->length);
  return record;
}
