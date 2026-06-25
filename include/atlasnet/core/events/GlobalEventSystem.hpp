#pragma once

#include "atlasnet/core/database/redis/Redis.hpp"
#include "atlasnet/core/events/IEvent.hpp"
#include "atlasnet/core/events/IEventSystem.hpp"
#include "atlasnet/core/job/JobHandle.hpp"
#include "atlasnet/core/job/JobOptions.hpp"
#include "atlasnet/core/job/JobSystem.hpp"
#include "enviroment/Enviroment.hpp"
#include "sw/redis++/async_subscriber.h"
#include "sw/redis++/subscriber.h"

#include <atomic>
#include <condition_variable>
#include <exception>
#include <functional>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace AtlasNet
{

class GlobalEventSystem : public IEventSystem
{
public:
  struct Config
  {
    Database::RedisConn* _redisConn;
    JobSystem* _jobSystem;
  };

  explicit GlobalEventSystem(const Config& config)
      : _redisConn(config._redisConn),
        _redisSubscriber(_redisConn->AsyncSubscribe()),
        _jobSystem(config._jobSystem), _running(true)
  {
    _redisSubscriber.on_meta(
        [this](sw::redis::Subscriber::MsgType type,
           const std::optional<std::string>& channel, long long count)
        {
          logger->info("Received meta event type: {} on channel: {} with subscription count: {}",
                       static_cast<int>(type),
                       channel.has_value() ? channel.value() : "<none>",
                       count);
         
        });

        _redisSubscriber.on_error([this](std::exception_ptr err )
        {
            try
            {
                if (err)
                {
                    std::rethrow_exception(err);
                }
            }
            catch (const std::exception& e)
            {
              logger->error("Redis subscriber error: {}", e.what());
            }
        });
    _redisSubscriber.on_message(
        [this](const std::string& channel, const std::string& message)
        { handleMessage(channel, message); });

    /* subscriberThread = std::jthread(
        [this](std::stop_token st)
        {
          std::unique_lock lock(_mutex);

          _subCv.wait(lock, st, [this] { return _hasSubscription; });

          lock.unlock();

          while (!st.stop_requested())
          {
            _redisSubscriber.consume();
          }
        }); */
  }

  ~GlobalEventSystem()
  {
    shutdown();
  }

protected:
  void impl_On(EventID eventID,
               std::function<void(const std::string_view&)> cb) override
  {
    logger->info("Registering Global listener for event ID {}", eventID);
    std::unique_lock lock(_mutex);

    std::string channel = channelForEvent(eventID);

    if (subscribedChannels.contains(channel))
    {
      listeners[eventID].push_back(std::move(cb));
      return;
    }
    subscribedChannels.insert(channel);
    std::future<void> subfut = _redisSubscriber.subscribe(channel);
    subfut.wait();

    listeners[eventID].push_back(std::move(cb));
    _hasSubscription = true;
    lock.unlock();
    _subCv.notify_all();
  }

  JobHandle impl_Emit(EventID eventID, const std::string_view& data) override
  {
    logger->info("Emitting Global event with ID {}", eventID);
    std::string payload(data);
    std::string channel = channelForEvent(eventID);

    return _jobSystem->Submit([this, channel = std::move(channel),
                               payload = std::move(payload)](JobContext&)
                              { _redisConn->Publish(channel, payload); });
  }

private:
  void handleMessage(const std::string& channel, const std::string& message)
  {
    if (!_running.load(std::memory_order_acquire))
      return;

    EventID eventID = extractEventIDFromChannel(channel);

    // Snapshot listeners under lock
    std::vector<std::function<void(const std::string_view&)>> callbacks;

    {
      std::shared_lock lock(_mutex);

      auto it = listeners.find(eventID);
      if (it == listeners.end())
        return;

      callbacks = it->second; // copy snapshot (safe iteration)
    }

    std::string_view view(message);

    // IMPORTANT: no locks held during callback execution
    for (auto& cb : callbacks)
    {
      cb(view);
    }
  }

  std::string channelForEvent(EventID eventID) const
  {
    return EventChannelPrefix + std::to_string(eventID);
  }

  EventID extractEventIDFromChannel(const std::string& channel) const
  {
    if (channel.rfind(EventChannelPrefix, 0) != 0)
      return 0;

    std::string_view suffix(channel.data() + EventChannelPrefix.size(),
                            channel.size() - EventChannelPrefix.size());

    return static_cast<EventID>(std::stoull(std::string(suffix)));
  }

  void shutdown()
  {
    bool expected = true;
    if (!_running.compare_exchange_strong(expected, false))
      return;

    {
      std::unique_lock lock(_mutex);

      for (const auto& channel : subscribedChannels)
      {
        _redisSubscriber.unsubscribe(channel);
      }
      subscribedChannels.clear();
      listeners.clear();
    }
    //shouldShutdown = true;
    //subscriberThread.join();
    {
    }
  }

private:
std::shared_ptr<spdlog::logger> logger = spdlog::stdout_color_mt("GlobalEventSystem");
  Database::RedisConn* _redisConn;
  JobSystem* _jobSystem;
  sw::redis::AsyncSubscriber _redisSubscriber;
  std::condition_variable_any _subCv;
  bool _hasSubscription = false;
  std::atomic_bool shouldShutdown{false};
  std::atomic_bool _running;
  //std::jthread subscriberThread;
  std::shared_mutex _mutex;

  std::unordered_set<std::string> subscribedChannels;

  std::unordered_map<EventID,
                     std::vector<std::function<void(const std::string_view&)>>>
      listeners;

  static inline const std::string EventChannelPrefix =
      Env::DatabaseNamespace + "events:";
  static inline const std::string EventChannelPostFix = ":Subs";
};

} // namespace AtlasNet