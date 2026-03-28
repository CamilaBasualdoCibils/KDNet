#include "atlasnet/core/container/Container.hpp"
#include "atlasnet/core/Address.hpp"
#include "atlasnet/core/Json.hpp"
#include "atlasnet/core/SocketAddress.hpp"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;

std::string get_self_container_id_from_hostname()
{
  char hostname[256] = {};
  if (::gethostname(hostname, sizeof(hostname)) != 0)
  {
    throw std::runtime_error("gethostname() failed");
  }
  hostname[sizeof(hostname) - 1] = '\0';
  return std::string(hostname);
}

std::string get_container_ip_on_network(
    const std::string& container_id, const std::string& network_name,
    const std::string& docker_sock = "/var/run/docker.sock")
{
  asio::io_context ioc;

  // Unix domain socket client
  asio::local::stream_protocol::socket socket{ioc};
  socket.connect(asio::local::stream_protocol::endpoint(docker_sock));

  // Beast can speak HTTP over any sync read/write stream.
  http::request<http::empty_body> req{
      http::verb::get, "/containers/" + container_id + "/json", 11};
  req.set(http::field::host, "localhost");
  req.set(http::field::user_agent, "beast-docker-client");
  req.prepare_payload();

  http::write(socket, req);

  beast::flat_buffer buffer;
  http::response<http::string_body> res;
  http::read(socket, buffer, res);

  if (res.result() != http::status::ok)
  {
    throw std::runtime_error(
        "Docker API error: " + std::to_string(res.result_int()) + " " +
        std::string(res.reason()));
  }

  auto doc = Json::parse(res.body());

  if (!doc.contains("NetworkSettings") ||
      !doc["NetworkSettings"].contains("Networks"))
  {
    throw std::runtime_error(
        "Docker response missing NetworkSettings.Networks");
  }

  const auto& networks = doc["NetworkSettings"]["Networks"];

  if (!networks.contains(network_name))
  {
    throw std::runtime_error("Container is not attached to network: " +
                             network_name);
  }

  const auto& net = networks[network_name];

  if (!net.contains("IPAddress"))
  {
    throw std::runtime_error("Network entry missing IPAddress");
  }

  return net["IPAddress"].get<std::string>();
}

AtlasNet::HostAddress AtlasNet::IContainer::GetOverlayAddressOfSelf() const
{

  std::string ip_s = get_container_ip_on_network(get_self_container_id_from_hostname(),
                              EnvVars::OverlayNetworkName);
                              return HostAddress(ip_s);
}

bool AtlasNet::IContainer::ShutdownRequested() const
{
  return shutdown.load(std::memory_order_acquire);
}

AtlasNet::IContainer::IContainer(ContainerType type) : type(type)
{
  auto handleShutdown = [](int)
  {
    std::cerr << "SIGINT received, shutting down container..." << std::endl;
    IContainer::Get().shutdown.store(true, std::memory_order_release);
    IContainer::Get().cv.notify_all();
  };
  std::signal(SIGINT, handleShutdown);
  std::signal(SIGTERM, handleShutdown);
}
void AtlasNet::IContainer::Init() {
    std::cerr << GetOverlayAddressOfSelf().to_string() << std::endl;
}
