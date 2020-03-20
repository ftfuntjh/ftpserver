#ifndef FTPSERVER_FTPWORKER_H
#define FTPSERVER_FTPWORKER_H
#include <functional>
#include "../FTPOperator.h"
#include "ace/Activation_Queue.h"
#include "ace/Message_Block.h"
#include "ace/SOCK_Stream.h"
#include "ace/Task.h"

class FTPSession;
using Task = std::function<void(FTPSession* session, ACE_SOCK_Stream& socket,
                                std::string& suffix)>;
class FTPSchedule : public ACE_Task_Base {
   public:
    FTPSchedule();
    int svc() override;
    int enqueue(ACE_Method_Request* request);

   private:
    ACE_Activation_Queue activation_queue;
};
#endif
