
#include "atlasnet/core/CoreDefs.hpp"
#include "atlasnet/core/address/Address.hpp"
#include "atlasnet/core/address/SocketAddress.hpp"
#include "atlasnet/core/client/ClientDataEntry.hpp"
#include "atlasnet/core/serialize/AtlasSerializer.hpp"
#include "atlasnet/core/serialize/BinarySerializer.hpp"
#include "atlasnet/core/serialize/ByteReader.hpp"
#include "atlasnet/core/serialize/ByteWriter.hpp"
#include "atlasnet/core/serialize/XMLSerializer.hpp"
#include "bitsery/serializer.h"
#include "glm/ext/quaternion_relational.hpp"
#include "glm/fwd.hpp"
#include "glm/gtc/epsilon.hpp"

#include <bitset>
#include <cstddef>
#include <cstdint>
#include <gtest/gtest.h>
#include <string>
#include <string_view>

TEST(Serialization, BasicOps)
{
  const uint8_t u8_val = 255;
  const uint16_t u16_val = 65535;
  const uint32_t u32_val = 4294967295;
  const uint64_t u64_val = 18446744073709551615ull;
  const int8_t i8_val = -128;
  const int16_t i16_val = -32768;
  const int32_t i32_val = -2147483648;
  const int64_t i64_val = -9223372036854775807ll - 1;
  const float f32_val = 3.14f;
  const double f64_val = 3.141592653589793;
  const std::string str_val = "Hello, AtlasNet!";
  const glm::vec4 vec4_val(1.0f, 2.0f, 3.0f, 4.0f);
  const uint32_t bits_val = 0b10101010101010101010101010101010;
  const glm::quat quat_val(1.0f, 0.0f, 0.0f, 0.0f);
  const glm::mat4 mat4_val(1.0f);
  const std::vector<uint8_t> blob_val = {0xDE, 0xAD, 0xBE, 0xEF};
  const std::array<uint8_t, 16> array_val = {0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD,
                                             0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF,
                                             0xDE, 0xAD, 0xBE, 0xEF};

  AtlasNet::ByteWriter writer;
  writer.u8(u8_val)
      .u16(u16_val)
      .u32(u32_val)
      .u64(u64_val)
      .i8(i8_val)
      .i16(i16_val)
      .i32(i32_val)
      .i64(i64_val)
      .f32(f32_val)
      .f64(f64_val)
      .str(str_val)
      .write_vector<4>(vec4_val)
      .bits(bits_val, 32)
      .quat(quat_val)
      .mat4(mat4_val)
      .blob(std::span(blob_val))
      .write_any(array_val);

  uint8_t u8_out;
  uint16_t u16_out;
  uint32_t u32_out;
  uint64_t u64_out;
  int8_t i8_out;
  int16_t i16_out;
  int32_t i32_out;
  int64_t i64_out;
  float f32_out;
  double f64_out;
  std::string str_out;
  glm::vec4 vec4_out;
  uint32_t bits_out;
  glm::quat quat_out;
  glm::mat4 mat4_out;
  std::span<const uint8_t> blob_out;
  std::array<uint8_t, 16> array_out;
  AtlasNet::ByteReader reader(writer.bytes());
  reader.u8(u8_out)
      .u16(u16_out)
      .u32(u32_out)
      .u64(u64_out)
      .i8(i8_out)
      .i16(i16_out)
      .i32(i32_out)
      .i64(i64_out)
      .f32(f32_out)
      .f64(f64_out)
      .str(str_out)
      .read_vector<4>(vec4_out)
      .bits(32, bits_out)
      .quat(quat_out)
      .mat4(mat4_out)
      .blob(blob_out)
      .read_any(array_out);

  EXPECT_EQ(u8_out, u8_val);
  EXPECT_EQ(u16_out, u16_val);
  EXPECT_EQ(u32_out, u32_val);
  EXPECT_EQ(u64_out, u64_val);
  EXPECT_EQ(i8_out, i8_val);
  EXPECT_EQ(i16_out, i16_val);
  EXPECT_EQ(i32_out, i32_val);
  EXPECT_EQ(i64_out, i64_val);
  EXPECT_FLOAT_EQ(f32_out, f32_val);
  EXPECT_DOUBLE_EQ(f64_out, f64_val);
  EXPECT_EQ(str_out, str_val);
  EXPECT_TRUE(glm::all(glm::epsilonEqual(vec4_out, vec4_val, 0.0001f)));
  EXPECT_EQ(bits_out, bits_val);
  EXPECT_TRUE(glm::all(glm::epsilonEqual(quat_out, quat_val, 0.0001f)));
  EXPECT_EQ(mat4_out, mat4_val);
  EXPECT_TRUE(std::equal(blob_out.begin(), blob_out.end(), blob_val.begin(),
                         blob_val.end()));
  EXPECT_TRUE(std::equal(array_out.begin(), array_out.end(), array_val.begin(),
                         array_val.end()));
}
struct Object
{
  uint8_t u8_val;
  uint16_t u16_val;
  uint32_t u32_val;
  uint64_t u64_val;
  int8_t i8_val;
  int16_t i16_val;
  int32_t i32_val;
  int64_t i64_val;
  float f32_val;
  double f64_val;
  std::string str_val;
  glm::vec4 vec4_val;
  uint32_t bits_val;
  glm::quat quat_val;
  glm::mat4 mat4_val;
  std::vector<uint8_t> blob_val;
  std::array<uint8_t, 16> array_val;
  template <typename Archive> void serialize(Archive& ar)
  {
    ar(u8_val);
    ar(u16_val);
    ar(u32_val);
    ar(u64_val);
    ar(i8_val);
    ar(i16_val);
    ar(i32_val);
    ar(i64_val);
    ar(f32_val);
    ar(f64_val);
    ar(str_val);
    ar(vec4_val);
    ar(bits_val);
    ar(quat_val);
    ar(mat4_val);
    ar(blob_val);
    ar(array_val);
  }
};
TEST(Serialization, archivetest)
{

  std::vector<uint8_t> blob = {0xDE, 0xAD, 0xBE, 0xEF};
  Object in{255,
            65535,
            4294967295,
            18446744073709551615ull,
            -128,
            -32768,
            -2147483648,
            -9223372036854775807ll - 1,
            3.14f,
            3.141592653589793,
            "Hello, AtlasNet!",
            glm::vec4(1.0f, 2.0f, 3.0f, 4.0f),
            0b10101010101010101010101010101010,
            glm::quat(1.0f, 0.0f, 0.0f, 0.0f),
            glm::mat4(1.0f),
            blob,
            {0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE,
             0xEF, 0xDE, 0xAD, 0xBE, 0xEF}};

  AtlasNet::ByteWriter writer;
  in.serialize(writer);

  Object out;
  AtlasNet::ByteReader reader(writer.bytes());
  out.serialize(reader);
  EXPECT_EQ(out.u8_val, in.u8_val);
  EXPECT_EQ(out.u16_val, in.u16_val);
  EXPECT_EQ(out.u32_val, in.u32_val);
  EXPECT_EQ(out.u64_val, in.u64_val);
  EXPECT_EQ(out.i8_val, in.i8_val);
  EXPECT_EQ(out.i16_val, in.i16_val);
  EXPECT_EQ(out.i32_val, in.i32_val);
  EXPECT_EQ(out.i64_val, in.i64_val);
  EXPECT_FLOAT_EQ(out.f32_val, in.f32_val);
  EXPECT_DOUBLE_EQ(out.f64_val, in.f64_val);
  EXPECT_EQ(out.str_val, in.str_val);
  EXPECT_TRUE(glm::all(glm::epsilonEqual(out.vec4_val, in.vec4_val, 0.0001f)));

  EXPECT_EQ(out.bits_val, in.bits_val);
  EXPECT_TRUE(glm::all(glm::epsilonEqual(out.quat_val, in.quat_val, 0.0001f)));
  EXPECT_EQ(out.mat4_val, in.mat4_val);
  EXPECT_TRUE(std::equal(out.blob_val.begin(), out.blob_val.end(),
                         in.blob_val.begin(), in.blob_val.end()));
  EXPECT_TRUE(std::equal(out.array_val.begin(), out.array_val.end(),
                         in.array_val.begin(), in.array_val.end()));
}
int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
TEST(Serialization, LoginDataSerialization)
{
  using namespace AtlasNet;
  LoginData entry;
  entry.address = SocketAddress(IPv4(127, 0, 0, 1), 8080);
  // entry.clientID;
  // entry.managingGateway = UUID::Generate();
  // entry.entityID = EntityID::Generate();
  ByteWriter writer;
  entry.Serialize(writer);

  ByteReader reader(writer.bytes());
  LoginData deserializedEntry;
  deserializedEntry.Deserialize(reader);
  EXPECT_EQ(entry.address, deserializedEntry.address);
  EXPECT_EQ(entry.clientID, deserializedEntry.clientID);
  EXPECT_EQ(entry.managingGateway, deserializedEntry.managingGateway);
  EXPECT_EQ(entry.entityID, deserializedEntry.entityID);

  _Json j;
  entry.to_json(j);
  std::cerr << j.dump(4) << std::endl;
  SUCCEED();
}

