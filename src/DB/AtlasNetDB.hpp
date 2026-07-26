#pragma once

#include "AtlasNet/Core/Service/AtlasNetService.hpp"
#include <memory>
#include <spdlog/sinks/stdout_color_sinks.h>
namespace AtlasNet
{
class AtlasNetDB : public AtlasNetService
{

public:

  AtlasNetDB(int argc, char** argv)
      : AtlasNetService(AtlasNetServiceType::DB, argc, argv)
  {
   
    
  
  }

private:
  void Initialize() override;
  void Tick() override;
};
}; // namespace AtlasNet