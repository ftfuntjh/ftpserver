#ifndef FTP_FTPEVENTHANDLE_H
#define FTP_FTPEVENTHANDLE_H

#include <functional>
#include <map>
#include <string>
#include <vector>
#include "FTPUser.h"
#include "ace/Event_Handler.h"
#include "ace/Guard_T.h"
#include "ace/Reactor.h"
#include "ace/SOCK_Acceptor.h"
#include "ace/SOCK_Connector.h"
#include "ace/SOCK_Stream.h"
#include "ace/Thread_Mutex.h"
#include "io/FTPEntity.h"
#include "tls/TLS_Layout.h"

class FTPSession;
using command_operator = std::function<void(
    FTPSession *session, ACE_SOCK_Stream &socket, std::string &suffix)>;

using operator_iterator = std::map<std::string, command_operator>::iterator;

class FTPSchedule;
class FTPSession : public ACE_Event_Handler {
   public:
    enum TransferType {
        ASCII,
        BINARY,
    };
    explicit FTPSession(ACE_Reactor *reactor = ACE_Reactor::instance());

    inline ACE_HANDLE get_handle() { return handle; }

    ACE_INT32 open();

    ACE_INT32 handle_input(ACE_HANDLE handle) override;

    ACE_INT32 handle_close(ACE_HANDLE handle, ACE_Reactor_Mask mask) override;

    inline ACE_SOCK_Stream &get_peer() { return peer; }

    inline ACE_SOCK_Stream &get_data_peer() { return data_peer; }

    virtual void process();

    virtual void event_process(std::string prefix, std::string suffix);

    void set_schedule(FTPSchedule *schedule);

    void write(const char *message);

    void set_user(FTPUser user);

    FTPUser &get_user();

    void set_transfer_type(const TransferType &type);

    std::string list_dir(const std::string &path);

    void close_dir(const std::string &path);

    const FTPEntity &get_dir(const std::string &path);

    void set_over_tsl(bool over);

    void set_rename_from(const std::string &path);

    const std::string &get_rename_from();

    inline ACE_Thread_Mutex &get_mutex() { return mutex; }

   private:
    int uid;
    int gid;
    FTPUser user;
    bool over_tsl;
    std::string cwd;
    ACE_HANDLE handle;
    std::string rename_from;
    std::map<std::string, FTPEntity> open_dirs;
    char buffer[512];
    TLS_Layout tls_layout;
    std::vector<std::string> commands;
    ACE_SOCK_Stream peer;
    ACE_SOCK_Stream data_peer;
    TransferType transfer_type;
    FTPSchedule *proxy;
    ACE_Thread_Mutex mutex;
};
#endif
