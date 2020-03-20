//
// Created by ftfunjth on 2020/3/14.
//

#include "FTPVirtualUser.h"
#include <cstring>
#include <fstream>
#include <sstream>
using namespace std;
string FTPVirtualUser::config_path = string("./virtual.txt");
shared_ptr<FTPVirtualUser> FTPVirtualUser::instance = NULL;

FTPVirtualUser::User FTPVirtualUser::get_user(const string &username) {
  return users.find(username)->second;
}

FTPVirtualUser::FTPVirtualUser(const string &config_path) {
  load_users(config_path);
}

shared_ptr<FTPVirtualUser> FTPVirtualUser::get_instance() {
  if (instance == NULL) {
    instance = shared_ptr<FTPVirtualUser>(
        new FTPVirtualUser(FTPVirtualUser::config_path));
  }
  return instance;
}
void FTPVirtualUser::load_users(const string &config_path) {
  static char buffer[512];
  fstream fs;
  fs.open(config_path, ios::in);
  while (fs) {
    memset(buffer, 0, sizeof(buffer));
    fs.getline(buffer, sizeof(buffer));
    stringstream ss((string(buffer)));
    User user;
    ss >> user.username >> user.password >> user.work_directory >>
        user.allowed_dir;
    users.insert(make_pair(user.username, user));
  }
}
bool FTPVirtualUser::exist(const string &username) {
  return users.find(username) != users.end();
}

void FTPVirtualUser::set_config_path(std::string path) {
  FTPVirtualUser::config_path = path;
}