TEST(Serialization, BinarySerializerManual)
{
  {
    AtlasNet::NetBinaryWriter serializer;
    std::vector<uint8_t> data = {1, 2, 3, 4, 5};
    serializer->container1b(data, data.size());

    AtlasNet::NetBinaryReader deserializer(serializer.GetBytes());
    std::vector<uint8_t> output(5);
    deserializer->container1b(output, data.size());

    EXPECT_EQ(data, output);
  }
  {
    std::string str = "Hello, AtlasNet!";
    AtlasNet::NetBinaryWriter serializer;
    serializer->text1b(str, str.size());

    AtlasNet::NetBinaryReader deserializer(serializer.GetBytes());
    std::string output;
    deserializer->text1b(output, str.size());
    EXPECT_EQ(str, output);
  }
}
TEST(Serialization, BinarySerializer)
{
  AtlasNet::NetBinaryWriter serializer;
  std::array<uint8_t, 5> data = {1, 2, 3, 4, 5};
  serializer(data[0], data[1], data[2], data[3], data[4]);

  AtlasNet::NetBinaryReader deserializer(serializer.GetBytes());
  std::array<uint8_t, 5> output;
  deserializer(output[0], output[1], output[2], output[3], output[4]);
  EXPECT_EQ(data, output);
}
TEST(Serialization, XMLSerializer)
{
  AtlasNet::XMLSerializer serializer;
  serializer("Hello, AtlasNet!", 42, 3.14f);
}

