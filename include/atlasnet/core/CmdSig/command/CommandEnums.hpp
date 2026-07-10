#pragma once

#include "steam/steamtypes.h"
#include <cstdint>
namespace AtlasNet
{

enum class CommandSendMode : uint8
{
  eReliable, // Reliable command, guaranteed to be delivered, sent as soon as
             // possible

  eReliableBatched, // Reliable command, batched together with other reliable
                    // commands

  eUnreliable, // Unreliable command, may be lost, sent as soon as possible

  eUnreliableBatched, // Unreliable command, batched together with other
                      // unreliable commands

  eNoDelay, // Command sent immediately without delay or dropped

  eInvalid
};

enum class CommandDeliveryGuarantee : uint8_t
{
  NoDelay, // Send immediately or drop.
           // No retries or acknowledgements.

  Unreliable,        // Retry enough to get onto the wire.
                     // Arrival is not guaranteed.
  UnreliableBatched, // Like Unreliable, but may be batched with other commands,
                     // introducing additional delay.

  GatewayConfirmed, // Guaranteed received by the AtlasNet gateway.
  GatewayConfirmedBatched,

  ServerConfirmed, // Guaranteed received by the owning shard/server
                   // through the AtlasNet network.
  ServerConfirmedBatched,

  Invalid
};

enum class CommandAckStatus : uint8_t
{
  Dropped, // Command was dropped and will not be delivered.
  Sent,    // Command was sent but not yet acknowledged.
  GatewayAck,
  ServerAck,
  TimedOut, // Command was not acknowledged within the expected time frame.
  Invalid
};

} // namespace AtlasNet