#ifndef FTPSERVER_FTPPROXY_H
#define FTPSERVER_FTPPROXY_H
#include "FTPRequest.h"
#include "FTPSchedule.h"
class FTPProxy {
   public:
    explicit FTPProxy(int thread_count);
    void call(FTPSession& session, std::string method, std::string arguments);
    void activate();
    void exit();

   private:
    FTPSchedule schedule;
}

#endif
