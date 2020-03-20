//
// Created by ftfunjth on 2020/3/14.
//

#include "FTPOperator.h"
#include <crypt.h>
#include <regex>
#include "FTPUser.h"
#include "ace/Mem_Map.h"
#include "ace/OS.h"
#include "cwalk.h"
using namespace std;

string ipaddr;

void set_ipaddr(const string &addr) { ipaddr = addr; }

static void passive(FTPSession *session, ACE_SOCK_Stream &socket,
                    string &suffix, bool epsv) {
    const regex ip_port_regex(R"((\d+)\.(\d+)\.(\d+)\.(\d+):(\d+))");
    char buffer[64];
    char remote_buffer[64];
    ACE_SOCK_Acceptor acceptor;
    ACE_Time_Value timeout(5);
    ACE_INET_Addr local_address, remote_address;
    if (acceptor.open(dynamic_cast<ACE_Addr &>(local_address), 1) != 0) {
        ACE_ERROR((LM_ERROR, "client acceptor open failed errno=%d msg='%m'.\n",
                   ACE_OS::last_error()));
        return;
    }
    memset(buffer, 0, sizeof(buffer));
    memset(remote_buffer, 0, sizeof(remote_buffer));
    acceptor.get_local_addr(local_address);
    session->get_peer().get_local_addr(remote_address);
    local_address.addr_to_string(buffer, sizeof(buffer));
    remote_address.addr_to_string(remote_buffer, sizeof(remote_buffer));
    string ip_port(buffer);
    string remote_ip_port(remote_buffer);
    smatch match;
    if (!regex_match(remote_ip_port, match, ip_port_regex)) {
        ACE_ERROR((LM_ERROR, "invalid regexp match.\n"));
        session->write("421 Server error.\r\n");
        return;
    }
    string message("227 Entering Extended Passive Mode ");
    if (!regex_match(ip_port, match, ip_port_regex)) {
        ACE_ERROR((LM_ERROR, "invalid regexp match.\n"));
        return;
    }

    string port_str = match[5];
    if (epsv) {
        message += "(|||" + port_str + "|)\r\n";
    } else {
        ipaddr = regex_replace(ipaddr, regex(R"(\.)"), R"(,)");
        message += ipaddr + ",";
        long port = strtol(port_str.c_str(), nullptr, 10);
        stringstream ss;
        ss << (port >> 8) << "," << (port & 0x00ffu);
        port_str.clear();
        ss >> port_str;
        message = message + port_str + "\r\n";
    }

    session->write(message.c_str());

    if (acceptor.accept(session->get_data_peer(), NULL, &timeout) != 0) {
        ACE_ERROR((LM_ERROR, "client timeout 5 seconds not connect.\n"));
    }
    acceptor.close();
}

static string absolute(const string &cwd, const string &path) {
    char buffer[512];
    if (cwk_path_is_absolute(path.c_str())) {
        return path;
    }
    memset(buffer, 0, sizeof(buffer));
    cwk_path_get_absolute(cwd.c_str(), path.c_str(), buffer, sizeof(buffer));
    return buffer;
}

#define OPERATOR_DEFINE(name) \
    void name(FTPSession *session, ACE_SOCK_Stream &socket, string &suffix)

OPERATOR_DEFINE(AUTH) {
    session->set_over_tsl(true);
    session->write("220 OK establish TLS connection.\r\n");
}

OPERATOR_DEFINE(MLSD) {
    regex invalid_path(R"(-)");
    suffix = absolute(session->get_user().work_directory, suffix);
    if (!session->get_user().is_allowed(suffix)) {
        session->write("500 no permission to do it.\r\n");
        return;
    }
    const FTPEntity &entity = session->get_dir(suffix);
    if (!entity.is_dirent()) {
        session->write("501 not a dirent.\r\n");
        session->get_data_peer().close();
        return;
    }

    if (!entity.is_exist()) {
        session->write("550 dir not exists.\r\n");
        session->get_data_peer().close();
        return;
    }

    if(session->get_data_peer().get_handle() == ACE_INVALID_HANDLE){
      session->write("500 User Not Connected.");
      session->get_data_peer().close();
      return;
    }
    session->write("150 Opening ASCII mode data connection for file list\r\n");

    string list = entity.mlsd();
    session->get_data_peer().send(list.c_str(), list.length());
    session->get_data_peer().close();
    session->write("226 Transfer complete.\r\n");
}

