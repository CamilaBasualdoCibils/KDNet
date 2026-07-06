#pragma once

#include "steam/steamtypes.h"
namespace AtlasNet
{

enum class CommandSendMode : uint8
{
  eReliable, // Reliable command, guaranteed to be delivered, sent as soon as
             // possible

  eReliableBatched, // Reliable command, batched together with other reliable
                    // commands

  eUnreliable,      // Unreliable command, may be lost, sent as soon as possible

  eUnreliableBatched, // Unreliable command, batched together with other
                      // unreliable commands

  eNoDelay,            // Command sent immediately without delay or dropped
  
  eInvalid
};
}