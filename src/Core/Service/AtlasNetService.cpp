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

inline std::vector<AtlasNet::AtlasNetService::Options::IngressSocketOption>
ParseSocketOptions(const std::vector<std::string>& ingressOptions)
{
  using AN = AtlasNet::AtlasNetService;
  std::vector<AN::Options::IngressSocketOption> result;

  auto parseEntry = [&](const std::string& entry)
  {
    if (entry.empty())
      return;

    auto colon = entry.find(':');

    std::string typeString;
    std::string args;

    if (colon == std::string::npos)
    {
      typeString = entry;
    }
    else
    {
      typeString = entry.substr(0, colon);
      args = entry.substr(colon + 1);
    }

    AN::Options::IngressSocketOption option{};

    bool parsedType =
        boost::describe::enum_from_string(typeString, option.type);

    if (!parsedType)
    {
      throw std::runtime_error("Unknown ingress transport type: " + typeString);
    }

    option.port = 0;

    // Parse comma-separated args
    std::stringstream stream(args);
    std::string arg;

    std::vector<std::string> extraArgs;

    while (std::getline(stream, arg, ','))
    {
      auto equals = arg.find('=');

      if (equals == std::string::npos)
      {
        extraArgs.push_back(arg);
        continue;
      }

      auto key = arg.substr(0, equals);
      auto value = arg.substr(equals + 1);

      if (key == "port")
      {
        option.port = static_cast<uint16_t>(std::stoi(value));
      }
      else
      {
        extraArgs.push_back(arg);
      }
    }

    // Preserve everything transport-specific
    for (size_t i = 0; i < extraArgs.size(); i++)
    {
      if (i)
        option.ExtraArgs += ",";

      option.ExtraArgs += extraArgs[i];
    }

    result.push_back(std::move(option));
  };

  for (const auto& input : ingressOptions)
  {
    // Support either:
    // ["a", "b"]
    // or:
    // ["a;b"]
    std::stringstream stream(input);
    std::string entry;

    while (std::getline(stream, entry, ';'))
    {
      parseEntry(entry);
    }
  }

  return result;
}
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
  logger->info("cluster messaging: {} -> {}",
               boost::describe::enum_to_string(options.clusterTransportType,
                                               "<INVALID>"),
               options.clusterListenPort != 0
                   ? std::to_string(options.clusterListenPort)
                   : "ephemeral");
  switch (options.clusterTransportType)
  {

  case Network::Cluster::ClusterTransportType::INVALID:
  case Network::Cluster::ClusterTransportType::UDP:
    clusterTransport = std::make_shared<Network::Cluster::UDPClusterTransport>(
        options.clusterListenPort, nullptr);
    break;
  case Network::Cluster::ClusterTransportType::DPDK:
    break;
  }
  for (const auto& socket : options.ingressSockets)
  {
    logger->info("Ingress Socket: {}:{} {}",
                 boost::describe::enum_to_string(socket.type, "<INVALID>"),
                 socket.port, socket.ExtraArgs);
  }

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
      // Ingress Sockets
      ("ingress-sockets", po::value<std::vector<std::string>>()->multitoken(),
       "Ingress Socket Types. EX: SteamNetSock:port=8888;UDP:port=1262")
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

  options.ingressSockets = ParseSocketOptions(
      !vm["ingress-sockets"].empty()
          ? vm["ingress-sockets"].as<std::vector<std::string>>()
          : (std::getenv("ATLASNET_INGRESS_SOCKETS")
                 ? std::vector<std::string>{std::getenv(
                       "ATLASNET_INGRESS_SOCKETS")}
                 : std::vector<std::string>{}));
  if (options.ingressSockets.empty())
  {
    logger->warn("AtlasNetService: No ingress sockets specified.\n Use "
                 "--ingress-sockets "
                 "or ATLASNET_INGRESS_SOCKETS environment variable.\n This "
                 "service will not "
                 "accept any incoming client connections.");
  }
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
