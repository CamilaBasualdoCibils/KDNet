#pragma once
#include <functional>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <unordered_map>
namespace AtlasNet
{
template <typename Key, typename Value> class Cache
{
public:
  using DefaultValueProvider = std::function<std::optional<Value>(const Key&)>;
  Cache(DefaultValueProvider defaultValueProvider) : defaultValueProvider_(std::move(defaultValueProvider)) {}
  std::optional<Value> Get(const Key& key)
  {
    std::shared_lock lock(mutex_);
    auto it = data_.find(key);
    if (it != data_.end())
    {
      return it->second;
    }
    if (defaultValueProvider_)
    {
      std::optional<Value> v = defaultValueProvider_(key);
      if (!v)
      {
        return std::nullopt;
      }
      lock.unlock();
      std::unique_lock lock(mutex_);
      data_[key] = *v;
      return v;
    }
    return std::nullopt;
  }
  void Invalidate(const Key& key)
  {
    std::unique_lock lock(mutex_);
    data_.erase(key);
  }
  void Clear()
  {
    std::unique_lock lock(mutex_);
    data_.clear();
  }

  std::unique_lock<std::shared_mutex> GetWriteLock()
  {
    return std::unique_lock<std::shared_mutex>(mutex_);
  }
  std::shared_lock<std::shared_mutex> GetReadLock()
  {
    return std::shared_lock<std::shared_mutex>(mutex_);
  }

private:
  std::unordered_map<Key, Value> data_;
  std::shared_mutex mutex_;

  DefaultValueProvider defaultValueProvider_;
};
} // namespace AtlasNet