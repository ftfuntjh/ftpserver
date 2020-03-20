#include "FTPSession.h"
#include <algorithm>
#include <regex>
#include "./ipc/FTPRequest.h"
#include "./ipc/FTPSchedule.h"
#include "FTPOperator.h"
#include "ace/Future.h"
#define CR '\r'
#define LF '\n'
#define ZERO '\0'
using namespace std;

#define ADD_TABLE_ENTITY(command) \
    make_pair<string, command_operator>(string(#command), &command)

map<string, command_operator> operator_map = {
    ADD_TABLE_ENTITY(USER), ADD_TABLE_ENTITY(PASS), ADD_TABLE_ENTITY(PORT),
    ADD_TABLE_ENTITY(SIZE), ADD_TABLE_ENTITY(LIST), ADD_TABLE_ENTITY(PASV),
    ADD_TABLE_ENTITY(PWD),  ADD_TABLE_ENTITY(TYPE), ADD_TABLE_ENTITY(CWD),
    ADD_TABLE_ENTITY(RETR), ADD_TABLE_ENTITY(STOR), ADD_TABLE_ENTITY(MKD),
    ADD_TABLE_ENTITY(SYST), ADD_TABLE_ENTITY(FEAT), ADD_TABLE_ENTITY(MDTM),
    ADD_TABLE_ENTITY(OPTS), ADD_TABLE_ENTITY(MLSD), ADD_TABLE_ENTITY(MLST),
    ADD_TABLE_ENTITY(DELE), ADD_TABLE_ENTITY(QUIT), ADD_TABLE_ENTITY(RNFR),
    ADD_TABLE_ENTITY(RNTO), ADD_TABLE_ENTITY(SITE), ADD_TABLE_ENTITY(NOOP),
    ADD_TABLE_ENTITY(MODE), ADD_TABLE_ENTITY(RMD),
};

FTPSession::FTPSession(ACE_Reactor *r)
    : ACE_Event_Handler(r),
      uid(-1),
      gid(-1),
      handle(ACE_INVALID_HANDLE),
      transfer_type(ASCII),
      over_tsl(false),
      proxy(NULL) {
    memset(buffer, 0, sizeof(buffer));
}

ACE_INT32 FTPSession::open() {
    ACE_INT32 ret = 0;
    ACE_INET_Addr remote_addr;
    get_peer().get_remote_addr(remote_addr);
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("open socket remote address is %s.\n"),
               ACE_TEXT(remote_addr.get_host_addr())));

    ret = reactor()->register_handler(peer.get_handle(), this,
                                      ACE_Event_Handler::READ_MASK);
    if (ret != 0)
        ACE_ERROR_RETURN(
            (LM_DEBUG, ACE_TEXT("register handle failed errno=%d msg='%m'.\n")),
            ret);
    return ret;
}

ACE_INT32 FTPSession::handle_input(ACE_HANDLE handle) {
    ACE_INT32 len;
    if (strlen(buffer) > sizeof(buffer) - 1) {
        ACE_DEBUG(
            (LM_DEBUG,
             ACE_TEXT(
                 "session recv buffer not enough space.just clear all.\n")));
        memset(buffer, 0, sizeof(buffer));
    }
    len = get_peer().recv(buffer, sizeof(buffer) - strlen(buffer) - 1);
    if (len == 0) {
        ACE_DEBUG((LM_DEBUG, "session remote close socket connection.\n"));
        return -1;
    }

    if (len < 0) {
        ACE_DEBUG((LM_DEBUG, "session socket connection error, errno=%d '%m'",
                   ACE_OS::last_error()));
        return -1;
    }

    if (over_tsl) {
        TLS_Record *record = TLS_Record::package((uint8_t *)buffer, len);
        tls_layout.with(record, this);
        delete record;
        return 0;
    }
    char *ptr = buffer;
    for (int i = 0; i < strlen(ptr) - 1; i++) {
        if (ptr[i] == CR && ptr[i + 1] == LF) {
            commands.emplace_back(ptr, ptr + i);
            ptr += i + 2;
            break;
        }
    }
    int i = 0;
    while (ptr < &buffer[sizeof(buffer)] && *ptr != ZERO) {
        buffer[i++] = *ptr++;
    }
    buffer[i] = ZERO;
    if (i < sizeof(buffer) - 1) {
        memset(&buffer[i + 1], 0, sizeof(buffer) - i - 1);
    }
    process();
    return 0;
}

ACE_INT32 FTPSession::handle_close(ACE_HANDLE handle, ACE_Reactor_Mask mask) {
    return ACE_Event_Handler::handle_close(handle, mask);
}
void FTPSession::process() {
    static regex command_regex(R"(^(\S+)\s*(.*)$)");
    if (commands.empty()) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("no incoming command.\n")));
        return;
    }

    while (!commands.empty()) {
        string command = commands[0];
        commands.erase(commands.begin());
        smatch match;
        if (!regex_match(command, match, command_regex)) {
            ACE_DEBUG((LM_DEBUG, ACE_TEXT("receive invalid command %s.\n"),
                       ACE_TEXT(command.c_str())));
            continue;
        }
        string command_prefix = match[1];
        string command_suffix = match[2];
        event_process(command_prefix, command_suffix);
    }
}
void FTPSession::event_process(string prefix, string suffix) {
    transform(prefix.begin(), prefix.end(), prefix.begin(), ::toupper);

    operator_iterator op = operator_map.find(prefix);
    if (op == operator_map.end()) {
        ACE_DEBUG(
            (LM_DEBUG, "command %s unrealized.\n", ACE_TEXT(prefix.c_str())));
        write("500 unrecognized command\r\n");
        return;
    }
    FTPRequest *request = new FTPRequest(*this, prefix, suffix);
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("command %s start process.\n"),
               ACE_TEXT(prefix.c_str())));
    ACE_Future<int> ret = proxy->enqueue(request);
    // op->second(this, this->get_peer(), suffix);
}
void FTPSession::write(const char *message) {
    peer.send_n(message, strlen(message));
}

void FTPSession::set_user(FTPUser user) { this->user = user; }

FTPUser &FTPSession::get_user() { return user; }

void FTPSession::set_transfer_type(const TransferType &type) {
    transfer_type = type;
}
string FTPSession::list_dir(const string &path) {
    string dest = path.empty() ? user.work_directory : path;
    map<string, FTPEntity>::iterator ent = open_dirs.find(dest);
    if (ent != open_dirs.end()) {
        return ent->second.ls();
    } else {
        open_dirs.emplace(dest, dest.c_str());
        ent = open_dirs.find(dest);
        ent->second.open();
        return ent->second.ls();
    }
}
void FTPSession::set_over_tsl(bool over) { over_tsl = over; }

void FTPSession::set_rename_from(const string &path) { rename_from = path; }

const string &FTPSession::get_rename_from() { return rename_from; }

const FTPEntity &FTPSession::get_dir(const string &path) {
    string dest = path.empty() ? user.work_directory : path;
    map<string, FTPEntity>::iterator ent = open_dirs.find(dest);
    if (ent != open_dirs.end()) {
        return ent->second;
    } else {
        open_dirs.emplace(dest, dest.c_str());
        ent = open_dirs.find(dest);
        ent->second.open();
        return ent->second;
    }
}
void FTPSession::close_dir(const string &path) {
    map<string, FTPEntity>::iterator ent = open_dirs.find(path);
    if (ent != open_dirs.end()) {
        open_dirs.erase(ent);
    }
}

void FTPSession::set_schedule(FTPSchedule *schedule) { this->proxy = schedule; }
