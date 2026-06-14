#pragma once

#include "RPCConcepts.hpp"

#define ATLASNET_RPC_SIG(...) __VA_ARGS__
#define ATLASNET_RPC_METHOD(Name, Signature)                                   \
  struct Name                                                                  \
      : AtlasNet::RPC_Internal::Method<AtlasNet::RPC_Internal::Fnv1a32(#Name), \
                                       Signature>                              \
  {                                                                            \
    static constexpr const char* GetName()                                     \
    {                                                                          \
      return #Name;                                                            \
    }                                                                          \
  };
#define ATLASNET_RPC(ServiceName, ...)                                         \
  struct ServiceName                                                           \
  {                                                                            \
    __VA_ARGS__                                                                \
  };