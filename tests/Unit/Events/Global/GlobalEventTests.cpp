#include "atlasnet/core/Address.hpp"
#include "atlasnet/core/SocketAddress.hpp"
#include "atlasnet/core/database/redis/Redis.hpp"
#include "atlasnet/core/database/redis/RedisConn.hpp"
#include "atlasnet/core/events/GlobalEventSystem.hpp"
#include "atlasnet/core/job/JobSystem.hpp"
#include <condition_variable>
#include <future>
#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include <netinet/in.h>
#include <sys/socket.h>
using namespace AtlasNet;
int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
static PortType GetFreePort()
{
  int sock = ::socket(AF_INET, SOCK_STREAM, 0);

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = 0; // OS chooses

  if (bind(sock, (sockaddr*)&addr, sizeof(addr)) < 0)
  {
    throw std::runtime_error("Failed to bind ephemeral port");
  }

  socklen_t len = sizeof(addr);
  getsockname(sock, (sockaddr*)&addr, &len);

  ::close(sock);
  return ntohs(addr.sin_port);
}
// function that execution a system command and returns result code and output
std::pair<int, std::string> execCommand(const std::string& command)
{
  std::array<char, 128> buffer;
  std::string result;
  int returnCode = -1;

  // Open a pipe to the command
  FILE* pipe = popen(command.c_str(), "r");
  if (!pipe)
    throw std::runtime_error("popen() failed!");

  // Read the output of the command
  while (fgets(buffer.data(), buffer.size(), pipe) != nullptr)
    result += buffer.data();

  // Get the return code of the command
  returnCode = pclose(pipe);
  return {returnCode, result};
}
class EventGlobalSystemTest : public ::testing::Test
{
  PortType redisPort = GetFreePort(); // Default Redis port

  std::unique_ptr<Database::RedisConn> redisConn;
  std::jthread redisThread;

protected:
  void SetUp() override
  {

    redisThread = std::jthread(
        [this]
        {
          auto [returnCode, output] =
              execCommand("redis-server --port " + std::to_string(redisPort) +
                          " --appendonly no");
          // std::cerr << "Redis: " << output << std::endl;
          if (returnCode != 0)
          {
            std::cerr << "Failed to start Redis server: " << output
                      << std::endl;
            FAIL()
                << "Could not start Redis server for GlobalEventSystem tests.";
          }
        });
    // Setup code for GlobalEventSystem tests can go here.

    redisConn = Database::RedisConn::Connect(
        {.host = IPv4("127.0.0.1"),
         .port = redisPort,
         .Mode = Database::RedisConn::RedisMode::eStandalone,
         .MaxConnectRetries = 5});
    if (!redisConn)
    {
      std::cerr << "Failed to connect to Redis server on port " << redisPort
                << std::endl;
      FAIL()
          << "Could not connect to Redis server for GlobalEventSystem tests.";
    }
    redisConn->KeyVal().GetSet().Set("test_key", "test_value");
    if (auto val = redisConn->KeyVal().GetSet().Get("test_key"))
    {
      std::cerr << "Successfully connected to Redis server. test_key: " << *val
                << std::endl;
    }
    else
    {
      std::cerr << "Failed to set/get test key in Redis server." << std::endl;
      FAIL() << "Could not perform basic operations on Redis server for "
                "GlobalEventSystem tests.";
    }
  }
  void TearDown() override
  {
    auto [returnCode, output] =
        execCommand("redis-cli -p " + std::to_string(redisPort) + " shutdown");
    if (returnCode != 0)
    {
      std::cerr << "Failed to shutdown Redis server: " << output << std::endl;
      FAIL() << "Could not shutdown Redis server for GlobalEventSystem tests.";
    }
    std::cerr << "Redis shutdown: " << output << std::endl;

    redisThread.join();
    // Cleanup code for GlobalEventSystem tests can go here.
  }

public:
  Database::RedisConn& GetRedisConn()
  {
    return *redisConn;
  }
  PortType GetRedisPort() const
  {
    return redisPort;
  }
};
ATLASNET_EVENT(SimpleTestEvent, ATLASNET_EVENT_FIELD(int, value));
TEST_F(EventGlobalSystemTest, BasicEventEmission)
{
  // This test is a placeholder. Implementing a full test for the
  // GlobalEventSystem would require setting up a Redis instance and ensuring
  // that the event system can connect to it, which is beyond the scope of this
  // unit test.
  JobSystem jobSystem({});
  GlobalEventSystem eventSystem(
      {._redisConn = &GetRedisConn(), ._jobSystem = &jobSystem});

  std::atomic_bool callbackInvoked = false;
  int expectedvalue = 42;
  std::promise<int> callbackValue;
  eventSystem.On<SimpleTestEvent>(
      [&](const SimpleTestEvent& message)
      {
        // In a real test, we would parse the message and verify its contents.
        std::cerr << "Received event message: " << message.value << std::endl;
        callbackInvoked = true;
        callbackValue.set_value(message.value);
      });

  eventSystem.Emit(SimpleTestEvent(expectedvalue))
      .wait(); // Wait for the event to be processed
  std::future<int> futureValue = callbackValue.get_future();
  std::future_status status = futureValue.wait_for(
      std::chrono::seconds(5)); // Wait for the callback to set the value
  if (status != std::future_status::ready)
  {
    FAIL() << "Callback was not invoked within the expected time.";
  }
  else
  {
    std::cerr << "Callback was invoked successfully." << std::endl;
    EXPECT_TRUE(callbackInvoked);
    EXPECT_EQ(futureValue.get(), expectedvalue);
  }
}