TEST(Serialization, BinarySerializer_int)
{
  AtlasNet::NetBinaryWriter serializer;
  int8_t i8_val = -128;
  int16_t i16_val = -32768;
  int32_t i32_val = -2147483648;
  int64_t i64_val = -9223372036854775807ll - 1;
  uint8_t u8_val = 255;
  uint16_t u16_val = 65535;
  uint32_t u32_val = 4294967295;
  uint64_t u64_val = 18446744073709551615ull;
  serializer(i8_val, i16_val, i32_val, i64_val, u8_val, u16_val, u32_val,
             u64_val);

  AtlasNet::NetBinaryReader deserializer(serializer.GetBytes());
  int8_t i8_out;
  int16_t i16_out;
  int32_t i32_out;
  int64_t i64_out;
  uint8_t u8_out;
  uint16_t u16_out;
  uint32_t u32_out;
  uint64_t u64_out;
  deserializer(i8_out, i16_out, i32_out, i64_out, u8_out, u16_out, u32_out,
               u64_out);
  EXPECT_EQ(i8_out, i8_val);
  EXPECT_EQ(i16_out, i16_val);
  EXPECT_EQ(i32_out, i32_val);
  EXPECT_EQ(i64_out, i64_val);
  EXPECT_EQ(u8_out, u8_val);
  EXPECT_EQ(u16_out, u16_val);
  EXPECT_EQ(u32_out, u32_val);
  EXPECT_EQ(u64_out, u64_val);
}
TEST(Serialization, BinarySerializer_float)
{
  AtlasNet::NetBinaryWriter serializer;
  float f32_val = 3.14f;
  double f64_val = 3.141592653589793;
  serializer(f32_val, f64_val);

  AtlasNet::NetBinaryReader deserializer(serializer.GetBytes());
  float f32_out;
  double f64_out;
  deserializer(f32_out, f64_out);
  EXPECT_FLOAT_EQ(f32_out, f32_val);
  EXPECT_DOUBLE_EQ(f64_out, f64_val);
}
TEST(Serialization, BinarySerializer_string)
{
  AtlasNet::NetBinaryWriter serializer;
  std::string str_val = "Hello, AtlasNet!";
  std::u8string u8str_val = u8"Hello, AtlasNet!";
  std::u16string u16str_val = u"Hello, AtlasNet!";
  std::u32string u32str_val = U"Hello, AtlasNet!";
  std::wstring wstr_val = L"Hello, AtlasNet!";
  serializer(str_val, u8str_val, u16str_val, u32str_val, wstr_val);

  AtlasNet::NetBinaryReader deserializer(serializer.GetBytes());
  std::string str_out;
  std::u8string u8str_out;
  std::u16string u16str_out;
  std::u32string u32str_out;
  std::wstring wstr_out;
  deserializer(str_out, u8str_out, u16str_out, u32str_out, wstr_out);
  EXPECT_EQ(str_out, str_val);
  EXPECT_EQ(u8str_out, u8str_val);
  EXPECT_EQ(u16str_out, u16str_val);
  EXPECT_EQ(u32str_out, u32str_val);
  EXPECT_EQ(wstr_out, wstr_val);
}
TEST(Serialization, BinarySerializer_array)
{
  size_t arraySize;
  {
    AtlasNet::NetBinaryWriter serializer;
    std::array<uint8_t, 5> array_val = {1, 2, 3, 4, 5};
    serializer(array_val);
    arraySize = serializer.GetBytes().size_bytes();
    AtlasNet::NetBinaryReader deserializer(serializer.GetBytes());
    std::array<uint8_t, 5> array_out;
    deserializer(array_out);
    EXPECT_EQ(array_out, array_val);
  }
  size_t vectorSize;
  {
    AtlasNet::NetBinaryWriter serializer;
    std::vector<uint8_t> vector_val = {1, 2, 3, 4, 5};
    serializer(vector_val);
    vectorSize = serializer.GetBytes().size_bytes();
    AtlasNet::NetBinaryReader deserializer(serializer.GetBytes());
    std::vector<uint8_t> vector_out;
    deserializer(vector_out);
    EXPECT_EQ(vector_out, vector_val);
  }
  EXPECT_LT(arraySize, vectorSize);
}
TEST(Serialization, BinarySerializer_array_vector_string)
{
  using type = std::vector<std::array<std::string, 5>>;
  AtlasNet::NetBinaryWriter serializer;
  type value = {{"Hello", "AtlasNet", "Test", "Serialization", "Array"},
                {"Hello", "AtlasNet", "Test", "Serialization", "Vector"},
                {"Hello", "AtlasNet", "Test", "Serialization", "String"}};
  serializer(value);
  AtlasNet::NetBinaryReader deserializer(serializer.GetBytes());
  type value_out;
  deserializer(value_out);
  EXPECT_EQ(value_out, value);
}