OPERATOR_DEFINE(LIST) {
    regex invalid_path(R"(-)");
    if (regex_search(suffix, invalid_path)) {
        suffix = session->get_user().work_directory;
    } else {
        suffix = absolute(session->get_user().work_directory, suffix);
    }
    if (!session->get_user().is_allowed(suffix)) {
        session->write("500 no permission to do it.\r\n");
        return;
    }
    session->write("150 Opening ASCII mode data connection for file list\r\n");
    string info = session->list_dir(suffix);
    session->get_data_peer().send(info.c_str(), info.length());
    session->get_data_peer().close();
    session->write("226 Transfer complete.\r\n");
}

OPERATOR_DEFINE(USER) {
    FTPUser user(suffix);
    if (user) {
        session->set_user(user);
        session->write("331 Username OK, Password required\r\n");
        return;
    }
    session->write("530 User not exist\r\n");
}

OPERATOR_DEFINE(PASV) { passive(session, socket, suffix, false); }

OPERATOR_DEFINE(SIZE) {
    suffix = absolute(session->get_user().work_directory, suffix);
    if (!session->get_user().is_allowed(suffix)) {
        session->write("500 no permission to do it.\r\n");
        return;
    }
    FTPEntity entity(suffix.c_str());
    if (entity.open() != 0) {
        session->write("500 file not exists.\r\n");
        return;
    }
    string message = "214 ";
    message += to_string(entity.get_size());
    message += "\r\n";
    session->write(message.c_str());
}

OPERATOR_DEFINE(PASS) {
    FTPUser &user = session->get_user();
    if (user.is_system()) {
        char *encrypted =
            ::crypt(suffix.c_str(), user.salt.empty() ? user.password.c_str()
                                                      : user.salt.c_str());
        if (encrypted == NULL) {
            goto failed;
        }
        if (!strncmp(encrypted, user.password.c_str(),
                     min(strlen(encrypted), strlen(user.password.c_str())))) {
            goto failed;
        }
    }
    if (suffix != user.password) {
        goto failed;
    }
    session->write("230 User logged in\r\n");
    return;
failed:
    session->write("530 Login authentication failed\r\n");
}

OPERATOR_DEFINE(PWD) {
    FTPUser &user = session->get_user();
    char buffer[512];
    memset(buffer, 0, sizeof(buffer));
    ACE_OS::strcat(buffer, "257 \"");
    ACE_OS::strcat(buffer, user.work_directory.c_str());
    ACE_OS::strcat(buffer, "\" is current directory\r\n");
    session->write(buffer);
}

OPERATOR_DEFINE(TYPE) {
    if (suffix == "I") {
        session->set_transfer_type(FTPSession::TransferType::ASCII);
        session->write("200 Command OK\r\n");
    } else if (suffix == "A") {
        session->set_transfer_type(FTPSession::TransferType::BINARY);
        session->write("200 Command OK\r\n");
    } else {
        session->write("500 Unrecognized command\r\n");
    }
}

OPERATOR_DEFINE(CWD) {
    suffix = absolute(session->get_user().work_directory, suffix);
    if (!session->get_user().is_allowed(suffix)) {
        session->write("500 No permission to do it.\r\n");
        return;
    }
    session->get_user().work_directory = suffix;
    session->write("200 Command OK\r\n");
}