TEST_F(EventGlobalSystemTest, MultipleListeners)
{
  JobSystem jobSystem({});
  GlobalEventSystem eventSystem(
      {._redisConn = &GetRedisConn(), ._jobSystem = &jobSystem});

  std::mutex mutex;
  std::condition_variable cv;
  std::atomic_int callbackCount = 0;
  std::atomic_bool valueMatched = true;
  int expectedvalue = 42;

  eventSystem.On<SimpleTestEvent>(
      [&](const SimpleTestEvent& message)
      {
        std::cerr << "Listener 1 received event message: " << message.value
                  << std::endl;

        if (message.value != expectedvalue)
          valueMatched = false;

        callbackCount.fetch_add(1, std::memory_order_relaxed);
        cv.notify_one();
      });

  eventSystem.On<SimpleTestEvent>(
      [&](const SimpleTestEvent& message)
      {
        std::cerr << "Listener 2 received event message: " << message.value
                  << std::endl;

        if (message.value != expectedvalue)
          valueMatched = false;

        callbackCount.fetch_add(1, std::memory_order_relaxed);
        cv.notify_one();
      });

  eventSystem.Emit(SimpleTestEvent(expectedvalue)).wait();

  {
    std::unique_lock<std::mutex> lock(mutex);
    const bool receivedBoth = cv.wait_for(
        lock, std::chrono::seconds(5),
        [&] { return callbackCount.load(std::memory_order_relaxed) == 2; });

    ASSERT_TRUE(receivedBoth)
        << "Not all listeners received the event within the expected time.";
  }

  EXPECT_TRUE(valueMatched.load());
  EXPECT_EQ(callbackCount.load(), 2);
}
TEST_F(EventGlobalSystemTest, NoListeners)
{
  JobSystem jobSystem({});
  GlobalEventSystem eventSystem(
      {._redisConn = &GetRedisConn(), ._jobSystem = &jobSystem});

  int expectedvalue = 42;
  // Should not crash or throw even if there are no listeners
  eventSystem.Emit(SimpleTestEvent(expectedvalue)).wait();
  SUCCEED();
}
TEST_F(EventGlobalSystemTest, MultipleListenersFork)
{
    int pipefd[2];
    ASSERT_EQ(::pipe(pipefd), 0);

    JobSystem jobSystem({});
    GlobalEventSystem eventSystem(
        {._redisConn = &GetRedisConn(), ._jobSystem = &jobSystem});

    std::atomic_int callbackCount = 0;
    std::atomic_bool valueMatched = true;
    std::mutex mutex;
    std::condition_variable cv;

    constexpr int expectedValue = 42;

    // =========================
    // PARENT LISTENERS
    // =========================
    eventSystem.On<SimpleTestEvent>(
        [&](const SimpleTestEvent& message)
        {
            if (message.value != expectedValue)
                valueMatched = false;

            callbackCount.fetch_add(1, std::memory_order_relaxed);
            cv.notify_one();
        });

    eventSystem.On<SimpleTestEvent>(
        [&](const SimpleTestEvent& message)
        {
            if (message.value != expectedValue)
                valueMatched = false;

            callbackCount.fetch_add(1, std::memory_order_relaxed);
            cv.notify_one();
        });

    pid_t pid = ::fork();
    ASSERT_NE(pid, -1);

    if (pid == 0)
    {
        // =========================
        // CHILD PROCESS (EMITTER)
        // =========================
        ::close(pipefd[0]);

        try
        {
            JobSystem childJobSystem({});

            std::unique_ptr<Database::RedisConn> childRedisConn = Database::RedisConn::Connect(
                {.host = IPv4("127.0.0.1"),
                 .port = GetRedisPort(),
                 .Mode = Database::RedisConn::RedisMode::eStandalone,
                 .MaxConnectRetries = 5});
            if (!childRedisConn)            {
                std::cerr << "Child failed to connect to Redis server on port " << GetRedisPort() << std::endl;
                const char msg[] = "EXCEPTION";
                ::write(pipefd[1], msg, sizeof(msg));
                ::close(pipefd[1]);
                _exit(2);
            }
            GlobalEventSystem childEventSystem(
                {._redisConn = childRedisConn.get(),
                 ._jobSystem = &childJobSystem});

            // give parent a moment to subscribe (important in pub/sub systems)
            std::this_thread::sleep_for(std::chrono::milliseconds(200));

            childEventSystem.Emit(SimpleTestEvent(expectedValue)).wait();

            const char msg[] = "OK";
            ::write(pipefd[1], msg, sizeof(msg));
            ::close(pipefd[1]);

            _exit(0);
        }
        catch (...)
        {
            const char msg[] = "EXCEPTION";
            ::write(pipefd[1], msg, sizeof(msg));
            ::close(pipefd[1]);
            _exit(2);
        }
    }

    // =========================
    // PARENT WAITS FOR CHILD
    // =========================
    ::close(pipefd[1]);

    char buffer[64] = {0};
    ::read(pipefd[0], buffer, sizeof(buffer));
    ::close(pipefd[0]);

    int status = 0;
    ::waitpid(pid, &status, 0);

    ASSERT_TRUE(WIFEXITED(status));
    ASSERT_EQ(WEXITSTATUS(status), 0);

    EXPECT_STREQ(buffer, "OK");

    // =========================
    // ASSERT PARENT RECEIVED EVENTS
    // =========================
    {
        std::unique_lock<std::mutex> lock(mutex);
        bool ok = cv.wait_for(
            lock,
            std::chrono::seconds(5),
            [&]
            {
                return callbackCount.load(std::memory_order_relaxed) == 2;
            });

        ASSERT_TRUE(ok)
            << "Parent did not receive both listener events in time";
    }

    EXPECT_TRUE(valueMatched.load());
    EXPECT_EQ(callbackCount.load(), 2);
}