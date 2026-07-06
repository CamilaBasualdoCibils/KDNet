#pragma once
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <unordered_map>
namespace AtlasNet
{
template <typename Key, typename Value> class Cache
{
public:
  void Insert(Key key, Value value)
  {
    std::unique_lock lock(mutex_);
    data_[key] = value;
  }
  std::optional<Value> Find(const Key& key)
  {
    std::shared_lock lock(mutex_);
    auto it = data_.find(key);
    if (it != data_.end())
    {
      return it->second;
    }
    return std::nullopt;
  }
  Value FindOrValue(const Key& key, const Value& defaultValue)
  {
    std::optional<Value> result = Find(key);
    if (result)
    {
      return *result;
    }
    return defaultValue;
  }
  template <typename DefaultValueProvider>
  requires std::is_invocable_r_v<std::optional<Value>, DefaultValueProvider, const Key&>
  std::optional<Value> FindEnsured(const Key& key, DefaultValueProvider defaultValueProvider)
  {
    std::optional<Value> result = Find(key);
    if (result)
    {
      return result;
    }
    std::optional<Value> defaultValue = defaultValueProvider(key);
    Insert(key, *defaultValue);
    return defaultValue;
  }
  void Erase(const Key& key)
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
};
} // namespace AtlasNet