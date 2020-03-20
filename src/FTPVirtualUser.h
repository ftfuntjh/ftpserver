//
// Created by ftfunjth on 2020/3/14.
//

#ifndef FTPSERVER_FTPVIRTUALUSER_H
#define FTPSERVER_FTPVIRTUALUSER_H

#include <map>
#include <memory>
#include <string>
class FTPVirtualUser {
public:
  struct User {
    std::string username;
    std::string password;
    std::string work_directory;
    std::string allowed_dir;
  };
  static std::shared_ptr<FTPVirtualUser> get_instance();

  FTPVirtualUser::User get_user(const std::string &username);

  bool exist(const std::string &username);

  static void set_config_path(std::string config_path);

private:
  void load_users(const std::string &config_path);

  FTPVirtualUser(const std::string &config_path);

  static std::shared_ptr<FTPVirtualUser> instance;
  static std::string config_path;
  std::map<std::string, User> users;
};

#endif // FTPSERVER_FTPVIRTUALUSER_H
