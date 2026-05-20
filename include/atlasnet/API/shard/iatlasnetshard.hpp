#pragma once

namespace AtlasNet {
class IAtlasNetShard
{
public:
    virtual ~IAtlasNetShard() = default;

    void AtlasNet_Shard_Init() {
        // Initialization code for the shard
    }

    virtual void OnShutdown() = 0; // Pure virtual function to be implemented by derived classes

};  
}
