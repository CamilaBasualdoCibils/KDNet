#pragma once
#include <shard/iatlasnetshard.hpp>

class DummyShard : public AtlasNet::IAtlasNetShard
{
public:
    DummyShard() = default;
    ~DummyShard() override = default;

    void Run()
    {
        AtlasNet_Shard_Init();
    }
    void OnShutdown() override {
        // Cleanup code for the shard
    }


};