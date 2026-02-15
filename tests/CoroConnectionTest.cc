#include "hayai/coro/AsyncConnection.h"
#include <gtest/gtest.h>

using namespace hayai;
using namespace hayai::coro;

namespace hayai {
namespace test {

class CoroConnectionTest : public ::testing::Test {
protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(CoroConnectionTest, Compilation) { SUCCEED(); }

} // namespace test
} // namespace hayai

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}