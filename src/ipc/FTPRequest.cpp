#include "FTPRequest.h"
#include <map>
#include "../FTPOperator.h"
#include "ace/Guard_T.h"
#include "ace/Thread_Mutex.h"

using namespace std;
extern map<string, command_operator> operator_map;

FTPRequest::FTPRequest(FTPSession& session, string method, string arguments)
    : session(session), method(method), arguments(arguments) {}

int FTPRequest::call() {
    ACE_Guard<ACE_Thread_Mutex> guard(session.get_mutex());
    map<string, command_operator>::iterator ent = operator_map.find(method);
    ent->second(&session, session.get_peer(), arguments);
    ret.set(0);
    return 0;
}
