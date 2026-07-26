#pragma once

#include "AtlasNet/Core/Core.hpp"
#include "AtlasNet/Core/Network/Address/Address.hpp"
#include "AtlasNet/Core/Network/Address/SocketAddress.hpp"
#include "AtlasNet/Core/Network/Transport/Connection/IConnection.hpp"
#include "AtlasNet/Core/Network/Transport/Connection/IConnectionListener.hpp"
#include "AtlasNet/Core/Network/Transport/Connection/IConnectionTransport.hpp"
#include "AtlasNet/Core/Network/Transport/ITransport.hpp"
#include "AtlasNet/Core/Network/Transport/TransportCommons.hpp"
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <chrono>
#include <memory>
#include <set>
#include <spdlog/logger.h>
#include <thread>
#include <unordered_set>
namespace AtlasNet
{
enum class AtlasNetServiceType
{
  Node,
  DB,
  Gateway
};
BOOST_DESCRIBE_ENUM(AtlasNetServiceType, Node, DB, Gateway);
class AtlasNetService
{
public:
  struct Options
  {
    struct SocketOption
    {
      Network::TransportType type;
      uint16_t port;
      std::string ExtraArgs;
    };
    std::vector<SocketOption> ingressSockets;

    Network::SocketAddress internalListenAddress;
    Network::SocketAddress dbAddress;
  };

private:
  Options options;
  const AtlasNetServiceType service_type;
  std::shared_ptr<spdlog::logger> logger;
  const int signalFd;
  const AtlasNetNodeID nodeID;
  std::atomic_bool stop_requested{false};

  std::shared_ptr<Network::IConnectionTransport> databaseTransport;
  std::shared_ptr<Network::IConnection> databaseConnection;


public:
  AtlasNetService(AtlasNetServiceType service_type, int argc, char** argv);
  void Run();

  virtual ~AtlasNetService() = default;

  auto GetLogger() const -> std::shared_ptr<spdlog::logger>
  {
    return logger;
  }
protected:
  virtual void AddOptions(boost::program_options::options_description& desc);
  virtual void ParseOptions(const boost::program_options::variables_map& vm);

private:
  void MainLoop();
  virtual void Initialize() = 0;
  virtual void Tick() = 0;

  static std::string GetHostID();
  static AtlasNet::Network::HostAddress GetNodeAddress();
  static int SetupSignals();
  std::optional<int> CheckForSignal();

};
} // namespace AtlasNet