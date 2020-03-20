//
// Created by ftfunjth on 2020/3/14.
//

#include "FTPEntity.h"
#include "ace/Dirent.h"
#include "ace/FILE.h"
#include "ace/FILE_Connector.h"
#include "ace/FILE_IO.h"
#include "ace/Log_Msg.h"
#include "ace/OS.h"
#include "cwalk.h"
#include <cstring>
#include <grp.h>
#include <pwd.h>
#include <regex>
#include <sstream>

#define S_RIGHT(str, type, sub_type, value, mode)                              \
  do {                                                                         \
    if ((S_I##type##sub_type) & (mode))                                        \
      (str) += (value);                                                        \
    else                                                                       \
      (str) += ("-");                                                          \
  } while (0)

#define S_GROUP(str, type, mode)                                               \
  S_RIGHT(str, R, type, "r", mode);                                            \
  S_RIGHT(str, W, type, "w", mode);                                            \
  S_RIGHT(str, X, type, "x", mode)

#define FILE_PERMISSION(str, mode)                                             \
  S_GROUP(str, USR, mode);                                                     \
  S_GROUP(str, GRP, mode);                                                     \
  S_GROUP(str, OTH, mode)

using EntityType = FTPEntity::EntityType;
using namespace std;

static string timespec2string(struct timespec *ts) {
  static char buffer[36];
  int ret, len = sizeof(buffer);

  struct tm t;
  string result;
  tzset();
  if (localtime_r(&(ts->tv_sec), &t) == NULL)
    return result;

  ret = strftime(buffer, sizeof(buffer), "%F %T", &t);
  if (ret == 0)
    len -= ret - 1;

  ret = snprintf(&buffer[strlen(buffer)], len, ".%09ld", ts->tv_nsec);
  if (ret >= len)
    ACE_ERROR((LM_ERROR, "buffer size is too small,need at least %d.\n", ret));

  result = buffer;
  return buffer;
}

static string time_val_of(const string &time) {
  stringstream ss;
  smatch match;
  regex daytime(R"(^(\d{4})-(\d{2})-(\d{2})\s*(\d{2}):(\d{2}):(\d{2})\.\d+)");
  if (regex_match(time, match, daytime)) {
    /* See https://files.stairways.com/other/ftp-list-specs-info.txt for
     * details. Used date format is: Date(MMM DD hh:mm) Dec 14 11:22
     * */
    auto year = match[1].str();
    auto month = match[2].str();
    auto day = match[3].str();
    auto hour = match[4].str();
    auto minus = match[5].str();
    auto seconds = match[6].str();
    ss << year << month << day << hour << minus << seconds;
  }
  return ss.str();
}
static string time_of(const string &time) {
  static char months[][4] = {"Jan", "Feb", "Mar", "Apr", "Jun", "Jul",
                             "Aug", "Sep", "Oct", "Nov", "Dec"};
  stringstream ss;
  smatch match;
  regex daytime(R"(^(\d{4})-(\d{2})-(\d{2})\s*(\d{2}):(\d{2}):(\d{2})\.\d+)");
  if (regex_match(time, match, daytime)) {
    /* See https://files.stairways.com/other/ftp-list-specs-info.txt for
     * details. Used date format is: Date(MMM DD hh:mm) Dec 14 11:22
     * */
    auto year = match[1].str();
    auto month = match[2].str();
    auto day = match[3].str();
    auto hour = match[4].str();
    auto minus = match[5].str();

    ss << months[stoi(month) - 1] << " " << day << " " << hour << ":" << minus;
  }
  return ss.str();
}

EntityType type_of(ACE_stat *stat) {
  EntityType type(EntityType::UNKNOWN);
  if (S_ISDIR(stat->st_mode))
    type = EntityType::DIRENT;
  else if (S_ISBLK(stat->st_mode))
    type = EntityType::B_DEVICE;
  else if (S_ISFIFO(stat->st_mode))
    type = EntityType::FIFO;
  else if (S_ISCHR(stat->st_mode))
    type = EntityType::C_DEVICE;
  else if (S_ISLNK(stat->st_mode))
    type = EntityType::LINK;
  else if (S_ISREG(stat->st_mode))
    type = EntityType::FILE;
  return type;
}

static int fstat_of(const string &path, ACE_stat *stat) {
  if (path.empty())
    ACE_ERROR_RETURN((LM_DEBUG, ACE_TEXT("can't open empty dir.\n")), -1);

  ACE_HANDLE handle = ACE_OS::open(path.c_str(), O_RDONLY);
  if (handle == ACE_INVALID_HANDLE)
    ACE_ERROR_RETURN((LM_ERROR, "open %s failed errno %d'%m'.\n",
                      ACE_TEXT(path.c_str()), ACE_OS::last_error()),
                     -1);

  if (ACE_OS::fstat(handle, stat) != 0) {
    ACE_OS::close(handle);
    ACE_ERROR_RETURN((LM_ERROR, "fstat %s failed  errno %d'%m'.\n",
                      ACE_TEXT(path.c_str()), ACE_OS::last_error()),
                     -1);
  }
  if (ACE_OS::close(handle) != 0)
    ACE_ERROR_RETURN((LM_ERROR, "close %s failed errno %d'%m'.\n", path.c_str(),
                      ACE_TEXT(path.c_str()), ACE_OS::last_error()),
                     -1);
  return 0;
}

static string username_of(int user_id) {
  static char buffer[512];
  memset(buffer, 0, sizeof(buffer));
  string result;
  struct passwd pwd;
  struct passwd *pwd_result;
  getpwuid_r(user_id, &pwd, buffer, sizeof(buffer), &pwd_result);
  if (pwd_result != nullptr) {
    result = string(pwd.pw_name);
  }
  return result;
}

static string group_name_of(int group_id) {
  static char buffer[512];
  memset(buffer, 0, sizeof(buffer));
  string result;
  struct group grp;
  struct group *grp_result;
  getgrgid_r(group_id, &grp, buffer, sizeof(buffer), &grp_result);
  if (grp_result != nullptr) {
    result = string(grp.gr_name);
  }
  return result;
}

FTPEntity::FTPEntity(const char *path, EntityType type)
    : type(type), uid(-1), gid(-1), size(-1), path(path), exist(false) {}

int64_t FTPEntity::get_size() { return size; }

string FTPEntity::get_user_name() { return own_username; }

string FTPEntity::get_group_name() { return group_name; }

string FTPEntity::get_permission_desc() { return permission; }

string FTPEntity::get_last_modify_time_desc() { return last_modify_time; }

int FTPEntity::open() {
  static char buffer[512];

  memset(buffer, 0, sizeof(buffer));
  ACE_OS::getcwd(buffer, sizeof(buffer));
  string pwd = buffer;

  if (path.empty())
    path = pwd;

  if (cwk_path_is_relative(path.c_str())) {
    memset(buffer, 0, sizeof(buffer));
    cwk_path_get_absolute(pwd.c_str(), path.c_str(), buffer, sizeof(buffer));
    path = buffer;
  }

  ACE_stat stat;
  if (fstat_of(path, &stat) != 0) {
    return -1;
  }

  type = type_of(&stat);
  own_username = username_of(stat.st_uid);
  group_name = group_name_of(stat.st_gid);
  FILE_PERMISSION(permission, stat.st_mode);
  last_modify_time = time_of(timespec2string(&stat.st_atim));
  size = stat.st_size;
  if (is_dirent()) {
    if (path.find_last_of('/') != path.length() - 1) {
      path += '/';
    }
    ACE_DIRENT *ent = NULL;
    ACE_Dirent dir(path.c_str());
    ent = dir.read();
    while (ent) {
      memset(buffer, 0, sizeof(buffer));
      strncat(buffer, path.c_str(), sizeof(buffer) - strlen(buffer) - 1);
      strncat(buffer, ent->d_name, sizeof(buffer) - strlen(ent->d_name) - 1);
      shared_ptr<FTPEntity> entity = make_shared<FTPEntity>(buffer);
      entity->file_name = ent->d_name;
      ACE_stat ent_stat;
      fstat_of(buffer, &ent_stat);
      entity->type = type_of(&ent_stat);
      entity->own_username = username_of(ent_stat.st_uid);
      entity->group_name = group_name_of(ent_stat.st_gid);
      FILE_PERMISSION(entity->permission, ent_stat.st_mode);
      entity->last_modify_time = time_of(timespec2string(&ent_stat.st_mtim));
      entity->last_modify_time_val =
          time_val_of(timespec2string(&ent_stat.st_mtim));
      entity->size = ent_stat.st_size;
      files.insert(entity);
      ent = dir.read();
    }
    dir.close();
  }
  exist = true;
  return 0;
}
string FTPEntity::ls() {
  static char buffer[512];
  stringstream ss;
  for (auto &file : files) {
    memset(buffer, 0, sizeof(buffer));
    snprintf(buffer, sizeof(buffer), "%c%s   1 %-10s %10lu %s %s\r\n",
             file->type == EntityType::DIRENT ? 'd' : '-',
             file->permission.c_str(), file->own_username.c_str(), file->size,
             file->last_modify_time.c_str(), file->file_name.c_str());
    ss << buffer;
  }
  return ss.str();
}

string FTPEntity::mlsd() const {
  stringstream ss;
  for (const shared_ptr<FTPEntity> &file : files) {
    string type, name, modify;
    switch (file->type) {
    case EntityType::DIRENT:
      type = "dir";
      break;
    default:
      type = "file";
      break;
    }
    ss << "type=" << type << ";Size=" << to_string(file->size)
       << ";Modify=" << file->last_modify_time_val << "; " << file->file_name
       << "\r\n";
  }
  return ss.str();
}

string FTPEntity::get_last_modify_time_val_desc() {
  return last_modify_time_val;
}
