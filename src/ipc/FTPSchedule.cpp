#include "FTPSchedule.h"
#include <memory>
#include <regex>
#include "FTPRequest.h"
#include "ace/OS.h"

using namespace std;

FTPSchedule::FTPSchedule() {}

int FTPSchedule::enqueue(ACE_Method_Request *request) {
    ACE_DEBUG((LM_DEBUG, "FTPSchedule [%t] enqueue.\n"));
    return this->activation_queue.enqueue(request);
}

int FTPSchedule::svc() {
    ACE_DEBUG((LM_DEBUG, "FTPSchedule [%t] start working.\n"));
    while (true) {
        auto_ptr<ACE_Method_Request> request(activation_queue.dequeue());
        if (request->call() == -1) {
            ACE_ERROR((LM_ERROR, "FTPSchedule [%t] call request failed.\n"));
            continue;
        }
    }
    return 0;
}
