#include "AtlasNet/Core/Service/AtlasNetService.hpp"
#include "AtlasNet/Core/Core.hpp"
#include "AtlasNet/Core/Network/Address/Address.hpp"
#include "AtlasNet/Core/Network/Address/SocketAddress.hpp"
#include "AtlasNet/Core/Network/Cluster/Transport/UDP/UDPClusterTransport.hpp"

#include <boost/describe/enum_to_string.hpp>
#include <boost/program_options.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <sys/signalfd.h>

AtlasNet::AtlasNetService::AtlasNetService(AtlasNetServiceType service_type,
                                           int argc, char** argv)
    : signalFd(SetupSignals()), nodeID(AtlasNet::AtlasNetNodeID::Generate()),
      service_type(service_type),
      logger(spdlog::stdout_color_mt(
          "AtlasNet:" + std::string(boost::describe::enum_to_string(
                            service_type, "<INVALID>"))))
{
  namespace po = boost::program_options;
  po::options_description desc{"AtlasNetService Options"};
  AddOptions(desc);
  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);
  if (vm.count("help"))
  {
    std::cerr << desc << std::endl;
    exit(0);
  }

  ParseOptions(vm);
}
void AtlasNet::AtlasNetService::Run()
{
  logger->info("Starting {}[{}] - {}...",
               boost::describe::enum_to_string(service_type, "<INVALID>"),
               nodeID.to_short_string(), nodeID.to_string());
  if (options.clusterTransportType ==
      Network::Cluster::ClusterTransportType::INVALID)
  {
    logger->warn(
        "No cluster transport specified. Use --cluster-transport or "
        "ATLASNET_CLUSTER_TRANSPORT environment variable. defaulting to UDP.");
    options.clusterTransportType = Network::Cluster::ClusterTransportType::UDP;
  }
  logger->info("cluster messaging: {}:{}",
               boost::describe::enum_to_string(options.clusterTransportType,
                                               "<INVALID>"),
               options.clusterListenPort != 0
                   ? std::to_string(options.clusterListenPort)
                   : "ephemeral");
  switch (options.clusterTransportType)
  {

  case Network::Cluster::ClusterTransportType::INVALID:
throw std::runtime_error(
        "Invalid cluster transport type. Use --cluster-transport or "
        "ATLASNET_CLUSTER_TRANSPORT environment variable.");
  case Network::Cluster::ClusterTransportType::UDP:
    clusterTransport = std::make_shared<Network::Cluster::UDPClusterTransport>(
        options.clusterListenPort, nullptr);
    break;
  case Network::Cluster::ClusterTransportType::DPDK:
    break;
  }
  GetLogger()->info("Cluster transport listening on port {}",
                    clusterTransport->GetListenPort());
  // Initialize the service
  Initialize();

  // Start the main loop
  MainLoop();
}

void AtlasNet::AtlasNetService::AddOptions(
    boost::program_options::options_description& desc)
{
  namespace po = boost::program_options;

  desc.add_options()

      // Node Sockets
      ("cluster-port", po::value<uint16_t>(),
       "Internal cluster port for node-to-node communication. EX: 1925");
}
void AtlasNet::AtlasNetService::ParseOptions(
    const boost::program_options::variables_map& vm)
{
  options.clusterListenPort =
      vm.count("cluster-port")
          ? static_cast<uint16_t>(vm["cluster-port"].as<uint16_t>())
          : (std::getenv("ATLASNET_CLUSTER_PORT")
                 ? static_cast<uint16_t>(
                       std::stoi(std::getenv("ATLASNET_CLUSTER_PORT")))
                 : 0); // Default to port 0 (ephemeral)
}

void AtlasNet::AtlasNetService::MainLoop()
{
  while (stop_requested.load() == false)
  {
    std::optional<int> signal = CheckForSignal();
    if (signal.has_value())
    {
      switch (signal.value())
      {
      case SIGINT:
        logger->info("SIGINT: Ctrl+C");
        break;

      case SIGTERM:
        logger->info("SIGTERM");
        break;

      case SIGHUP:
        logger->info("SIGHUP: Reload config");
        break;

      case SIGQUIT:
        logger->info("SIGQUIT");
        break;
      default:
        logger->info("Received signal: {}. Ignoring.", signal.value());
        break;
      }
      stop_requested.store(true);
      continue;
    }
    Tick();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}
std::string AtlasNet::AtlasNetService::GetHostID()
{
  return "INVALID";
}
AtlasNet::Network::HostAddress AtlasNet::AtlasNetService::GetNodeAddress()
{
  char hostname[256];
  if (gethostname(hostname, sizeof(hostname)) != 0)
  {
    throw std::runtime_error("Failed to get hostname");
  }
  return Network::HostAddress(hostname);
}
int AtlasNet::AtlasNetService::SetupSignals()
{
  sigset_t mask;
  sigemptyset(&mask);

  sigaddset(&mask, SIGINT);
  sigaddset(&mask, SIGTERM);
  sigaddset(&mask, SIGHUP);
  sigaddset(&mask, SIGQUIT);
  if (pthread_sigmask(SIG_BLOCK, &mask, nullptr) != 0)
    throw std::runtime_error("pthread_sigmask failed");

  int signalFd = signalfd(-1, &mask, SFD_CLOEXEC | SFD_NONBLOCK);

  if (signalFd == -1)
    throw std::runtime_error("signalfd failed");
  return signalFd;
}
std::optional<int> AtlasNet::AtlasNetService::CheckForSignal()
{
  signalfd_siginfo info;

  ssize_t n = read(signalFd, &info, sizeof(info));

  if (n != sizeof(info))
    return std::nullopt;
  return info.ssi_signo;
}
