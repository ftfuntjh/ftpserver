//
// Created by ftfunjth on 2020/3/14.
//

#ifndef FTPSERVER_FTPUSER_H
#define FTPSERVER_FTPUSER_H

#include <string>
struct FTPUser {
  enum UserType { SYSTEM, VIRTUAL, UNKNOWN };
  FTPUser();
  explicit FTPUser(std::string username, UserType type = UNKNOWN);

  inline bool is_system() { return type == SYSTEM; };
  inline bool is_virtual() { return type == VIRTUAL; }
  explicit operator bool();

  bool is_allowed(const std::string &path);
  std::string username;
  std::string password;
  std::string work_directory;
  std::string allowed_directory;
  std::string salt;
  UserType type;
};

#endif // FTPSERVER_FTPUSER_H