OPERATOR_DEFINE(RETR) {
    ACE_Mem_Map mmap;
    suffix = absolute(session->get_user().work_directory, suffix);
    if (!session->get_user().is_allowed(suffix)) {
        session->write("500 no permission to do it.\r\n");
        return;
    }
    session->write("150 Opening ASCII mode data connection for file list\r\n");
    if (mmap.map(suffix.c_str()) != 0) {
        ACE_ERROR((LM_ERROR, "ACE_Mem_Map open failed errno=%d msg='%m'.\n"));
        session->write("451 Read File Failed.\r\n");
        return;
    }
    session->get_data_peer().send(mmap.addr(), mmap.size());
    session->get_data_peer().close();
    mmap.close();
    session->write("226 Transfer complete.\r\n");
}

OPERATOR_DEFINE(STOR) {
    char buffer[512];
    size_t read_size = 0, total_size = 0;
    suffix = absolute(session->get_user().work_directory, suffix);
    if (!session->get_user().is_allowed(suffix)) {
        session->write("500 no permission to do it.\r\n");
        return;
    }

    ACE_SOCK_Stream peer = session->get_data_peer();
    ACE_FILE_Connector connector;
    ACE_FILE_IO file;
    ACE_FILE_Addr file_addr(suffix.c_str());

    if (connector.connect(file, file_addr) != 0) {
        ACE_ERROR((LM_ERROR, "open file %s failed errno=%d msg='%m'.\n",
                   suffix.c_str()));
        session->write("451 Open file failed.\r\n");
        return;
    }
    memset(buffer, 0, sizeof(buffer));
    session->write("150 Open Data Connection for file.\r\n");
    while ((read_size = peer.recv(buffer, sizeof(buffer))) > 0) {
        total_size += read_size;
        file.send(buffer, read_size);
        memset(buffer, 0, sizeof(buffer));
    }
    file.close();
    size_t dir_len = 0;
    cwk_path_get_dirname(suffix.c_str(), &dir_len);
    session->close_dir(string(&suffix[0], &suffix[dir_len - 1]));
    session->write("226 Transfer complete.\r\n");
}

OPERATOR_DEFINE(MKD) {
    string absolute_path = session->get_user().work_directory;

    if (!session->get_user().is_allowed(suffix)) {
        session->write("500 no permission to do it.\r\n");
        return;
    }

    if (absolute_path[absolute_path.length() - 1] != '/') {
        absolute_path += ACE_DIRECTORY_SEPARATOR_CHAR;
    }
    absolute_path += suffix;
    ACE_OS::mkdir(absolute_path.c_str());
    string response = "257 \"";
    response += absolute_path + "\" OK\r\n";
    session->write(response.c_str());
}

OPERATOR_DEFINE(SYST) { session->write("215 UNIX Type: L8:\r\n"); }

OPERATOR_DEFINE(FEAT) {
    session->write(
        "211-Extensions supported:\r\n"
        "MLST\r\n"
        "MLSD\r\n"
        "SIZE\r\n"
        "COMPRESSION\r\n"
        "UTF8\r\n"
        "EPSV\r\n"
        "MDTM\r\n"
        "211 END\r\n");
}

OPERATOR_DEFINE(MDTM) {
    suffix = absolute(session->get_user().work_directory, suffix);
    FTPEntity entity(suffix.c_str());
    if (!session->get_user().is_allowed(suffix)) {
        session->write("500 no permission to do it.\r\n");
        return;
    }

    if (entity.open() != 0) {
        session->write("550 Open file Error.\r\n");
        return;
    }
    string response = "213 " + entity.get_last_modify_time_val_desc() + "\r\n";
    session->write(response.c_str());
}

OPERATOR_DEFINE(OPTS) { session->write("220 Command OK.\r\n"); }

OPERATOR_DEFINE(MLST) {}

OPERATOR_DEFINE(DELE) {
    suffix = absolute(session->get_user().work_directory, suffix);

    if (!session->get_user().is_allowed(suffix)) {
        session->write("500 no permission to do it.\r\n");
        return;
    }

    if (remove(suffix.c_str()) != 0) {
        session->write("500 delete failed.\r\n");
    } else {
        session->close_dir(suffix);
        session->write("250 Command ok.\r\n");
    }
}

OPERATOR_DEFINE(QUIT) {
    session->get_peer().close_reader();
    session->write("221 Goodbye.\r\n");
}

