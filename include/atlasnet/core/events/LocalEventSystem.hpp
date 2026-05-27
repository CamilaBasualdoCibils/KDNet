#pragma once

#include "IEvent.hpp"
#include "atlasnet/core/events/IEventSystem.hpp"
#include "atlasnet/core/job/JobContext.hpp"
#include "atlasnet/core/job/JobHandle.hpp"
#include "atlasnet/core/job/JobOptions.hpp"
#include "atlasnet/core/job/JobSystem.hpp"
#include "enviroment/Enviroment.hpp"
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
namespace AtlasNet
{
class LocalEventSystem : public IEventSystem
{
public:
  struct Config
  {
    JobSystem* jobSystem;
  };
  explicit LocalEventSystem(const Config& config) : jobSystem(config.jobSystem)
  {
  }

protected:
  void impl_On(EventID eventID,
               std::function<void(const std::string_view&)> cb) override
  {
    std::cerr << "Registering Local listener for event ID " << eventID
              << std::endl;
    std::unique_lock lock(mutex);
    listeners[eventID].push_back(std::move(cb));
  }

  JobHandle impl_Emit(EventID eventID, const std::string_view& event) override
{
  return jobSystem->Submit(
      [this, eventID, eventstr = std::string(event)](JobContext&)
      {
        std::vector<std::function<void(const std::string_view&)>> callbacks;

        {
          std::shared_lock lock(mutex);

          auto it = listeners.find(eventID);
          if (it == listeners.end())
            return;

          callbacks = it->second; // COPY OUT
        }

        // lock is released here

        for (auto& cb : callbacks)
        {
          cb(eventstr);
        }
      });
}

private:
  JobSystem* jobSystem;

  std::shared_mutex mutex;
  std::unordered_map<EventID,
                     std::vector<std::function<void(const std::string_view&)>>>
      listeners;
};
} // namespace AtlasNet
