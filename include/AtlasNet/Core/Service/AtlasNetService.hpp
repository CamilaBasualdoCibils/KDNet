#pragma once

#include "AtlasNet/Core/Core.hpp"
#include "AtlasNet/Core/Network/Address/Address.hpp"
#include "AtlasNet/Core/Network/Address/SocketAddress.hpp"
#include "AtlasNet/Core/Network/Cluster/ClusterCommons.hpp"
#include "AtlasNet/Core/Network/Cluster/Transport/IClusterTransport.hpp"
#include "AtlasNet/Core/Network/Ingress/IngressCommons.hpp"
#include <boost/describe.hpp>
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
    struct IngressSocketOption
    {
      Network::Ingress::IngressTransportType type;
      uint16_t port;
      std::string ExtraArgs;
    };
    std::vector<IngressSocketOption> ingressSockets;
    Network::Cluster::ClusterTransportType clusterTransportType;
    Network::PortType clusterListenPort;
    Network::SocketAddress dbAddress;
  };

private:
  Options options;
  const AtlasNetServiceType service_type;
  std::shared_ptr<spdlog::logger> logger;
  const int signalFd;
  const AtlasNetNodeID nodeID;
  std::atomic_bool stop_requested{false};
  std::shared_ptr<Network::Cluster::IClusterTransport> clusterTransport;
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