OPERATOR_DEFINE(RNFR) {
    if (cwk_path_is_relative(suffix.c_str())) {
        suffix = absolute(session->get_user().work_directory, suffix);
    }
    if (!session->get_user().is_allowed(suffix)) {
        session->write("500 no permission to do it.\r\n");
        return;
    }

    session->set_rename_from(suffix);
    session->write("250 OK prepare rename to.\r\n");
}

OPERATOR_DEFINE(RNTO) {
    char buffer[512];
    if (session->get_rename_from().empty()) {
        session->write("500 RNTO must start by RNFR.\r\n");
        return;
    }
    if (cwk_path_is_relative(suffix.c_str())) {
        memset(buffer, 0, sizeof(buffer));
        string filename;
        struct cwk_segment segment;
        if (!cwk_path_get_last_segment(suffix.c_str(), &segment)) {
            session->write("500 Invalid dest path.\r\n");
            return;
        }
        filename = segment.begin;
        suffix = absolute(suffix, session->get_rename_from());
        size_t dirname_len;
        cwk_path_get_dirname(suffix.c_str(), &dirname_len);
        memset(buffer, 0, sizeof(buffer));
        suffix = string(&suffix[0], &suffix[dirname_len - 1]);
        cwk_path_join(suffix.c_str(), filename.c_str(), buffer, sizeof(buffer));
        suffix = buffer;
    }
    if (rename(session->get_rename_from().c_str(), suffix.c_str()) != 0) {
        session->write("500 Rename Failed.\r\n");
        return;
    }
    session->write("250 Command OK.\r\n");
}

FTP_OPERATOR_DECLARE(SITE) {
    regex command_regex(R"((\S+)\s+(\S+)\s+(.+)$)");
    smatch match;
    string command, value, path;
    if (!regex_match(suffix, match, command_regex)) {
        goto failed;
    }
    command = match[1];
    value = match[2];
    path = match[3];
    if (command == "CHMOD") {
        path = absolute(session->get_user().work_directory, path);
        if (chmod(path.c_str(), strtol(value.c_str(), NULL, 8)) != 0) {
            session->write("501 change file permission failed.\r\n");
            return;
        }
        session->write("200 OK command finished.\r\n");
        session->close_dir(suffix.c_str());
        return;
    }
failed:
    ACE_DEBUG((LM_DEBUG, "unknown SITE %s TYPE %s VALUE %s PATH %s.\n",
               suffix.c_str(), command.c_str(), value.c_str(), path.c_str()));
    session->write("500 I don't known what to do.\r\n");
}

FTP_OPERATOR_DECLARE(NOOP) { session->write("200 Wait for you.\r\n"); }

FTP_OPERATOR_DECLARE(MODE) { session->write("500 I Don't Known.\r\n"); }

FTP_OPERATOR_DECLARE(EPSV) { passive(session, socket, suffix, true); }

FTP_OPERATOR_DECLARE(PORT) {
    regex format(R"((\d+,\d+,\d+,\d+),(\d+),(\d+))");
    smatch match;
    if (!regex_match(suffix, match, format)) {
        session->write("501 invalid ip:port.\r\n");
        return;
    }
    string ip = regex_replace(match[1].str(), regex(R"(,)"), ".");

    uint16_t port =
        static_cast<uint16_t>(strtol(match[2].str().c_str(), NULL, 10) * 256) +
        static_cast<uint16_t>(strtol(match[3].str().c_str(), NULL, 10));
    ACE_DEBUG((LM_DEBUG, "PORT dest address is %s:%d.\n", ip.c_str(), port));
    ACE_SOCK_Connector connector;
    ACE_INET_Addr remote(port, ip.c_str());
    if (connector.connect(session->get_data_peer(), remote) != 0) {
        ACE_ERROR((LM_ERROR,
                   "PORT connect to remote host failed errno=%d msg='%m'.\n",
                   ACE_OS::last_error()));
        session->write("500 dest unreachable.\r\n");
        return;
    }
    session->write("200 command ok.\r\n");
}
