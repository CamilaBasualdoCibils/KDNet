#pragma once

#include "boost/static_string/static_string.hpp"
#include <bitset>
#include <chrono>
#include <cstdint>
namespace AtlasNet
{
using SnowflakeString = boost::static_string<20>;
template <uint64_t SequenceBits, uint64_t WorkerBits, uint64_t TimestampBits>
  requires(SequenceBits + WorkerBits + TimestampBits == 64)
class Snowflake
{

  uint64_t _value;

  constexpr static uint64_t SequenceMask = (1ULL << SequenceBits) - 1;
  constexpr static uint64_t WorkerMask = (1ULL << WorkerBits) - 1;
  constexpr static uint64_t TimestampMask = (1ULL << TimestampBits) - 1;

  constexpr static uint64_t SequenceShift = 0;
  constexpr static uint64_t WorkerShift = SequenceBits;
  constexpr static uint64_t TimestampShift = SequenceBits + WorkerBits;

public:
  class Generator
  {
  public:
    explicit Generator(uint64_t workerId) : m_worker(workerId) {}
    Snowflake Next()
    {
      auto now = CurrentMilliseconds();

      if (now == m_lastTimestamp)
      {
        ++m_sequence;

        if (m_sequence > SequenceMask)
        {
          do
          {
            now = CurrentMilliseconds();
          } while (now == m_lastTimestamp);

          m_lastTimestamp = now;
          m_sequence = 0;
        }
      }
      else
      {
        m_lastTimestamp = now;
        m_sequence = 0;
      }

      return Snowflake::FromParts(now, m_worker, m_sequence);
    }

  private:
    static uint64_t CurrentMilliseconds()
    {
      using namespace std::chrono;

      return duration_cast<milliseconds>(system_clock::now().time_since_epoch())
          .count();
    }
    uint64_t m_worker;
    uint64_t m_sequence = 0;
    uint64_t m_lastTimestamp = 0;
  };
  Snowflake() : _value(0) {}
  ~Snowflake() = default;
  explicit Snowflake(uint64_t value) : _value(value) {}

protected:
public:
  [[nodiscard]] constexpr uint64_t value() const
  {
    return _value;
  }
  [[nodiscard]] constexpr uint64_t getSequence() const
  {
    return (_value >> SequenceShift) & SequenceMask;
  }
  [[nodiscard]] constexpr uint64_t getWorker() const
  {
    return (_value >> WorkerShift) & WorkerMask;
  }
  [[nodiscard]] constexpr uint64_t getTimestamp() const
  {
    return (_value >> TimestampShift) & TimestampMask;
  }
  [[nodiscard]]
  static constexpr Snowflake FromParts(uint64_t timestamp, uint64_t worker,
                                       uint64_t sequence)
  {
    return Snowflake(((timestamp & TimestampMask) << TimestampShift) |
                     ((worker & WorkerMask) << WorkerShift) |
                     ((sequence & SequenceMask) << SequenceShift));
  }

  constexpr auto operator<=>(const Snowflake&) const = default;

  /**
   * @brief 20 because the maximum length of the string representation of a
   * 64-bit integer is 20 characters.
   *
   * @return boost::static_string<20>
   */
  SnowflakeString toString() const
  {
    boost::static_string<20> str;

    auto [ptr, ec] =
        std::to_chars(str.data(), str.data() + str.capacity(), value());

    assert(ec == std::errc{});

    str.resize(ptr - str.data());

    return str;
  }
  static std::optional<Snowflake> fromString(std::string_view str)
  {
    uint64_t value{};

    auto [ptr, ec] =
        std::from_chars(str.data(), str.data() + str.size(), value);

    if (ec != std::errc{} || ptr != str.data() + str.size())
      return std::nullopt;

    return Snowflake(value);
  }
};

}; // namespace AtlasNet
namespace std
{
    template <
        uint64_t SequenceBits,
        uint64_t WorkerBits,
        uint64_t TimestampBits>
    struct hash<AtlasNet::Snowflake<SequenceBits, WorkerBits, TimestampBits>>
    {
        constexpr std::size_t operator()(
            const AtlasNet::Snowflake<SequenceBits, WorkerBits, TimestampBits>& id) const noexcept
        {
            return std::hash<uint64_t>{}(id.value());
        }
    };
}