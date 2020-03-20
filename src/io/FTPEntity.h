//
// Created by ftfunjth on 2020/3/14.
//

#ifndef FTPSERVER_FTPENTITY_H
#define FTPSERVER_FTPENTITY_H

#include "ace/FILE.h"
#include "ace/FILE_Connector.h"
#include "ace/FILE_IO.h"
#include <memory>
#include <set>
#include <string>

class FTPEntity;

template <typename Tp>
struct FTPEntityComp : public std::binary_function<Tp, Tp, bool> {

  bool operator()(const Tp &x, const Tp &y) const {
    if (x->get_path() == ".." && y->get_path() != ".")
      return true;
    else if (x->get_path() == ".")
      return true;
    else if (y->get_path() == ".")
      return false;
    else
      return x->get_path() < y->get_path();
  }
};
class FTPEntity {
public:
  enum EntityType {
    UNKNOWN,
    DIRENT,
    FILE,
    FIFO,
    SOCKET,
    LINK,
    C_DEVICE,
    B_DEVICE
  };

public:
  FTPEntity() = delete;

  explicit FTPEntity(const char *path, EntityType type = EntityType::UNKNOWN);

  virtual ~FTPEntity() = default;

  int open();

  inline std::string get_path() { return path; }
  inline EntityType get_type() { return type; }

  inline bool is_dirent() const { return type == EntityType::DIRENT; }

  inline bool is_file() const { return type == EntityType::FILE; }

  inline bool is_exist() const { return exist; }

  int64_t get_size();

  std::string get_user_name();

  std::string get_group_name();

  std::string get_permission_desc();

  std::string get_last_modify_time_desc();

  std::string get_last_modify_time_val_desc();

  std::string ls();

  std::string mlsd() const;

public:
  FTPEntity(const FTPEntity &) = delete;
  FTPEntity &operator=(const FTPEntity &) = delete;

private:
  EntityType type;
  std::string path;
  int uid;
  int gid;
  int64_t size;
  std::string file_name;
  std::string last_modify_time;
  std::string last_modify_time_val;
  std::string own_username;
  std::string group_name;
  std::string permission;
  std::set<std::shared_ptr<FTPEntity>,
           FTPEntityComp<std::shared_ptr<FTPEntity>>>
      files;
  bool exist;
};

#endif // FTPSERVER_FTPENTITY_H
