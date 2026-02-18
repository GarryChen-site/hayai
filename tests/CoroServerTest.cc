#include "hayai/coro/AsyncServer.h"
#include <gtest/gtest.h>

using namespace hayai;
using namespace hayai::coro;

namespace hayai {
namespace test {

class CoroServerTest : public ::testing::Test {
protected:
  void SetUp() override {}
  void TearDown() override {}
};

// Test: AsyncServer creation and basic API
TEST_F(CoroServerTest, ServerCreation) {
  EventLoop loop;
  InetAddress addr(9996);

  AsyncServer server(&loop, addr, "TestServer");

  EXPECT_EQ(server.name(), "TestServer");
  EXPECT_EQ(server.getLoop(), &loop);
}

// Test: Server start
TEST_F(CoroServerTest, ServerStart) {
  EventLoop loop;
  InetAddress addr(9995);

  AsyncServer server(&loop, addr, "TestServer");

  // Should not throw
  EXPECT_NO_THROW(server.start());
}

} // namespace test
} // namespace hayai

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
