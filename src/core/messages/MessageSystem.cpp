#include "atlasnet/core/messages/MessageSystem.hpp"
#include "atlasnet/core/Address.hpp"
#include "atlasnet/core/SocketAddress.hpp"
#include "atlasnet/core/assert.hpp"
#include "atlasnet/core/events/MessagingEvents.hpp"
#include "atlasnet/core/job/JobContext.hpp"
#include "atlasnet/core/job/JobEnums.hpp"
#include "atlasnet/core/job/JobOptions.hpp"
#include "atlasnet/core/job/JobSystem.hpp"
#include "atlasnet/core/messages/HandshakePacket.hpp"
#include "atlasnet/core/messages/Message.hpp"
#include "atlasnet/core/serialize/ByteReader.hpp"
#include "atlasnet/core/serialize/ByteWriter.hpp"
#include "boost/describe/enum_to_string.hpp"
#include "enviroment/Enviroment.hpp"
#include "steam/isteamnetworkingutils.h"
#include "steam/steamnetworkingsockets.h"
#include "steam/steamnetworkingtypes.h"

#include <cstdint>
#include <format>
#include <iostream>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <span>
#include <vector>

AtlasNet::MessageSystem::MessageSystem(const Config& config) : config_(config)
{
  AN_ASSERT(config_.jobSystem, "JobSystem must be provided");
  if (config_.handshakeHandler)
  {
    std::cerr << "Handshake handler provided. Incoming connections will be "
                 "subject to handshake validation.\n";
  }
  else
  {
    std::cerr << "WARNING: No handshake handler provided. All incoming "
                 "connections will be accepted by default.\n";
  }
  SteamNetworkingIdentity identity;
  if (config_.handshakeIdentity)
  {
    ByteWriter handshakeWriter;
    config_.handshakeIdentity->Serialize(handshakeWriter);
    bool success = identity.SetGenericBytes(handshakeWriter.data(),
                                            handshakeWriter.size());
    AN_ASSERT(success,
              "Failed to set SteamNetworkingIdentity with handshake data");
    if (!success)
    {
      throw std::runtime_error(
          "Failed to set SteamNetworkingIdentity with handshake data");
    }
  }

  SteamNetworkingErrMsg errMsg = {0};
  const bool result = GameNetworkingSockets_Init(&identity, errMsg);
  if (errMsg[0] != '\0')
  {
    std::cerr << "GameNetworkingSockets_Init message: " << errMsg << std::endl;
  }
  if (!result)
  {
    throw std::runtime_error(
        std::string("Failed to initialize GameNetworkingSockets: ") + errMsg);
  }

  SteamNetworkingUtils()->SetDebugOutputFunction(
      k_ESteamNetworkingSocketsDebugOutputType_Msg,
      [](ESteamNetworkingSocketsDebugOutputType, const char* pszMsg)
      { std::cerr << "GNS Debug: " << pszMsg << std::endl; });

  _GNS = SteamNetworkingSockets();

  _pollGroup = GNS().CreatePollGroup();
  if (_pollGroup == k_HSteamNetPollGroup_Invalid)
  {
    GameNetworkingSockets_Kill();
    throw std::runtime_error("Failed to create SteamNetworking poll group");
  }

  _pollJobHandle = config_.jobSystem->Submit(
      [this](JobContext& handle)
      {
        if (shutdown.load(std::memory_order_acquire))
        {
          std::cerr << "MessageSystem poll job exiting due to shutdown signal."
                    << std::endl;
          return;
        }
        else
        {
          handle.repeat_once(std::chrono::milliseconds(1000 / Env::TickRate));
        }

        GNS().RunCallbacks();
        _Parse_Incoming_Messages();
      },

      JobOpts::Name("MessageSystem::Poll"),
      JobOpts::Notify<JobNotifyLevel::eNone>(),
      JobOpts::TPriority<JobPriority::eHigh>{});
}

