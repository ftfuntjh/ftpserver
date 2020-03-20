//
// Created by ftfunjth on 2020/3/15.
//

#ifndef FTPSERVER_TLS_LAYOUT_H
#define FTPSERVER_TLS_LAYOUT_H

#include "TLS_Record.h"
#include <vector>

class FTPSession;
struct TLS_Layout {
  std::vector<TLS_Record *> packets;
  int with(TLS_Record *record, FTPSession *session);
};

#endif // FTPSERVER_TLS_LAYOUT_H
