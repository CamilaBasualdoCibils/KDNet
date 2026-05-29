#pragma once

#include "IServiceAdapter.hpp"
#include "atlasnet/core/Json.hpp"

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <iostream>
#include <nlohmann/json.hpp>

#include <fstream>
#include <string>

namespace AtlasNet
{

class KubernetesServiceAdapter : public IServiceAdapter
{
public:
  enum ImagePullPolicy
  {
    Always,
    IfNotPresent,
    Never
  };
  KubernetesServiceAdapter(
      const std::string_view& serviceName, const std::string_view& imageName,
      ImagePullPolicy imagePullPolicy,
      const std::string& namespaceName = "");

  void Create() override;

  bool Exists() const override;

  void Destroy() override;

  void SetReplicaCount(uint32_t count) override;

  uint32_t GetReplicaCount() const override;

private:
  struct Response
  {
    int status;
    std::string body;
  };

  std::string _namespace;

  boost::asio::io_context _ioc;
  boost::asio::ssl::context _ctx;
  mutable boost::asio::ip::tcp::resolver _resolver;
  mutable boost::beast::ssl_stream<boost::beast::tcp_stream> _stream;

  std::string _token;
  ImagePullPolicy _imagePullPolicy;

private:
  static std::string DetectNamespace();

  void LoadToken();

  void ConfigureTLS();

  Response Request(const std::string& method, const std::string& target,
                   const std::string& body,
                   const std::string& contentType = "application/json") const;

  _Json BuildStatefulSetJSON() const;
};

} // namespace AtlasNet