#ifndef FTP_FPTSERVER_H
#define FTP_FPTSERVER_H
#include <string>
#include "./ipc/FTPSchedule.h"
#include "FTPAcceptHandle.h"
void set_ipaddr(const std::string &addr);
class FTPServer {
   public:
    FTPServer(const std::string &ip, const ACE_UINT16 &port, int thread_count);

    int open();

   private:
    std::string ip;
    ACE_UINT16 port;
    FTPAcceptHandle accept_handle;
    FTPSchedule work_threads;
    int thread_count;
};
#endif
