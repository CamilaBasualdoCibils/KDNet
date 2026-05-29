#pragma once
#include <cstdint>
#include <string>
#include <string_view>
namespace AtlasNet
{
    class IAtlasNetClient
    {
public:
        void AtlasNetClient_Init();
        
        enum class AtlasNetClientError
        {
            None,
            ConnectionFailed,
            UnknownError
        };
        bool AtlasNetClient_Connect(const std::string_view& address, uint16_t port, AtlasNetClientError* error = nullptr,std::string* errorMessage = nullptr);
    };
}