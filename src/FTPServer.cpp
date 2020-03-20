#include "FTPServer.h"
#include "ace/Log_Msg.h"
using namespace std;

FTPServer::FTPServer(const string &ip, const ACE_UINT16 &port, int thread_count)
    : ip(ip), port(port), thread_count(thread_count) {}

int FTPServer::open() {
    work_threads.activate(THR_NEW_LWP | THR_JOINABLE | THR_INHERIT_SCHED,thread_count);
    accept_handle.set_schedule(&work_threads);
    if (accept_handle.open(ip, port) != 0)
        ACE_ERROR_RETURN(
            (LM_ERROR,
             ACE_TEXT("open listen port failed errno=%d msg='%m'.\n")),
            -1);
    return 0;
}
