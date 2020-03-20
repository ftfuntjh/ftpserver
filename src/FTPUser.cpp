//
// Created by ftfunjth on 2020/3/14.
//
#include "FTPUser.h"
#include "FTPVirtualUser.h"
#include "ace/Log_Msg.h"
#include "ace/OS.h"
#include "cwalk.h"
#include <pwd.h>
#include <shadow.h>

using namespace std;
FTPUser::FTPUser() : type(UNKNOWN) {}

FTPUser::FTPUser(string username, UserType type)
    : type(type), username(username) {}

FTPUser::operator bool() {
  if (username.empty())
    return false;
  if (type != UserType::VIRTUAL) {
    passwd *user = getpwnam(username.c_str());
    if (user) {
      spwd *sp;
      sp = getspnam(user->pw_name);
      endspent();
      this->type = SYSTEM;
      this->username = string(user->pw_name);
      this->work_directory = string(user->pw_dir);
      if (sp) {
        if (ACE_OS::last_error() != 0)
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("get user shadow failed errno=%d msg='%m'.\n")));
        else
          this->salt = sp->sp_pwdp;
      }
      this->password = user->pw_passwd;
      return true;
    }
  }

  shared_ptr<FTPVirtualUser> ftp_virtual_users = FTPVirtualUser::get_instance();
  if (ftp_virtual_users->exist(username)) {
    FTPVirtualUser::User user = ftp_virtual_users->get_user(username);
    this->type = UserType::VIRTUAL;
    this->username = user.username;
    this->password = user.password;
    this->work_directory = user.work_directory;
    this->allowed_directory = user.allowed_dir;
    return true;
  }

  return false;
}
bool FTPUser::is_allowed(const string &path) {
  size_t command_size =
      cwk_path_get_intersection(allowed_directory.c_str(), path.c_str());
  if (command_size < allowed_directory.length()) {
    command_size =
        cwk_path_get_intersection(work_directory.c_str(), path.c_str());
    if (command_size < work_directory.length())
      return false;
  }
  return true;
}