AtlasNet::MessageSystem::ListenSocketHandle&
AtlasNet::MessageSystem::OpenListenSocket(PortType port)
{
  std::unique_lock lock(_mutex);

  if (_listenSockets.contains(port))
  {
    throw std::runtime_error("Listen socket already open");
  }

  SteamNetworkingIPAddr localAddr;
  localAddr.Clear();
  localAddr.m_port = port;

  SteamNetworkingConfigValue_t opts[2];
  opts[0].SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged,
                 (void*)&OnSteamNetConnectionStatusChanged);
  opts[1].SetInt64(k_ESteamNetworkingConfig_ConnectionUserData, (int64_t)this);

  const HSteamListenSocket listenSocket =
      GNS().CreateListenSocketIP(localAddr, 2, opts);

  if (listenSocket == k_HSteamListenSocket_Invalid)
  {
    throw std::runtime_error("Failed to create listen socket on port " +
                             std::to_string(port));
  }

  std::cerr << "Opened listen socket on port " << port << std::endl;

  auto inserted = _listenSockets.emplace(
      port, std::make_unique<ListenSocketHandle>(*this, listenSocket, port));

  return *inserted.first->second;
}

AtlasNet::MessageSystem::ListenSocketHandle&
AtlasNet::MessageSystem::GetListenSocket(PortType port)
{
  std::shared_lock lock(_mutex);

  auto it = _listenSockets.find(port);
  if (it == _listenSockets.end())
  {
    throw std::runtime_error("Listen socket not open");
  }

  return *it->second;
}

void AtlasNet::MessageSystem::SteamNetConnectionStatusChanged(
    SteamNetConnectionStatusChangedCallback_t* pInfo)
{
  if (!pInfo)
    return;

  switch (pInfo->m_info.m_eState)
  {
  case k_ESteamNetworkingConnectionState_Connecting:
    OnConnectionStatus_Connecting(pInfo);
    break;

  case k_ESteamNetworkingConnectionState_Connected:
    OnConnectionStatus_Connected(pInfo);
    break;

  case k_ESteamNetworkingConnectionState_ClosedByPeer:
    OnConnectionStatus_ClosedByPeer(pInfo);
    break;

  case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
    OnConnectionStatus_ProblemDetectedLocally(pInfo);

    break;

  case k_ESteamNetworkingConnectionState_None:
    std::cout << "k_ESteamNetworkingConnectionState_None for: "
              << pInfo->m_info.m_szConnectionDescription << std::endl;
    break;
  case k_ESteamNetworkingConnectionState_FindingRoute:
  case k_ESteamNetworkingConnectionState_FinWait:
  case k_ESteamNetworkingConnectionState_Linger:
  case k_ESteamNetworkingConnectionState_Dead:
  case k_ESteamNetworkingConnectionState__Force32Bit:
  {
    SocketAddress address(pInfo->m_info.m_addrRemote);
    throw std::runtime_error(
        std::format("Unexpected connection state {} for {}",
                    (int)pInfo->m_info.m_eState, address.to_string()));
  }
  break;
  }
}

AtlasNet::ConnectionState
AtlasNet::MessageSystem::GetConnectionState(const SocketAddress& address) const
{
  std::shared_lock lock(_mutex);

  auto connectionIt = _connections.find(address);
  if (connectionIt != _connections.end())
  {
    const HSteamNetConnection handle = connectionIt->second.GetHandle();
    SteamNetConnectionInfo_t info;
    if (handle != k_HSteamNetConnection_Invalid &&
        GNS().GetConnectionInfo(handle, &info))
    {
      return static_cast<ConnectionState>(info.m_eState);
    }
  }

  return ConnectionState::eNone;
}

bool AtlasNet::MessageSystem::IsConnectingTo(const SocketAddress& address) const
{
  return GetConnectionState(address) == ConnectionState::eConnecting;
}

bool AtlasNet::MessageSystem::IsConnectedTo(const SocketAddress& address) const
{
  return GetConnectionState(address) == ConnectionState::eConnected;
}

