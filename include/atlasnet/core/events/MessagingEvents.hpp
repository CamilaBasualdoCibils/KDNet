#pragma once

#include "atlasnet/core/SocketAddress.hpp"
#include "atlasnet/core/events//IEvent.hpp"

ATLASNET_EVENT(ConnectionStartedInternallyEvent,
               ATLASNET_EVENT_FIELD(AtlasNet::SocketAddress, address));

ATLASNET_EVENT(ConnectionRequestReceivedEvent,
               ATLASNET_EVENT_FIELD(AtlasNet::SocketAddress, remoteAddr),
               ATLASNET_EVENT_FIELD(AtlasNet::PortType, localPort));

ATLASNET_EVENT(ConnectionAcceptedInternallyPreHandshakeEvent,
               ATLASNET_EVENT_FIELD(AtlasNet::SocketAddress, address));
               
ATLASNET_EVENT(ConnectionAcceptedExternalPreHandshakeEvent,
               ATLASNET_EVENT_FIELD(AtlasNet::SocketAddress, address));

ATLASNET_EVENT(ConnectionEstablishedEvent,
               ATLASNET_EVENT_FIELD(AtlasNet::SocketAddress, address));

ATLASNET_EVENT(HandshakeProcessDeniedEvent,
               ATLASNET_EVENT_FIELD(AtlasNet::SocketAddress, address));

ATLASNET_EVENT(ConnectionTerminatedByRemoteEvent,
               ATLASNET_EVENT_FIELD(AtlasNet::SocketAddress, address));

ATLASNET_EVENT(ConnectionTerminatedByLocalEvent,
               ATLASNET_EVENT_FIELD(AtlasNet::SocketAddress, address));