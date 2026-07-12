#pragma once
#include "atlasnet/core/RPC/RPCMacros.hpp"
#include "atlasnet/core/RPC/RPCMessage.hpp"
#include "atlasnet/core/RPC/RPCSystem.hpp"
#include "atlasnet/core/RPC/new/RPCConcepts_N.hpp"
#include "atlasnet/core/address/SocketAddress.hpp"

#include "atlasnet/core/messages/MessageSystem.hpp"
#include "atlasnet/core/tasks/TaskSystem.hpp"

#include <condition_variable>
#include <gtest/gtest.h>
#include <iostream>
#include <mutex>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

int pick_available_port()
{
  int Min = 1024;
  int Max = 65535;
  if (Min > Max)
    std::swap(Min, Max);

  auto can_bind = [](int port, int sock_type) -> bool
  {
    int fd = ::socket(AF_INET, sock_type, 0);
    if (fd < 0)
      return false;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(static_cast<uint16_t>(port));

    const bool ok =
        (::bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0);
    ::close(fd);
    return ok;
  };

  for (int port = Min; port <= Max; ++port)
  {
    // Consider the port "available" only if both TCP and UDP can bind.
    if (can_bind(port, SOCK_STREAM) && can_bind(port, SOCK_DGRAM))
      return port;
  }

  return -1; // no free port in range
}
using namespace AtlasNet;
int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

ATLASNET_RPC(
    TESTRpc,
    // TestMethod(int,float) -> void
    ATLASNET_RPC_METHOD(TestMethod, ATLASNET_RPC_SIG(void(int, float)));
    // TestMethod_Ret() -> int
    ATLASNET_RPC_METHOD(TestMethod_Ret, ATLASNET_RPC_SIG(int()));
    // TestMethod_Ret_String(std::string_view) -> std::string
    ATLASNET_RPC_METHOD(TestMethod_Ret_String,
                        ATLASNET_RPC_SIG(std::string(std::string_view))););

ATLASNET_RPC(MyOtherRPC,
             // OtherMethod(std::string) -> void
             ATLASNET_RPC_METHOD(OtherMethod,
                                 ATLASNET_RPC_SIG(void(std::string)));
             // OtherMethod_Ret_iota(std::vector<int>) -> int
             ATLASNET_RPC_METHOD(OtherMethod_Ret_iota,
                                 ATLASNET_RPC_SIG(int(std::vector<int>))));

