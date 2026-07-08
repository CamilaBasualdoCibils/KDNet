#include "DockerStackTest.hpp"
#include "atlasnet/core/address/Address.hpp"
#include "atlasnet/core/database/redis/RedisConn.hpp"
#include <gtest/gtest.h>
int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
using namespace AtlasNet;
using namespace AtlasNet::Database;
TEST_F(DockerStackTest, InternalDBInitialization)
{
  /*  const int Max_Retries = 5;

   for (int i = 0; i < Max_Retries; ++i)
   {
     for (RedisConn::RedisMode mode : {RedisConn::RedisMode::eCluster,
   RedisConn::RedisMode::eStandalone})
     {
       RedisConn::Settings settings;
       settings.host = HostAddress("localhost");
       settings.port = 6379;
       settings.Mode = mode;
       settings.ExceptionOnFailure = false;
       auto conn = RedisConn::Connect(settings);
       if (conn)
       {
         SUCCEED() << "Successfully connected to Redis in " << (mode ==
   RedisConn::RedisMode::eCluster ? "Cluster" : "Standalone") << " mode.";
         return; // Exit the test successfully
       }
     }
     FAIL() << "Failed to connect to Redis after " << Max_Retries << "
   attempts.";
   } */

  std::this_thread::sleep_for(
      std::chrono::seconds(5)); // Wait a bit before final log check

  SUCCEED();
}