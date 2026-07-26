
#include "AtlasNetNode.hpp"
#include "AtlasNet/Core/Network/Address/Address.hpp"
#include "AtlasNet/Core/Network/Address/SocketAddress.hpp"
#include <cstdlib>

void AtlasNet::AtlasNetNode::AddOptions(
    boost::program_options::options_description& desc)
{
  AtlasNetService::AddOptions(desc);
  desc.add_options()
      // DB Host
      ("DB-host", boost::program_options::value<std::string>(),
       "Database host address. EX: 127.0.0.1")
       // DB Port
       (
          "DB-port", boost::program_options::value<uint16_t>(),
          "Database port. EX: 6379");
        
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
}