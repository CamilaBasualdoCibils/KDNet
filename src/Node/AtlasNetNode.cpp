
#include "AtlasNetNode.hpp"
#include "AtlasNet/Core/Network/Address/Address.hpp"
#include "AtlasNet/Core/Network/Address/SocketAddress.hpp"
#include <cstdlib>

inline std::vector<AtlasNet::AtlasNetNode::NodeConfig::IngressSocketOption>
ParseSocketOptions(const std::vector<std::string>& ingressOptions)
{
  using AN = AtlasNet::AtlasNetService;
  std::vector<AtlasNet::AtlasNetNode::NodeConfig::IngressSocketOption> result;

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

   AtlasNet::AtlasNetNode::NodeConfig::IngressSocketOption option{};

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
void AtlasNet::AtlasNetNode::AddOptions(
    boost::program_options::options_description& desc)
{
  AtlasNetService::AddOptions(desc);
  desc.add_options()
      // Ingress Sockets
      ("ingress-sockets",
       boost::program_options::value<std::vector<std::string>>()->multitoken(),
       "Ingress Socket Types. EX: SteamNetSock:port=8888;UDP:port=1262")
      // DB Host
      ("DB-host", boost::program_options::value<std::string>(),
       "Database host address. EX: 127.0.0.1")
      // DB Port
      ("DB-port", boost::program_options::value<uint16_t>(),
       "Database port. EX: 6379");
}
void AtlasNet::AtlasNetNode::Initialize() {


  for (const auto& socket : nodeConfig.ingressSockets)
  {
    GetLogger()->info("Ingress Socket: {}:{} {}",
                 boost::describe::enum_to_string(socket.type, "<INVALID>"),
                 socket.port, socket.ExtraArgs);
  }
  GetLogger()->info("Pinging DB at {}...", nodeConfig.dbAddress.to_string());
}

void AtlasNet::AtlasNetNode::ParseOptions(
    const boost::program_options::variables_map& vm)
{
  if (!(vm.count("DB-host") && std::getenv("ATLASNET_DB_HOST")) ||
      !(vm.count("DB-port") && std::getenv("ATLASNET_DB_PORT")))
  {
    GetLogger()->error(
        "Database host and port must be specified via command line DB-Host and "
        "DB-Port or "
        "environment variables ATLASNET_DB_HOST and ATLASNET_DB_PORT.");
    throw std::runtime_error("Database host and port must be specified");
  }
  Network::HostAddress dbHostAddress(vm.count("DB-host") &&
                                             !vm["DB-host"].empty()
                                         ? vm["DB-host"].as<std::string>()
                                         : std::getenv("ATLASNET_DB_HOST"));
  Network::PortType dbPort =
      vm.count("DB-port") && !vm["DB-port"].empty()
          ? static_cast<uint16_t>(vm["DB-port"].as<uint16_t>())
          : (std::getenv("ATLASNET_DB_PORT")
                 ? static_cast<uint16_t>(
                       std::stoi(std::getenv("ATLASNET_DB_PORT")))
                 : 0);

  nodeConfig.dbAddress = Network::SocketAddress(dbHostAddress, dbPort);
   nodeConfig.ingressSockets = ParseSocketOptions(
      !vm["ingress-sockets"].empty()
          ? vm["ingress-sockets"].as<std::vector<std::string>>()
          : (std::getenv("ATLASNET_INGRESS_SOCKETS")
                 ? std::vector<std::string>{std::getenv(
                       "ATLASNET_INGRESS_SOCKETS")}
                 : std::vector<std::string>{}));
  if (nodeConfig.ingressSockets.empty())
  {
    GetLogger()->warn("AtlasNetService: No ingress sockets specified.\n Use "
                 "--ingress-sockets "
                 "or ATLASNET_INGRESS_SOCKETS environment variable.\n This "
                 "service will not "
                 "accept any incoming client connections.");
  }
}