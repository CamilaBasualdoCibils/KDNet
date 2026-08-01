#pragma once

#include "AtlasNet/Core/Core.hpp"
namespace AtlasNet::Network::Cluster
{

struct ClusterDatagram
{
  AtlasNetNodeID source;

  std::span<const std::byte> payload;

  void* owner = nullptr;
  void* userdata = nullptr;
  void (*release)(void*, void*) = nullptr;
  void Release()
  {
    if (release)
      release(owner, userdata);
  }
};
} // namespace AtlasNet::Network::Cluster