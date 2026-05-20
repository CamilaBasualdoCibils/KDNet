#pragma once

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <stdexcept>
#include <string>
inline std::string
QueryDockerSocket(const std::string& method, const std::string& target,
                  const std::string& body = "",
                  const std::string& socketPath = "/var/run/docker.sock")
{
  namespace asio = boost::asio;
  namespace beast = boost::beast;
  namespace http = beast::http;

  asio::io_context ioc;
  asio::local::stream_protocol::socket socket(ioc);

  socket.connect(asio::local::stream_protocol::endpoint(socketPath));

  http::request<http::string_body> req;
  req.version(11); // HTTP/1.1
  req.method_string(method);
  req.target(target);
  req.set(http::field::host, "localhost");
  req.set(http::field::user_agent, "AtlasNet-DockerSocket");
  req.set(http::field::accept, "application/json");

  if (!body.empty())
  {
    req.set(http::field::content_type, "application/json");
    req.body() = body;
    req.prepare_payload();
  }

  http::write(socket, req);

  beast::flat_buffer buffer;
  http::response<http::string_body> res;
  http::read(socket, buffer, res);

  beast::error_code ec;
  socket.shutdown(asio::local::stream_protocol::socket::shutdown_both, ec);
  socket.close(ec);

  if (res.result_int() < 200 || res.result_int() >= 300)
  {
    throw std::runtime_error("Docker API request failed. HTTP " +
                             std::to_string(res.result_int()) + ": " +
                             res.body());
  }

  return res.body();
}

inline std::string GetSelfContainerName()
{
  if (const char* h = std::getenv("HOSTNAME"))
  {
    if (*h)
    {
      return std::string(h);
    }
  }

  throw std::runtime_error(
      "Could not determine current container ID from HOSTNAME");
}