AtlasNet::JobHandle
AtlasNet::MessageSystem::Connect(const SocketAddress& address)
{

  {
    std::shared_lock lock(_mutex);

    // Reuse an existing connect job if one is already tracked.
    auto existingJobIt = _connectJobs.find(address);
    if (existingJobIt != _connectJobs.end())
    {
      return existingJobIt->second;
    }

    // If already connected, return a completed no-op job so callers can
    // still safely chain on_complete() if they want.
    auto connIt = _connections.find(address);
    if (connIt != _connections.end() &&
        connIt->second.GetState() == ConnectionState::eConnected)
    {
      return config_.jobSystem->Submit(
          [](JobContext&) {},
          JobOpts::Name(std::format("MessageSystem::AlreadyConnected {}",
                                    address.to_string())),
          JobOpts::Notify<JobNotifyLevel::eNone>(),
          JobOpts::TPriority<JobPriority::eHigh>{});
    }
  }

  // Submit outside any long-lived write logic? Normally yes.
  // But to avoid duplicate jobs for the same address, we still need to
  // serialize the "check + insert" with a unique lock before returning.
  std::unique_lock lock(_mutex);

  // Double-check after upgrading.
  auto existingJobIt = _connectJobs.find(address);
  if (existingJobIt != _connectJobs.end())
  {
    return existingJobIt->second;
  }

  auto connIt = _connections.find(address);
  if (connIt != _connections.end() &&
      connIt->second.GetState() == ConnectionState::eConnected)
  {
    lock.unlock();
    return config_.jobSystem->Submit(
        [](JobContext&) {},
        JobOpts::Name(std::format("MessageSystem::AlreadyConnected {}",
                                  address.to_string())),
        JobOpts::Notify<JobNotifyLevel::eNone>(),
        JobOpts::TPriority<JobPriority::eHigh>{});
  }

  JobHandle handle = config_.jobSystem->Submit(
      [this, address](JobContext& ctx)
      {
        _Connect_to_job(ctx, address);

        // Clean up tracked connect jobs once the connection reaches
        // a terminal or ready state.
        const ConnectionState state = GetConnectionState(address);
        if (state == ConnectionState::eConnected ||
            state == ConnectionState::eClosedByPeer ||
            state == ConnectionState::eProblemDetectedLocally ||
            state == ConnectionState::eNone)
        {
          std::unique_lock cleanupLock(_mutex);
          _connectJobs.erase(address);
          std::cerr << std::format(
                           "Connect job for {} completed with state {}. "
                           "Cleaned up tracked job.",
                           address.to_string(),
                           boost::describe::enum_to_string(state, "UNKNOWN"))
                    << std::endl;
        }
      },
      JobOpts::TPriority<JobPriority::eHigh>{},
      JobOpts::Notify<JobNotifyLevel::eOnStartAndComplete>{},
      JobOpts::Name(
          std::format("MessageSystem::ConnectTo {}", address.to_string())));

  _connectJobs[address] = handle;
  return handle;
}

void AtlasNet::MessageSystem::Shutdown()
{
  if (shutdown.exchange(true, std::memory_order_acq_rel))
  {
    return;
  }
  _pollJobHandle->wait();
  std::cerr << "Shutting down MessageSystem..." << std::endl;

  // If your JobSystem supports cancellation, do it here.
  // Example:
  // _pollJobHandle.Cancel();
  //
  // And also cancel/wait any connect jobs in _connectJobs.

  std::vector<HSteamNetConnection> connectionHandles;
  std::vector<HSteamListenSocket> listenHandles;

  {
    std::unique_lock lock(_mutex);

    connectionHandles.reserve(_connections.size());
    for (const auto& [address, connection] : _connections)
    {
      (void)address;
      connectionHandles.push_back(connection.GetHandle());
    }
    _connections.clear();

    listenHandles.reserve(_listenSockets.size());
    for (const auto& [port, listenSocket] : _listenSockets)
    {
      (void)port;
      if (listenSocket)
      {
        listenHandles.push_back(listenSocket->handle);
      }
    }
    _listenSockets.clear();

    _connectJobs.clear();
  }

  for (HSteamNetConnection handle : connectionHandles)
  {
    if (handle != k_HSteamNetConnection_Invalid)
    {
      GNS().CloseConnection(handle, 0, "Shutting down", false);
    }
  }

  for (HSteamListenSocket handle : listenHandles)
  {
    if (handle != k_HSteamListenSocket_Invalid)
    {
      GNS().CloseListenSocket(handle);
    }
  }

  if (_pollGroup != k_HSteamNetPollGroup_Invalid)
  {
    GNS().DestroyPollGroup(_pollGroup);
    _pollGroup = k_HSteamNetPollGroup_Invalid;
  }

  GameNetworkingSockets_Kill();
  std::cerr << "MessageSystem shutdown complete." << std::endl;
}

