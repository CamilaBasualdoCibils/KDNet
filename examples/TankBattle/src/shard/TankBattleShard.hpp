
#pragma once
#include "shard/iatlasnetshard.hpp"

class TankBattleShard : AtlasNet::IAtlasNetShard
{
public:
    TankBattleShard() = default;
    ~TankBattleShard() override = default;

    void Run()
    {
        AtlasNet_Shard_Init();
        
        OnShutdown();
    }
    void OnShutdown() override {
        // Cleanup code for the shard
        std::cerr << "Shutting down AtlasNet Shard..." << std::endl;
    }

};