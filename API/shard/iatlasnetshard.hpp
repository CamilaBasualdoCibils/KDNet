#pragma once

#include <iostream>
namespace AtlasNet {
class IAtlasNetShard
{
public:
    virtual ~IAtlasNetShard() = default;

    void AtlasNet_Shard_Init() {
        // Initialization code for the shard
        std::cerr << "Initializing AtlasNet Shard..." << std::endl;
    }

    virtual void OnShutdown() = 0; // Pure virtual function to be implemented by derived classes

};  
}