TEST(RPC, BaseMessage)
{
  TaskSystem taskSystem(TaskSystem::Config{});
  MessageSystem msgSystem(MessageSystem::Config{.taskSystem = &taskSystem});
  const PortType port = pick_available_port();
  std::mutex mutex;
  bool success = false;
  std::condition_variable cv;
  std::cout << std::format("Request Hash ID: {}", RpcRequestMessage::TypeIdHash)
            << std::endl;
  std::cout << std::format("Response Hash ID: {}",
                           RpcResponseMessage::TypeIdHash)
            << std::endl;
  msgSystem.OpenListenSocket(port)
      .On<RpcRequestMessage>(
          [&](const RpcRequestMessage& msg, const SocketAddress&)
          {
            RpcResponseMessage response{
                .callID = msg.callID,
                .payload = std::vector<uint8_t>{1, 2, 3, 4, 5},
            };
            auto messageHandle = msgSystem.QueueMessage(
                response, SocketAddress(IPv4(127, 0, 0, 1), port),
                MessageSendMode::eReliableBatched);
            EXPECT_EQ(messageHandle->get().code,
                      MessageSendResultCode::eSuccess);
          })
      .On<RpcResponseMessage>(
          [&](const RpcResponseMessage& msg, const SocketAddress&)
          {
            success = true;
            cv.notify_one();
          });

  RpcRequestMessage request{
      .methodId = 123,
      .callID = 456,
      .payload = std::vector<uint8_t>{10, 20, 30},
  };
  auto messageHandle =
      msgSystem.QueueMessage(request, SocketAddress(IPv4(127, 0, 0, 1), port),
                             MessageSendMode::eReliableBatched);
  EXPECT_EQ(messageHandle->get().code, MessageSendResultCode::eSuccess);

  std::unique_lock lock(mutex);
  cv.wait_for(lock, std::chrono::seconds(5), [&success] { return success; });
  EXPECT_TRUE(success);
}
TEST(RPC, SelfReceive)
{
  TaskSystem taskSystem(TaskSystem::Config{});
  const PortType port = pick_available_port();
  bool success = false;
  std::mutex mutex;
  std::condition_variable cv;
  MessageSystem msgSystem(MessageSystem::Config{.taskSystem = &taskSystem});

  RPCSystem rpc(RPCSystem::Config{.port = port, .messageSystem = &msgSystem});

  rpc.Bind<TESTRpc::TestMethod>(
      [&](int a, float b)
      {
        std::cout << "TestMethod called with a=" << a << " b=" << b
                  << std::endl;
        success = true;
        cv.notify_one();
      });

  rpc.Call<TESTRpc::TestMethod>(SocketAddress(IPv4(127, 0, 0, 1), port), 42,
                                3.14f);

  std::unique_lock lock(mutex);
  cv.wait_for(lock, std::chrono::seconds(5), [&success] { return success; });
  EXPECT_TRUE(success);
}
TEST(RPC, SelfReceiveAndReply)
{
  TaskSystem taskSystem(TaskSystem::Config{});

  MessageSystem msgSystem(MessageSystem::Config{.taskSystem = &taskSystem});
  const PortType port = pick_available_port();
  bool success = false;

  RPCSystem rpc(RPCSystem::Config{.port = port, .messageSystem = &msgSystem});

  rpc.Bind<TESTRpc::TestMethod_Ret_String>(
      [&](std::string_view str) -> std::string
      {
        std::cout << "TestMethod_Ret_String called with str=" << str
                  << std::endl;
        success = true;

        std::string strCopy(str);
        return strCopy + " world";
      });

  std::cout << std::format("Request Hash ID: {}", RpcRequestMessage::TypeIdHash)
            << std::endl;
  std::cout << std::format("Response Hash ID: {}",
                           RpcResponseMessage::TypeIdHash)
            << std::endl;

  std::future<std::string> result = rpc.Call<TESTRpc::TestMethod_Ret_String>(
      SocketAddress(IPv4(127, 0, 0, 1), port), "Hello");

  auto status = result.wait_for(std::chrono::seconds(5));
  if (status == std::future_status::ready)
  {
    std::string resultValue = result.get();
    EXPECT_EQ(resultValue, "Hello world");
  }
  else
  {
    FAIL() << "RPC call did not complete in time";
  }

  EXPECT_TRUE(success);
}
TEST(RPC, SelfReceiveWrongPort)
{
  TaskSystem taskSystem(TaskSystem::Config{});
  const PortType port = pick_available_port();

  MessageSystem msgSystem(MessageSystem::Config{.taskSystem = &taskSystem});
  RPCSystem rpc(RPCSystem::Config{.port = port, .messageSystem = &msgSystem});

  bool success = false;
  std::mutex mutex;
  std::condition_variable cv;
  rpc.Bind<TESTRpc::TestMethod>(
      [&](int a, float b)
      {
        std::cout << "TestMethod called with a=" << a << " b=" << b
                  << std::endl;
        success = true;
        cv.notify_one();
      });
  rpc.Call<TESTRpc::TestMethod>(SocketAddress(IPv4(127, 0, 0, 1), port + 1), 42,
                                3.14f);

  std::unique_lock lock(mutex);
  cv.wait_for(lock, std::chrono::seconds(5), [&success] { return success; });
  EXPECT_FALSE(success);
}
TEST(RPC, SelfReceiveAnyPort)
{
  TaskSystem taskSystem(TaskSystem::Config{});

  MessageSystem msgSystem(MessageSystem::Config{.taskSystem = &taskSystem});
  const PortType port = pick_available_port();
  msgSystem.OpenListenSocket(port);
  RPCSystem rpc(RPCSystem::Config{.messageSystem = &msgSystem});

  bool success = false;
  std::mutex mutex;
  std::condition_variable cv;
  rpc.Bind<TESTRpc::TestMethod>(
      [&](int a, float b)
      {
        std::cout << "TestMethod called with a=" << a << " b=" << b
                  << std::endl;
        success = true;
        cv.notify_one();
      });
  rpc.Call<TESTRpc::TestMethod>(SocketAddress(IPv4(127, 0, 0, 1), port), 42,
                                3.14f);

  std::unique_lock lock(mutex);
  cv.wait_for(lock, std::chrono::seconds(5), [&success] { return success; });
  EXPECT_TRUE(success);
}
TEST(RPC, ForkParentCallsChildAndGetsResult)
{
  const PortType parentPort = pick_available_port();
  const PortType childPort = pick_available_port();

  int readyPipe[2];
  ASSERT_EQ(pipe(readyPipe), 0) << "Failed to create pipe";

  const int expectedResult =
      std::chrono::system_clock::now().time_since_epoch().count() %
      10000; // Just some arbitrary value to return from child to parent
  pid_t pid = fork();
  ASSERT_GE(pid, 0) << "fork() failed";
  if (pid == 0)
  {
    // Child process: hosts RPC server on childPort.
    close(readyPipe[0]);

    TaskSystem childTaskSystem(TaskSystem::Config{});
    MessageSystem childMsgSystem(
        MessageSystem::Config{.taskSystem = &childTaskSystem});
    RPCSystem childRpc(
        RPCSystem::Config{.port = childPort, .messageSystem = &childMsgSystem});

    std::mutex mutex;
    std::condition_variable cv;
    bool handled = false;

    childRpc.Bind<TESTRpc::TestMethod_Ret>(
        [&]() -> int
        {
          {
            std::lock_guard<std::mutex> lock(mutex);
            handled = true;
            std::cerr << "Child received TestMethod_Ret call" << std::endl;
          }
          cv.notify_one();
          return expectedResult;
        });

    // Signal readiness to parent.
    const uint8_t ready = 1;
    (void)write(readyPipe[1], &ready, 1);
    close(readyPipe[1]);

    std::unique_lock<std::mutex> lock(mutex);
    const bool gotCall =
        cv.wait_for(lock, std::chrono::seconds(10), [&] { return handled; });
    std::this_thread::sleep_for(std::chrono::seconds(2));
    std::cerr << "Exiting child process" << std::endl;
    _exit(gotCall ? 0 : 2);
  }

  // Parent process: sets up its own RPC on parentPort, then calls child.
  close(readyPipe[1]);

  uint8_t ready = 0;
  ASSERT_EQ(read(readyPipe[0], &ready, 1), 1)
      << "Parent failed waiting for child readiness";
  close(readyPipe[0]);

  TaskSystem parentTaskSystem(TaskSystem::Config{});
  MessageSystem parentMsgSystem(
      MessageSystem::Config{.taskSystem = &parentTaskSystem});
  RPCSystem parentRpc(
      RPCSystem::Config{.port = parentPort, .messageSystem = &parentMsgSystem});

  std::future<int> result = parentRpc.Call<TESTRpc::TestMethod_Ret>(
      SocketAddress(IPv4(127, 0, 0, 1), childPort));

  auto status = result.wait_for(std::chrono::seconds(10));
  ASSERT_EQ(status, std::future_status::ready)
      << "RPC future not ready in time";
  EXPECT_EQ(result.get(), expectedResult);

  int childStatus = 0;
  ASSERT_EQ(waitpid(pid, &childStatus, 0), pid);
  ASSERT_TRUE(WIFEXITED(childStatus));
  EXPECT_EQ(WEXITSTATUS(childStatus), 0);
}

ATLASNET_RPC_NEW(NewStyleRPC, NewTestMethod, void, int, float);
ATLASNET_RPC_NEW(NewStyleRPC, NewTestMethod_Ret, int);
ATLASNET_RPC_NEW(NewStyleRPC, NewTestMethod_Ret_String, std::string,
                 std::string_view);
