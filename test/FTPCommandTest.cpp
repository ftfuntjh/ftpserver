//
// Created by ftfunjth on 2020/1/20.
//

#include <ace/INET_Addr.h>
#include <ace/SOCK_Connector.h>
#include <ace/SOCK_Stream.h>
#include <cmath>
#include <gtest/gtest.h>
#include <string>
#include <vector>

#define DELIMITER "\r\n"
#define USERCOMMAND "USER root\r\n"
#define PASSTIVECOMMAND "PASV\r\n"
#define PWDCOMMAND "PWD\r\n"
#define PASSWORD "PASS 6mQZOjo7nudgsbWk\r\n"
#define LISTCOMMAND "LIST\r\n"
#define TYPEI "TYPE I\r\n"
using std::string;
using std::vector;

string parse(const std::string &raw_str) {
  vector<string> command_list{};
  auto start = 0U;
  auto end = raw_str.find(DELIMITER);
  while (start < raw_str.length() && end < raw_str.length() &&
         end != string::npos) {
    command_list.push_back(raw_str.substr(start, end - start));
    start = end + 2;
    end = raw_str.find(DELIMITER);
  }
  for (auto &command : command_list) {
    auto command_prefix = command.substr(0, 4);
    if (command_prefix == "USER") {
    } else if (command_prefix == "PASS") {
      return command.substr(5);
    } else if (command_prefix == "RETR") {
      // TODO
    } else if (command_prefix == "PASV") {
    } else if (command_prefix == "PORT") {
      // TODO
    }
  }
  return string{};
}

class TestEnvironment : public ::testing::Environment {
public:
  void SetUp() override {
    remote_addr = ACE_INET_Addr{21, "106.13.186.71"};
    peer = ACE_SOCK_Stream{};
    connector = ACE_SOCK_Connector{};
    ASSERT_EQ(connector.connect(peer, remote_addr), 0);
  }

  static TestEnvironment *getEnv() { return env; }

  ACE_SOCK_Stream &getPeer() { return peer; }

  char *getBuffer() { return buffer; }

  ACE_SOCK_Connector &getConnector() { return connector; }

  static TestEnvironment *env;
  ACE_INET_Addr remote_addr;
  ACE_SOCK_Stream peer;
  ACE_SOCK_Connector connector;
  char buffer[2048];
};

class UserCommandTest : public ::testing::Test {};

TEST_F(UserCommandTest, CouldParseUserCommand) {
  auto password = parse("PASS c1996g9y21\r\n");
  ASSERT_EQ(password, "c1996g9y21");

  TestEnvironment::getEnv()->getPeer().recv(
      TestEnvironment::getEnv()->getBuffer(), 2048);
  ASSERT_EQ(std::string{TestEnvironment::getEnv()->getBuffer()},
            "220 (vsFTPd 3.0.3)\r\n");
  memset(TestEnvironment::getEnv()->getBuffer(), 0, 2048);

  ASSERT_EQ(TestEnvironment::getEnv()->getPeer().send(USERCOMMAND,
                                                      strlen(USERCOMMAND)),
            strlen(USERCOMMAND));
  TestEnvironment::getEnv()->getPeer().recv(
      TestEnvironment::getEnv()->getBuffer(), 2048);
  ASSERT_EQ(std::string{TestEnvironment::getEnv()->getBuffer()},
            "331 Please specify the password.\r\n");
  memset(TestEnvironment::getEnv()->getBuffer(), 0, 2048);

  ASSERT_EQ(
      TestEnvironment::getEnv()->getPeer().send(PASSWORD, strlen(PASSWORD)),
      strlen(PASSWORD));
  TestEnvironment::getEnv()->getPeer().recv(
      TestEnvironment::getEnv()->getBuffer(), 2048);
  ASSERT_EQ(std::string{TestEnvironment::getEnv()->getBuffer()},
            "230 Login successful.\r\n");
  memset(TestEnvironment::getEnv()->getBuffer(), 0, 2048);

  ASSERT_EQ(
      TestEnvironment::getEnv()->getPeer().send(PWDCOMMAND, strlen(PWDCOMMAND)),
      strlen(PWDCOMMAND));
  TestEnvironment::getEnv()->getPeer().recv(
      TestEnvironment::getEnv()->getBuffer(), 2048);
  ASSERT_EQ(std::string{TestEnvironment::getEnv()->getBuffer()},
            "257 \"/root\" is the current directory\r\n");
  memset(TestEnvironment::getEnv()->getBuffer(), 0, 2048);

  ASSERT_EQ(TestEnvironment::getEnv()->getPeer().send(TYPEI, strlen(TYPEI)),
            strlen(TYPEI));
  TestEnvironment::getEnv()->getPeer().recv(
      TestEnvironment::getEnv()->getBuffer(), 2048);
  memset(TestEnvironment::getEnv()->getBuffer(), 0, 2048);

  ASSERT_EQ(TestEnvironment::getEnv()->getPeer().send(PASSTIVECOMMAND,
                                                      strlen(PASSTIVECOMMAND)),
            strlen(PASSTIVECOMMAND));
  TestEnvironment::getEnv()->getPeer().recv(
      TestEnvironment::getEnv()->getBuffer(), 2048);
  std::string address{TestEnvironment::getEnv()->getBuffer()};
  std::cout << std::endl << address << std::endl;
  auto end = address.find_last_of(',');
  auto start = address.find_last_of(',', end - 1);
  auto ip_port = address.substr(start + 1, address.length() - start - 1);
  auto first = ip_port.substr(0, ip_port.find_first_of(','));
  auto last = ip_port.substr(ip_port.find_last_of(',') + 1);
  long first_val = std::strtol(first.c_str(), nullptr, 10) * 256;
  long second_val = std::strtol(last.c_str(), nullptr, 10);

  memset(TestEnvironment::getEnv()->getBuffer(), 0, 2048);

  ACE_SOCK_Connector connector;
  ACE_SOCK_Stream data_peer;
  connector.connect(data_peer,
                    ACE_INET_Addr{static_cast<u_short>(first_val + second_val),
                                  "106.13.186.71"});

  ASSERT_EQ(TestEnvironment::getEnv()->getPeer().send(LISTCOMMAND,
                                                      strlen(LISTCOMMAND)),
            strlen(LISTCOMMAND));
  TestEnvironment::getEnv()->getPeer().recv(
      TestEnvironment::getEnv()->getBuffer(), 2048);
  string v = string{TestEnvironment::getEnv()->getBuffer()};
  std::cout << v << std::endl;
  memset(TestEnvironment::getEnv()->getBuffer(), 0, 2048);

  data_peer.recv(TestEnvironment::getEnv()->getBuffer(), 2048);
  auto result = string{TestEnvironment::getEnv()->getBuffer()};
  std::cout << result << std::endl;
  memset(TestEnvironment::getEnv()->getBuffer(), 0, 2048);
  TestEnvironment::getEnv()->getPeer().recv(
      TestEnvironment::getEnv()->getBuffer(), 2048);
  std::cout << string{TestEnvironment::getEnv()->getBuffer()};
}

TestEnvironment *TestEnvironment::env = dynamic_cast<TestEnvironment *>(
    testing::AddGlobalTestEnvironment(new TestEnvironment));

int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
