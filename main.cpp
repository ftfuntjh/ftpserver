#include <ace/Proactor.h>
#include <cxxopts.hpp>
#include "src/FTPServer.h"
#include "src/FTPVirtualUser.h"

using namespace std;
using namespace cxxopts;

#define USAGE \
    "                                                                           \n \
###### ##### #####   ####  ###### #####  #    # ###### #####                  \n \
#        #   #    # #      #      #    # #    # #      #    #                 \n \
#####    #   #    #  ####  #####  #    # #    # #####  #    #                 \n \
#        #   #####       # #      #####  #    # #      #####                  \n \
#        #   #      #    # #      #   #   #  #  #      #   #                  \n \
#        #   #       ####  ###### #    #   ##   ###### #    #                 \n \
                                                                              \n \
Usage ftpserver [-p port] [-a ip] [-v] [-d]                                   \n \
                                                                              \n \
Options                                                                       \n \
                                                                              \n \
  -a,--port set the listen address.default  127.0.0.1                         \n \
  -d,--debug debug mode                                                       \n \
  -p,--port set the listen port. default 10085                                \n \
  -u,--virtual set the virtual user configuration path                        \n \
  -r,--real_ip set the bind ip address                                        \n \
  -v,--verbose verbose output                                                    \
"

// todo make the process becomes a daemon process in windows platform

#ifdef UNIX
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>

inline void daemon_process() {
    pid_t pid = fork();
    if (pid > 0) {
        exit(0);
    }
    if (pid < 0) {
        exit(1);
    }

    if (setsid() < 0) {
        exit(-1);
    }

    pid = fork();
    if (pid < 0) {
        exit(1);
    }

    if (pid > 0) {
        exit(0);
    }

    umask(0);

    chdir("/");

    int x;

    for (x = sysconf(_SC_OPEN_MAX); x >= 0; x--) close(x);

    openlog("ftpserver", LOG_PID, LOG_DAEMON);
}
#endif
extern string ipaddr;
int main(int argc, char *argv[], char *env[]) {
    Options options("ftpserver", "a simple ftp protoctol implement.");
    options.add_options()(
        "u,virtual", "the virtual user configuration file path",
        cxxopts::value<string>()->default_value("./virtual_user.txt"))(
        "d,debug", "Enable debugging")(
        "r,real_ip", "get the passive mode server ip",
        cxxopts::value<string>()->default_value("0.0.0.0"))(
        "h,help", "Usage ftpserver [-p port] [-a ip] [-v] [-d] [-r] [-t]")(
        "t,threads", "set the work threads count",
        cxxopts::value<int>()->default_value("4"))(
        "a,ip", "Listen Address",
        cxxopts::value<string>()->default_value("127.0.0.1"))(
        "p,port", "Listen Port", cxxopts::value<int>()->default_value("10086"))(
        "v,verbose", "Verbose output",
        cxxopts::value<bool>()->default_value("false"));
    string ip;
    int port;
    int thread_count;
    try {
        ParseResult result = options.parse(argc, argv);
        ip = result["ip"].as<string>();
        port = result["port"].as<int>();
        thread_count = result["threads"].as<int>();
        string config_path = result["virtual"].as<string>();
        set_ipaddr(result["real_ip"].as<string>());
        FTPVirtualUser::set_config_path(config_path);
        if (result.count("help")) {
            cout << USAGE << endl;
            return 0;
        }
    } catch (const OptionException &e) {
        cerr << USAGE << endl;
        return 0;
    }

    FTPServer ftpserver(ip, port, thread_count);
#ifdef UNIX
    daemon_process();
#endif
    ftpserver.open();
    ACE_Reactor::run_event_loop();
    return 0;
}
