
#include "atlasnet/core/events/IEvent.hpp"
#include "atlasnet/core/tasks/TaskSystem.hpp"
#include <atlasnet/core/events/LocalEventSystem.hpp>
#include <atomic>
#include <chrono>
#include <gtest/gtest.h>
using namespace AtlasNet;

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

ATLASNET_EVENT(TestEvent, ATLASNET_EVENT_FIELD(int, value));
TEST(EventLocalSystemTest, BasicEventEmission)
{
  TaskSystem jobSystem({});
  LocalEventSystem eventSystem({LocalEventSystem::Config{&jobSystem}});
  std::atomic_bool callbackInvoked = false;

  eventSystem.On<TestEvent>(
      [&](const TestEvent& event)
      {
        callbackInvoked = true;
        EXPECT_EQ(event.value, 42);
      });

  TestEvent event(42);
  eventSystem.Emit(event)->wait_for(std::chrono::seconds(1));
  EXPECT_TRUE(callbackInvoked);
}

TEST(EventLocalSystemTest, MultipleListeners)
{
  TaskSystem jobSystem({});
  LocalEventSystem eventSystem({&jobSystem});
  std::atomic_int callbackCount = 0;


  eventSystem.On<TestEvent>(
      [&](const TestEvent& event)
      {
        callbackCount++;
        EXPECT_EQ(event.value, 42);
      });

  eventSystem.On<TestEvent>(
      [&](const TestEvent& event)
      {
        callbackCount++;
        EXPECT_EQ(event.value, 42);
      });

  TestEvent event(42);
  eventSystem.Emit(event)->wait_for(std::chrono::seconds(1));
  EXPECT_EQ(callbackCount.load(), 2);
}
TEST(EventLocalSystemTest, NoListeners)
{
  TaskSystem jobSystem({});
  LocalEventSystem eventSystem({&jobSystem});



  TestEvent event(42);
  // Should not crash or throw even if there are no listeners
  eventSystem.Emit(event)->wait_for(std::chrono::seconds(1));
  SUCCEED();
}