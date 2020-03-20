#include "FTPAcceptHandle.h"
#include "./ipc/FTPSchedule.h"
#include "FTPSession.h"
#include "ace/Log_Msg.h"

using namespace std;

FTPAcceptHandle::FTPAcceptHandle(ACE_Reactor *reactor)
    : ACE_Event_Handler(reactor), schedule(NULL) {}

ACE_INT32 FTPAcceptHandle::open(const string &ip, const ACE_UINT16 &port) {
    ACE_INET_Addr addr(port, ip.c_str());

    if (acceptor_.open(addr, 1) == -1)
        ACE_ERROR_RETURN(
            (LM_ERROR,
             ACE_TEXT("listen ip addr '%s:%d' failed errno=%d msg='%m'.\n"),
             ACE_TEXT(ip.c_str()), port, ACE_OS::last_error()),
            -1);

    return reactor()->register_handler(this, ACE_Event_Handler::ACCEPT_MASK);
}

ACE_INT32 FTPAcceptHandle::handle_input(ACE_HANDLE handle) {
    FTPSession *ftp_operator = NULL;
    ACE_NEW_RETURN(ftp_operator, FTPSession(reactor()), -1);
    ftp_operator->set_schedule(schedule);
    if (acceptor_.accept(ftp_operator->get_peer()) == -1) {
        ACE_DEBUG(
            (LM_DEBUG,
             ACE_TEXT("accept handle input error errno=%d msg='%m'. \n")));
        ftp_operator->handle_close(handle, 0);
        return -1;
    }

    if (ftp_operator->open() == -1) {
        ACE_DEBUG(
            (LM_DEBUG,
             ACE_TEXT("accept handle input open error. errno=%d msg='%m'.\n")));
        ftp_operator->handle_close(handle, 0);
        return -1;
    }

    ACE_DEBUG(
        (LM_DEBUG,
         ACE_TEXT("accept handle input ok, now send welcome message.\n")));
    ftp_operator->write("220 Welcome to ftfunjth FTP service\r\n");
    return 0;
}

ACE_HANDLE FTPAcceptHandle::get_handle() const {
    return acceptor_.get_handle();
}

ACE_INT32 FTPAcceptHandle::handle_close(ACE_HANDLE handle,
                                        ACE_Reactor_Mask mask) {
    acceptor_.close();
    delete this;
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("accept handle close ok.\n")));
    return 0;
}

void FTPAcceptHandle::set_schedule(FTPSchedule *schedule) {
    this->schedule = schedule;
}
