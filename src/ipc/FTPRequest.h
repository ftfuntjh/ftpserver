#ifndef FTPSERVER_FTPREQUEST_H
#define FTPSERVER_FTPREQUEST_H
#include <string>
#include "../FTPSession.h"
#include "ace/Future.h"
#include "ace/Method_Request.h"
class FTPRequest : public ACE_Method_Request {
   public:
    FTPRequest(FTPSession& session, std::string method, std::string arguments);
    int call() override;

   private:
    FTPSession& session;
    std::string method;
    std::string arguments;
    ACE_Future<int> ret;
};
#endif
