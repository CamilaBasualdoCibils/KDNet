
#include "atlasnet/core/network/transport/SteamNetSock/SteamNetSock.hpp"
#include <X11/extensions/randr.h>

#include <condition_variable>
#include <gtest/gtest.h>

#include <netinet/in.h>
using namespace AtlasNet::Network;
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

TEST(SteamNetSock, Init)
{
  SteamNetSockTransport transport;
}
TEST(SteamNetSock, Listen)
{
  SteamNetSockTransport transport;
  int port = pick_available_port();
  ASSERT_NE(port, -1) << "No available port found for testing.";
  SocketAddress address(HostAddress("127.0.0.1"), port);
  auto listener = transport.Listen(address);

  EXPECT_NE(listener, nullptr) << "Failed to create listener on port " << port;
}
TEST(SteamNetSock, ListenAndConnect)
{
  SteamNetSockTransport transport;
  int port = pick_available_port();
  ASSERT_NE(port, -1) << "No available port found for testing.";
  SocketAddress address(HostAddress("127.0.0.1"), port);
  auto listener = transport.Listen(address);

  std::condition_variable cv;
  std::mutex mtx;
  std::condition_variable cv2;
  std::mutex mtx2;
  std::atomic_bool serverConnectionEstablished{false},
      clientConnectionEstablished{false};
  EXPECT_NE(listener, nullptr) << "Failed to create listener on port " << port;
  std::shared_ptr<IConnection> ListenerConnection;
  listener->SetConnectionRequestCallback(
      [&](ConnectionRequest& request, IListener& listener)
      {
        std::cout << "Connection request from: "
                  << request.RemoteAddress().to_string() << std::endl;
        ListenerConnection = request.Accept();
        ListenerConnection->OnStateChange(
            [&](SocketConnectionState state)
            {
              if (state == SocketConnectionState::eConnected)
              {
                std::cout << "Listener connection established with "
                          << ListenerConnection->RemoteAddress().to_string()
                          << std::endl;
                serverConnectionEstablished = true;
                std::lock_guard<std::mutex> lock(mtx);
                cv2.notify_one();
              }
            });
      });
  auto connection = transport.Connect(address);
  EXPECT_NE(connection, nullptr)
      << "Failed to connect to listener on port " << port;

  connection->OnPacketArrival(
      [](IConnection& conn, std::span<const uint8_t> data)
      {
        std::string message(reinterpret_cast<const char*>(data.data()),
                            data.size());
        std::cout << "Received message from "
                  << conn.RemoteAddress().to_string() << ": " << message
                  << std::endl;
      });
  connection->OnStateChange(
      [&](SocketConnectionState  state)
      {
        if (state == SocketConnectionState::eConnected)
        {
          clientConnectionEstablished = true;
          std::lock_guard<std::mutex> lock(mtx);
          cv.notify_one();
        }
      });
  {
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [&] { return clientConnectionEstablished.load(); });
    EXPECT_EQ(connection->GetState(), SocketConnectionState::eConnected)
        << "Connection did not reach connected state within timeout.";
  }
  {
    std::unique_lock<std::mutex> lock(mtx2);
    cv2.wait(lock, [&] { return serverConnectionEstablished.load(); });
    EXPECT_EQ(ListenerConnection->GetState(), SocketConnectionState::eConnected)
        << "Listener connection did not reach connected state within timeout.";
  }
}

TEST(SteamNetSock, SendRecv)
{
  SteamNetSockTransport transport;
  int port = pick_available_port();
  ASSERT_NE(port, -1) << "No available port found for testing.";
  SocketAddress address(HostAddress("127.0.0.1"), port);
  auto listener = transport.Listen(address);

  std::condition_variable cv;
  std::mutex mtx;
  std::condition_variable cv2;
  std::mutex mtx2;
  std::atomic_bool serverConnectionEstablished{false},
      clientConnectionEstablished{false};
  EXPECT_NE(listener, nullptr) << "Failed to create listener on port " << port;
  std::shared_ptr<IConnection> serverConnection;
  listener->SetConnectionRequestCallback(
      [&](ConnectionRequest& request, IListener& listener)
      {
        std::cout << "Connection request from: "
                  << request.RemoteAddress().to_string() << std::endl;
        serverConnection = request.Accept();
        serverConnection->OnStateChange(
            [&](SocketConnectionState state)
            {
              if (state == SocketConnectionState::eConnected)
              {
                std::cout << "Listener connection established with "
                          << serverConnection->RemoteAddress().to_string()
                          << std::endl;
                serverConnectionEstablished = true;
                std::lock_guard<std::mutex> lock(mtx);
                cv2.notify_one();
              }
            });
      });
  auto clientConnection = transport.Connect(address);
  EXPECT_NE(clientConnection, nullptr)
      << "Failed to connect to listener on port " << port;

  clientConnection->OnStateChange(
      [&](SocketConnectionState state)
      {
        if (state == SocketConnectionState::eConnected)
        {
          clientConnectionEstablished = true;
          std::lock_guard<std::mutex> lock(mtx);
          cv.notify_one();
        }
      });
  {
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [&] { return clientConnectionEstablished.load(); });
    EXPECT_EQ(clientConnection->GetState(), SocketConnectionState::eConnected)
        << "Connection did not reach connected state within timeout.";
  }
  {
    std::unique_lock<std::mutex> lock(mtx2);
    cv2.wait(lock, [&] { return serverConnectionEstablished.load(); });
    EXPECT_EQ(serverConnection->GetState(), SocketConnectionState::eConnected)
        << "Listener connection did not reach connected state within timeout.";
  }

  auto OnMessage =
      [](IConnection& connection, std::span<const uint8_t> data)
  {
    std::string message(reinterpret_cast<const char*>(data.data()),
                        data.size());
  };

  serverConnection->OnPacketArrival(OnMessage);
  clientConnection->OnPacketArrival(OnMessage);

  serverConnection->Send(std::span<const uint8_t>(
      reinterpret_cast<const uint8_t*>("Hello from server"), 17),
      SocketSendMode::eReliable);
  clientConnection->Send(std::span<const uint8_t>(
      reinterpret_cast<const uint8_t*>("Hello from client"), 17),
      SocketSendMode::eReliable);
}