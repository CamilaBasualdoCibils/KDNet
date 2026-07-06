
#include <gtest/gtest.h>
#include <random>
#include <unordered_map>

#include "atlasnet/core/Snowflake.hpp"
#include "atlasnet/core/serialize/ByteReader.hpp"
#include "spdlog/logger.h"
#include "spdlog/sinks/stdout_color_sinks.h"
using namespace AtlasNet;
using SnowflakeId = TSnowflake<12, 10, 42>;
int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
auto logger = spdlog::stdout_color_mt("SnowflakeTests");
TEST(Snowflake, DefaultConstructedIsZero)
{
    SnowflakeId id;

    EXPECT_EQ(id.getSequence(), 0);
    EXPECT_EQ(id.getWorker(), 0);
    EXPECT_EQ(id.getTimestamp(), 0);
}
TEST(Snowflake, PackZero)
{
    auto id = SnowflakeId::FromParts(0, 0, 0);

    EXPECT_EQ(id.getTimestamp(), 0);
    EXPECT_EQ(id.getWorker(), 0);
    EXPECT_EQ(id.getSequence(), 0);
}
TEST(Snowflake, PackValues)
{
    auto id = SnowflakeId::FromParts(
        123456789,
        321,
        4095);

    EXPECT_EQ(id.getTimestamp(), 123456789);
    EXPECT_EQ(id.getWorker(), 321);
    EXPECT_EQ(id.getSequence(), 4095);
}
TEST(Snowflake, MaximumValues)
{
    constexpr uint64_t ts =
        (1ULL << 42) - 1;

    constexpr uint64_t worker =
        (1ULL << 10) - 1;

    constexpr uint64_t seq =
        (1ULL << 12) - 1;

    auto id = SnowflakeId::FromParts(ts, worker, seq);

    EXPECT_EQ(id.getTimestamp(), ts);
    EXPECT_EQ(id.getWorker(), worker);
    EXPECT_EQ(id.getSequence(), seq);
}
TEST(Snowflake, OverflowIsMasked)
{
    auto id = SnowflakeId::FromParts(
        UINT64_MAX,
        UINT64_MAX,
        UINT64_MAX);

    EXPECT_EQ(
        id.getTimestamp(),
        (1ULL << 42) - 1);

    EXPECT_EQ(
        id.getWorker(),
        (1ULL << 10) - 1);

    EXPECT_EQ(
        id.getSequence(),
        (1ULL << 12) - 1);
}
TEST(Snowflake, StringRoundTrip)
{
    auto original =
        SnowflakeId::FromParts(123456, 42, 1337);

    auto str = original.toString();
    logger->info("Original ID as string: {}", str);
    auto parsed = SnowflakeId::fromString(str);

    ASSERT_TRUE(parsed.has_value());

    EXPECT_EQ(parsed.value(), original);
}
TEST(Snowflake, ZeroToString)
{
    auto id = SnowflakeId::FromParts(0,0,0);

    SnowflakeId::Str str = id.toString();
    EXPECT_EQ(str.find_first_not_of("0-"), std::string::npos);
}
TEST(Snowflake, MaximumToString)
{
    auto id = SnowflakeId::FromParts(
        (1ULL<<42)-1,
        (1ULL<<10)-1,
        (1ULL<<12)-1);

    auto parsed =
        SnowflakeId::fromString(id.toString());

    ASSERT_TRUE(parsed);
        logger->info("Parsed ID: {}", parsed.value().toString());
    EXPECT_EQ(*parsed, id);
}
TEST(Snowflake, InvalidStringLetters)
{
    EXPECT_FALSE(
        SnowflakeId::fromString("hello"));
}
TEST(Snowflake, EmptyString)
{
    EXPECT_FALSE(
        SnowflakeId::fromString(""));
}
TEST(Snowflake, OverflowString)
{
    EXPECT_FALSE(
        SnowflakeId::fromString(
            "18446744073709551616"));
}
TEST(Snowflake, RejectTrailingCharacters)
{
    EXPECT_FALSE(
        SnowflakeId::fromString("123abc"));
}
TEST(Snowflake, Equality)
{
    auto a = SnowflakeId::FromParts(1,2,3);
    auto b = SnowflakeId::FromParts(1,2,3);

    EXPECT_EQ(a, b);
}
TEST(Snowflake, Inequality)
{
    auto a = SnowflakeId::FromParts(1,2,3);
    auto b = SnowflakeId::FromParts(1,2,4);

    EXPECT_NE(a, b);
}
TEST(Snowflake, Ordering)
{
    auto a = SnowflakeId::FromParts(1,0,0);
    auto b = SnowflakeId::FromParts(2,0,0);

    EXPECT_LT(a, b);
}
TEST(Snowflake, RandomizedPackUnpack)
{
    std::mt19937_64 rng(12345);

    constexpr uint64_t tsMask =
        (1ULL<<42)-1;

    constexpr uint64_t workerMask =
        (1ULL<<10)-1;

    constexpr uint64_t seqMask =
        (1ULL<<12)-1;

    for (int i = 0; i < 100000; ++i)
    {
        auto ts = rng() & tsMask;
        auto worker = rng() & workerMask;
        auto seq = rng() & seqMask;

        auto id =
            SnowflakeId::FromParts(
                ts,
                worker,
                seq);

        EXPECT_EQ(
            id.getTimestamp(),
            ts);

        EXPECT_EQ(
            id.getWorker(),
            worker);

        EXPECT_EQ(
            id.getSequence(),
            seq);
    }
}
TEST(Snowflake, RandomizedStringRoundTrip)
{
    std::mt19937_64 rng(123);

    for (int i = 0; i < 100000; ++i)
    {
        uint64_t value = rng();
        auto id = SnowflakeId(value);

        auto parsed =
            SnowflakeId::fromString(
                id.toString());

        ASSERT_TRUE(parsed);

        EXPECT_EQ(*parsed, id);
        EXPECT_EQ(value,parsed.value().value());

    }
}

TEST(Snowflake, SerializeDeserialize)
{
    auto id = SnowflakeId::FromParts(1, 2, 3);
    ByteWriter bw;
    id.Serialize(bw);
    ByteReader br(bw.bytes());
    SnowflakeId deserialized;
    deserialized.Deserialize(br);
    EXPECT_EQ(id, deserialized);
}