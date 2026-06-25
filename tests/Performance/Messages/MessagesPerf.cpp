#include "atlasnet/core/SocketAddress.hpp"
#include "atlasnet/core/job/JobSystem.hpp"
#include "atlasnet/core/messages/Message.hpp"
#include "atlasnet/core/messages/MessageSystem.hpp"
#include <array>
#include <gtest/gtest.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <thread>
#include <vector>

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
int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

ATLASNET_MESSAGE(BigMessageTestMessage,
                 ATLASNET_MESSAGE_DATA(std::vector<uint8_t>, data));

                 using DataArray = std::array<uint8_t, 100000>;
ATLASNET_MESSAGE(PerfMessage,
                 ATLASNET_MESSAGE_DATA(DataArray, payload));

TEST(MessagePerformance, StreamThroughput)
{
    using namespace AtlasNet;

    constexpr size_t PayloadSize = 100000;
    constexpr std::chrono::seconds TestDuration(10);

    JobSystem jobsys(JobSystem::Config{});
    MessageSystem msgsys(MessageSystem::Config{
        .jobSystem = &jobsys
    });

    const PortType port = pick_available_port();
    const HostName dnsAddr("localhost");
    SocketAddress serverAddr(dnsAddr, port);

    msgsys.OpenListenSocket(port);
    msgsys.Connect(serverAddr);

    std::atomic<uint64_t> receivedMessages = 0;
    std::atomic<uint64_t> receivedBytes = 0;

    msgsys.On<PerfMessage>(
        [&](const PerfMessage& msg, const SocketAddress&)
        {
            receivedMessages.fetch_add(1, std::memory_order_relaxed);
            receivedBytes.fetch_add(
                msg.payload.size(),
                std::memory_order_relaxed);
        });

    PerfMessage msg;
    std::memset(msg.payload.data(), 0x42, msg.payload.size());

    auto start = std::chrono::steady_clock::now();
    auto endTime = start + TestDuration;

    std::atomic<uint64_t> sentMessages = 0;
    
    unsigned int numThreads = 2;
    std::vector<std::thread> threads;

    for (unsigned int i = 0; i < numThreads; ++i)
    {
        threads.emplace_back([&]()
        {
            while (std::chrono::steady_clock::now() < endTime)
            {
                msgsys.SendMessage(
                    msg,
                    serverAddr,
                    AtlasNet::MessageSendMode::eUnreliableBatched);

                sentMessages.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    for (auto& thread : threads)
    {
        thread.join();
    }

    // Allow in-flight packets to finish.
    std::this_thread::sleep_for(std::chrono::seconds(2));

    msgsys.Shutdown();
    jobsys.Shutdown();
    auto elapsed =
        std::chrono::duration<double>(
            std::chrono::steady_clock::now() - start)
            .count();

    const double bytesPerSecond =
        static_cast<double>(receivedBytes.load()) / elapsed;

    const double mbPerSecond =
        bytesPerSecond / (1024.0 * 1024.0);

    const double gbps =
        (bytesPerSecond * 8.0) / 1'000'000'000.0;

    std::cout
        << "\n=== AtlasNet Throughput ===\n"
        << "Threads:           " << numThreads << "\n"
        << "Sent Messages:     " << sentMessages.load() << "\n"
        << "Received Messages: " << receivedMessages.load() << "\n"
        << "Received Bytes:    " << receivedBytes.load() << "\n"
        << "Bytes/sec:         " << bytesPerSecond << "\n"
        << "MB/sec:            " << mbPerSecond << "\n"
        << "Gbps:              " << gbps << "\n";

    EXPECT_GT(receivedMessages.load(), 0);
}