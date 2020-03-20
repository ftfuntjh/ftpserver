#ifndef FTP_FTPACCEPTHANDLE_H
#define FTP_FTPACCEPTHANDLE_H

#include <string>
#include "ace/Event_Handler.h"
#include "ace/Reactor.h"
#include "ace/SOCK_Acceptor.h"

class FTPSchedule;
class FTPAcceptHandle : public ACE_Event_Handler {
   public:
    explicit FTPAcceptHandle(ACE_Reactor *reactor = ACE_Reactor::instance());

    ACE_INT32 open(const std::string &ip, const ACE_UINT16 &port);

    ACE_INT32 handle_input(ACE_HANDLE handle) final;

    ACE_INT32 handle_close(ACE_HANDLE handle, ACE_Reactor_Mask mask) final;

    ACE_HANDLE get_handle() const override;

    inline ACE_SOCK_Acceptor &get_acceptor() { return acceptor_; }

    void set_schedule(FTPSchedule * schedule);

   private:
    ACE_SOCK_Acceptor acceptor_;
    FTPSchedule *schedule;
};

#endif