AtlasNet::MessageSystem::~MessageSystem()
{
  Shutdown();
}

void AtlasNet::MessageSystem::SetIdentity(
    const SteamNetworkingIdentity& identity)
{
  GNS().ResetIdentity(&identity);
}

void AtlasNet::MessageSystem::ListenSocketHandle::DispatchCallbacks(
    const IMessage& message, MessageIDHash typeIdHash,
    const SocketAddress& caller_address)
{
  HandlerFunc dispatcher;
  bool found = false;

  {
    std::shared_lock lock(socket_mutex);
    auto it = _handlers.find(typeIdHash);
    if (it != _handlers.end())
    {
      dispatcher = it->second;
      found = true;
    }
  }

  if (found)
  {
    std::cerr << std::format("Dispatching message of type hash {} received on "
                             "listen socket port {} to socket dispatcher\n",
                             typeIdHash, port)
              << std::endl;
    dispatcher(message, caller_address);
  }
  else
  {
    std::cerr << std::format(
        "No dispatcher registered for message type with hash {}\n", typeIdHash);
  }
}
void AtlasNet::MessageSystem::MessageSystem::_Parse_Incoming_Messages()
{

  ISteamNetworkingMessage* pIncomingMessages[32] = {};
  const int numMsgs = GNS().ReceiveMessagesOnPollGroup(
      _pollGroup, pIncomingMessages,
      static_cast<int>(std::size(pIncomingMessages)));

  if (numMsgs < 0)
  {
    std::cerr << "ReceiveMessagesOnPollGroup failed" << std::endl;
    return;
  }

  for (int i = 0; i < numMsgs; ++i)
  {
    ISteamNetworkingMessage* msg = pIncomingMessages[i];
    if (!msg)
      continue;

    SteamNetConnectionInfo_t info;
    if (!GNS().GetConnectionInfo(msg->m_conn, &info))
    {
      std::cerr << "Failed to get connection info for incoming message"
                << std::endl;
      msg->Release();
      continue;
    }

    std::optional<PortType> ListenSocketPortReceivedOn = std::nullopt;
    if (info.m_hListenSocket != k_HSteamListenSocket_Invalid)
    {
      SteamNetworkingIPAddr listenAddr;
      if (GNS().GetListenSocketAddress(info.m_hListenSocket, &listenAddr))
      {
        ListenSocketPortReceivedOn = listenAddr.m_port;
      }
    }

    std::optional<SocketAddress> addressRemote;
    if (info.m_addrRemote.IsIPv4())
    {
      const uint32 ip4Packed = info.m_addrRemote.GetIPv4();
      const uint16 port = info.m_addrRemote.m_port;
      addressRemote = SocketAddress(IPv4(ip4Packed), port);
    }
    else
    {
      addressRemote = SocketAddress(IPv6(info.m_addrRemote.m_ipv6),
                                    info.m_addrRemote.m_port);
    }
    std::cout << std::format("Incoming message from {}",
                             addressRemote->to_string())
              << std::endl;

    AN_ASSERT(addressRemote->IsValid(),
              "Invalid remote address in incoming message");

    ByteReader readerID(std::span(static_cast<const uint8_t*>(msg->m_pData),
                                  static_cast<std::size_t>(msg->m_cbSize)));

    const MessageIDHash typeIdHash = IMessage::DeserializeTypeIdHash(readerID);
    std::cout << std::format("Message of type hash: {} {}", typeIdHash,
                             ListenSocketPortReceivedOn.has_value()
                                 ? std::format("on listen socket port {}",
                                               *ListenSocketPortReceivedOn)
                                 : "on client port")
              << std::endl;

    // Copy dispatcher out while holding the lock, then invoke
    // unlocked.
    DispatchFunc dispatcher;
    bool found = false;
    {
      std::shared_lock lock(_mutex);
      auto it = _dispatchTable.find(typeIdHash);
      if (it != _dispatchTable.end())
      {
        dispatcher = it->second;
        found = true;
      }
    }

    if (found)
    {
      std::vector<uint8_t> bytes;
      bytes.assign(static_cast<const uint8_t*>(msg->m_pData),
                   static_cast<const uint8_t*>(msg->m_pData) +
                       static_cast<std::size_t>(msg->m_cbSize));

      config_.jobSystem->Submit(
          [this, dispatcher = dispatcher, bytes = std::move(bytes),
           addressRemote = *addressRemote,
           port_received_on = ListenSocketPortReceivedOn](JobContext&)
          {
            ByteReader readerFull(bytes);
            AN_ASSERT(dispatcher != nullptr,
                      "Dispatcher should not be null here");
            dispatcher(readerFull, addressRemote, port_received_on);
          },
          JobOpts::Name(
              std::format("MessageSystem::DispatchMessageArrivalEvent "
                          "typeHash {} from {}",
                          typeIdHash, addressRemote->to_string())),
          JobOpts::TPriority<JobPriority::eHigh>{},
          JobOpts::Notify<JobNotifyLevel::eOnStartAndComplete>{});
    }

    msg->Release();
  }
}
HSteamNetConnection
AtlasNet::MessageSystem::GetConnectionHandle(const SocketAddress& address) const
{
  std::shared_lock lock(_mutex);

  if (_connections.contains(address))
  {
    return _connections.at(address).GetHandle();
  }
  std::cerr << "Connection handle requested for non-existent connection to "
            << address.to_string() << std::endl;
  std::cerr << "Known connections:\n";
  for (const auto& [addr, conn] : _connections)
  {
    std::cerr << " - " << addr.to_string() << std::endl;
  }
  throw std::runtime_error("Connection not found");
}
void AtlasNet::MessageSystem::_Connect_to_job(JobContext& handle,
                                              const SocketAddress& address)
{

  if (shutdown.load(std::memory_order_acquire))
  {
    std::cerr << "Aborting connection attempt to " << address.to_string()
              << " due to shutdown signal." << std::endl;
    return;
  }

  const ConnectionState state = GetConnectionState(address);

  if (state != ConnectionState::eConnected &&
      state != ConnectionState::eConnecting)
  {
    std::cerr << std::format(
                     "Starting connection attempt to {} from state {}\n",
                     address.to_string(),
                     boost::describe::enum_to_string(state, "UNKNOWN"))
              << std::endl;
    SteamNetworkingConfigValue_t opts[2];
    opts[0].SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged,
                   (void*)&OnSteamNetConnectionStatusChanged);
    opts[1].SetInt64(k_ESteamNetworkingConfig_ConnectionUserData,
                     (int64_t)this);

    std::cerr << "Before ToSteamAddr\n";
    SteamNetworkingIPAddr steamAddr = address.ToSteamAddr();
    std::cerr << "After ToSteamAddr\n";

    std::cerr << "Before ConnectByIPAddress\n";
    const HSteamNetConnection con =
        GNS().ConnectByIPAddress(steamAddr, 2, opts);
    std::cerr << "After ConnectByIPAddress\n";
    if (con == k_HSteamNetConnection_Invalid)
    {
      char errMsg[1024] = {};
      steamAddr.ToString(errMsg, sizeof(errMsg), true);
      std::cerr << "Failed to connect to " << errMsg << std::endl;
      return;
    }

    if (!GNS().SetConnectionPollGroup(con, _pollGroup))
    {
      GNS().CloseConnection(con, 0, "Failed to assign poll group", false);
      std::cerr << "Failed to assign outgoing connection to poll group"
                << std::endl;
      return;
    }

    {
      std::cerr
          << "Getting MessageSystem Lock to track new outgoing connection to "
          << address.to_string() << std::endl;
      std::unique_lock lock(_mutex);
      Connection conn(*this, con);
      conn.connState = ConnectionState::eConnecting;

      auto it = _connections.find(address);
      if (it == _connections.end())
      {
        _connections.emplace(address, std::move(conn));
      }
      else
      {
        it->second.connState = ConnectionState::eConnecting;
      }
      std::cerr << "Tracked new outgoing connection to " << address.to_string()
                << std::endl;
    }

    std::cerr << "Initiated connection to " << address.to_string() << std::endl;

    handle.repeat_once(std::chrono::milliseconds(1000 / Env::TickRate));
    return;
  }

  if (state == ConnectionState::eConnecting)
  {
    std::cerr << std::format(
                     "Still connecting to {}, will check again in {} ms\n",
                     address.to_string(), 1000 / Env::TickRate)
              << std::endl;
    handle.repeat_once(std::chrono::milliseconds(1000 / Env::TickRate));
  }
  else if (state == ConnectionState::eConnected)
  {
    std::cerr << std::format("Successfully connected to {}\n",
                             address.to_string())
              << std::endl;
  }
}
std::optional<AtlasNet::MessageSystem::Connection>
AtlasNet::MessageSystem::GetConnection(const SocketAddress& address) const
{
  std::shared_lock lock(_mutex);
  if (_connections.contains(address))
  {
    return _connections.at(address);
  }
  return std::nullopt;
}
void AtlasNet::MessageSystem::GetConnections(
    std::unordered_map<SocketAddress, Connection>& connections) const
{
  std::shared_lock lock(_mutex);
  connections = _connections;
}
size_t AtlasNet::MessageSystem::GetNumConnections() const
{
  std::shared_lock lock(_mutex);
  return _connections.size();
}
AtlasNet::MessageSystem::ListenSocketHandle::ListenSocketHandle(
    MessageSystem& system, HSteamListenSocket handle, PortType port)
    : system(system), handle(handle), port(port)
{
}
void AtlasNet::MessageSystem::Connection::SendMessage(
    const void* data, uint32_t size, MessageSendMode mode) const
{
  system.GNS().SendMessageToConnection(handle, data, size,
                                       static_cast<int>(mode), nullptr);
}
bool AtlasNet::MessageSystem::attempt_handshake(
    const SteamNetworkingIdentity& identity,
    HandshakeIdentity& handshakeIdentity)
{
  if (config_.handshakeHandler)
  {
    if (identity.IsInvalid())
    {
      std::cerr << "Received connection with invalid identity, rejecting.\n";
      return false;
    }

    HandshakeIdentity handshakeIdentity;
    try
    {
      int cbLen;
      const uint8_t* bytes = identity.GetGenericBytes(cbLen);
      ByteReader reader(std::span<const uint8_t>(bytes, cbLen));
      handshakeIdentity.Deserialize(reader);
    }
    catch (const std::exception& e)
    {
      std::cerr << "Failed to parse handshake identity: " << e.what()
                << std::endl;
      return false;
    }
    HandshakeResponsePacket response =
        config_.handshakeHandler(handshakeIdentity, SocketAddress());
    return response.accepted;
  }
  return true; // accept by default if no handler provided
}
void AtlasNet::MessageSystem::OnConnectionStatus_Connecting(
    SteamNetConnectionStatusChangedCallback_t* pInfo)
{
  SocketAddress address(pInfo->m_info.m_addrRemote);
  std::cerr << "k_ESteamNetworkingConnectionState_Connecting: "
            << address.to_string() << std::endl;
  const bool isIncoming =
      (pInfo->m_info.m_hListenSocket != k_HSteamListenSocket_Invalid);

  /* ==================== EVENTS ==================*/
  if (config_.localEventSystem)
  {
    if (isIncoming)
    {
      SteamNetworkingIPAddr localAddr;
      GNS().GetListenSocketAddress(pInfo->m_info.m_hListenSocket, &localAddr);
      SocketAddress remoteAddr(pInfo->m_info.m_addrRemote);
      ConnectionRequestReceivedEvent event;
      event.remoteAddr = remoteAddr;
      event.localPort = localAddr.m_port;
      config_.localEventSystem->Emit(event);
    }
    else
    {
      ConnectionStartedInternallyEvent event;
      SocketAddress remoteAddr(pInfo->m_info.m_addrRemote);
      event.address = remoteAddr;
      config_.localEventSystem->Emit(event);
    }
  }
  std::optional<HandshakeIdentity> remoteHandshakeIdentity;
  /* =================  HANDSHAKE ================== */
  if (config_.handshakeHandler)
  {
    // only check for handshake if incoming because if we initiated then the
    // identity is not available to us yet
    if (isIncoming)
    {

      /* if the identity of the other is invalid then reject*/
      if (pInfo->m_info.m_identityRemote.IsInvalid() && isIncoming)
      {
        std::cerr << std::format(
                         "WARNING: Incoming connection from {} has invalid "
                         "identity. Rejecting connection.\n",
                         address.to_string())
                  << std::endl;
        GNS().CloseConnection(pInfo->m_hConn,
                              (int)HandshakeResponseCode::eReject,
                              "Invalid identity", false);
        return;
      }
      else /*otherwise try parse*/
      {

        try
        {
          int cbLen = 0;
          const uint8_t* handshakeData =
              pInfo->m_info.m_identityRemote.GetGenericBytes(cbLen);
          ByteReader reader(std::span<const uint8_t>(handshakeData, cbLen));
          remoteHandshakeIdentity.emplace();
          remoteHandshakeIdentity->Deserialize(reader);
        }
        catch (const std::exception& e)
        {
          std::cerr
              << std::format(
                     "Failed to parse handshake identity packet from {}: {}. "
                     "Rejecting connection.\n",
                     address.to_string(), e.what())
              << std::endl;
          GNS().CloseConnection(pInfo->m_hConn,
                                (int)HandshakeResponseCode::eReject,
                                "Failed to parse handshake data", false);
          remoteHandshakeIdentity.reset();
          return;
        }
        /* call the handshake handler*/
        if (HandshakeResponsePacket response =
                config_.handshakeHandler(*remoteHandshakeIdentity, address);
            !response.accepted)
        {
          std::cerr << std::format("Handshake rejected for connection from {}. "
                                   "Closing connection. Reason: {}",
                                   address.to_string(), response.rejectReason)
                    << std::endl;
          GNS().CloseConnection(pInfo->m_hConn,
                                (int)HandshakeResponseCode::eReject,
                                response.rejectReason.c_str(), false);
          return;
        }
        else
        {
          std::cerr << std::format("Handshake accepted for connection from {}. "
                                   "Proceeding with connection.",
                                   address.to_string())
                    << std::endl;
        }
      }
    }
  }

  /* Accept the connection*/
  if (isIncoming)
  {

    GNS().SetConnectionUserData(pInfo->m_hConn, (int64)this);
    std::cout << "Accepting incoming connection: "
              << pInfo->m_info.m_szConnectionDescription << std::endl;
    const EResult r = GNS().AcceptConnection(pInfo->m_hConn);
    if (r != k_EResultOK)
    {
      std::cout << "AcceptConnection failed: " << static_cast<int>(r) << " for "
                << pInfo->m_info.m_szConnectionDescription << std::endl;

      GNS().CloseConnection(pInfo->m_hConn, 0, "AcceptConnection failed",
                            false);
      return;
    }

    if (!GNS().SetConnectionPollGroup(pInfo->m_hConn, _pollGroup))
    {
      std::cerr << "Failed to assign accepted connection to poll group"
                << std::endl;
      GNS().CloseConnection(pInfo->m_hConn, 0, "Failed to assign poll group",
                            false);
      return;
    }

    {
      std::unique_lock lock(_mutex);

      Connection conn(*this, pInfo->m_hConn);
      conn.authState = AuthState::eUnknown;
      conn.connState = ConnectionState::eConnecting;
      conn.handshakeIdentity = remoteHandshakeIdentity;

      auto it = _connections.find(address);
      if (it == _connections.end())
      {
        _connections.emplace(address, std::move(conn));
      }
      else
      {
        it->second.connState = ConnectionState::eConnecting;
      }
    }
    if (config_.localEventSystem)
    {
      ConnectionAcceptedInternallyPreHandshakeEvent event;
      event.address = address;
      config_.localEventSystem->Emit(event);
    }
  }
  else
  {
    std::cout << "Outgoing connection (initiated locally): "
              << pInfo->m_info.m_szConnectionDescription << std::endl;
  }
}
void AtlasNet::MessageSystem::OnConnectionStatus_Connected(
    SteamNetConnectionStatusChangedCallback_t* pInfo)
{
  SocketAddress address(pInfo->m_info.m_addrRemote);
  std::cerr << "k_ESteamNetworkingConnectionState_Connected: "
            << address.to_string() << std::endl;
  const bool isIncoming =
      (pInfo->m_info.m_hListenSocket != k_HSteamListenSocket_Invalid);

  if (!isIncoming)
  {
    HandshakeIdentity handshakeIdentity;
    bool handshakeresult =
        attempt_handshake(pInfo->m_info.m_identityRemote, handshakeIdentity);
    if (!handshakeresult)
    {
      std::cerr << std::format(
                       "Handshake failed for outgoing connection to {}. "
                       "Closing connection.",
                       address.to_string())
                << std::endl;
      GNS().CloseConnection(pInfo->m_hConn, (int)HandshakeResponseCode::eReject,
                            "Handshake failed", false);
      std::unique_lock lock(_mutex);
      auto it = _connections.find(address);
      if (it != _connections.end())
      {
        it->second.connState = ConnectionState::eClosedByLocalHost;
        // it->second.authState = AuthState::eHandshakePending;
      }
      else
      {
        // Defensive: callback may win race against insertion path.
        Connection conn(*this, pInfo->m_hConn);
        conn.connState = ConnectionState::eClosedByLocalHost;
        // conn.authState = AuthState::eHandshakePending;
        _connections.emplace(address, std::move(conn));
      }
      return;
    }
    std::unique_lock lock(_mutex);
    auto it = _connections.find(address);
    if (it != _connections.end())
    {
      it->second.connState = ConnectionState::eConnected;
      it->second.handshakeIdentity = handshakeIdentity;
      // it->second.authState = AuthState::eHandshakePending;
    }
    else
    {
      // Defensive: callback may win race against insertion path.
      Connection conn(*this, pInfo->m_hConn);
      conn.connState = ConnectionState::eConnected;
      conn.handshakeIdentity = handshakeIdentity;
      // conn.authState = AuthState::eHandshakePending;
      _connections.emplace(address, std::move(conn));
    }
  }
  if (config_.localEventSystem)
  {
    std::optional<HandshakeIdentity> handshakeIdentity;
    {
      std::shared_lock lock(_mutex);
      auto it = _connections.find(address);
      if (it != _connections.end())
      {
        handshakeIdentity = it->second.handshakeIdentity;
      }
    }
    ConnectionEstablishedEvent event;
    event.address = address;
    if (config_.handshakeHandler)
    {
      event.source = handshakeIdentity->role == HandshakeRole::eClient
                         ? ConnectionSource::External
                         : ConnectionSource::Internal;
    }
    else
    {
      event.source = ConnectionSource::Unverified;
    }

    config_.localEventSystem->Emit(event);
  }

  std::cout << "Connection connected: "
            << pInfo->m_info.m_szConnectionDescription << std::endl;
}
void AtlasNet::MessageSystem::OnConnectionStatus_ClosedByPeer(
    SteamNetConnectionStatusChangedCallback_t* pInfo)
{
  SocketAddress address(pInfo->m_info.m_addrRemote);
  std::unique_lock lock(_mutex);
  auto it = _connections.find(address);
  if (it != _connections.end())
  {
    it->second.connState = ConnectionState::eClosedByPeer;
  }
  std::cout << "Connection closed by peer: "
            << pInfo->m_info.m_szConnectionDescription << std::endl;
}

void AtlasNet::MessageSystem::OnConnectionStatus_ProblemDetectedLocally(
    SteamNetConnectionStatusChangedCallback_t* pInfo)
{
  SocketAddress address(pInfo->m_info.m_addrRemote);
  std::unique_lock lock(_mutex);
  auto it = _connections.find(address);
  if (it != _connections.end())
  {
    it->second.connState = ConnectionState::eProblemDetectedLocally;
  }
  std::cout << "Problem detected locally: "
            << pInfo->m_info.m_szConnectionDescription << std::endl;
}
