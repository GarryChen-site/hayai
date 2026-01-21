#include "hayai/utils/Buffer.h"
#include <cstring>
#include <gtest/gtest.h>
#include <unistd.h>

namespace hayai {
namespace test {

class BufferTest : public ::testing::Test {
protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(BufferTest, AppendAndRetrieve) {
  Buffer buf;
  buf.append("hello", 5);
  EXPECT_EQ(buf.readableBytes(), 5);

  // Test peek interface
  const char *ptr = buf.peek();
  EXPECT_EQ(std::string_view(ptr, buf.readableBytes()), "hello");

  buf.retrieve(2);
  EXPECT_EQ(buf.readableBytes(), 3);
  EXPECT_EQ(buf.retrieveAllAsString(), "llo");
  EXPECT_EQ(buf.readableBytes(), 0);
}

TEST_F(BufferTest, Growth) {
  Buffer buf(10); // Small initial size
  const char *longString = "this is a long string that should trigger growth";
  buf.append(longString, std::strlen(longString));
  EXPECT_EQ(buf.readableBytes(), std::strlen(longString));
  EXPECT_EQ(buf.retrieveAllAsString(), longString);
}

TEST_F(BufferTest, PointerInterface) {
  Buffer buf;
  buf.append("test", 4);

  // Clean const char* access
  const char *bufData = buf.peek();
  EXPECT_EQ(buf.readableBytes(), 4);
  EXPECT_EQ(bufData[0], 't');
  EXPECT_EQ(bufData[3], 't');
}

TEST_F(BufferTest, MakeSpace) {
  Buffer buf(20);

  // Fill buffer
  buf.append("12345678", 8);
  EXPECT_EQ(buf.readableBytes(), 8);

  // Read some data (creates prepend space)
  buf.retrieve(5);
  EXPECT_EQ(buf.readableBytes(), 3);
  EXPECT_EQ(buf.prependableBytes(), Buffer::kCheapPrepend + 5);

  // Append more data - should trigger makeSpace, not resize
  buf.append("abcdefgh", 8);
  EXPECT_EQ(buf.readableBytes(), 11);
  EXPECT_EQ(buf.retrieveAsString(11), "678abcdefgh");
}

TEST_F(BufferTest, FindCRLF) {
  Buffer buf;
  buf.append("hello\r\nworld", 12);

  const char *crlf = buf.findCRLF();
  ASSERT_NE(crlf, nullptr);

  size_t pos = crlf - buf.peek();
  EXPECT_EQ(pos, 5);
}

TEST_F(BufferTest, RetrievePartial) {
  Buffer buf;
  buf.append("abcdef", 6);

  std::string first3 = buf.retrieveAsString(3);
  EXPECT_EQ(first3, "abc");
  EXPECT_EQ(buf.readableBytes(), 3);
  EXPECT_EQ(buf.retrieveAllAsString(), "def");
}

TEST_F(BufferTest, StringViewAppend) {
  Buffer buf;
  std::string_view sv = "test string";
  buf.append(sv);

  EXPECT_EQ(buf.readableBytes(), sv.size());
  EXPECT_EQ(buf.retrieveAllAsString(), sv);
}

TEST_F(BufferTest, Swap) {
  Buffer buf1;
  Buffer buf2;

  buf1.append("buffer1", 7);
  buf2.append("buffer2", 7);

  buf1.swap(buf2);

  EXPECT_EQ(buf1.retrieveAllAsString(), "buffer2");
  EXPECT_EQ(buf2.retrieveAllAsString(), "buffer1");
}

TEST_F(BufferTest, Shrink) {
  // The shrink operation should not break the buffer
  // After shrink, buffer should still be usable
  Buffer buf;
  buf.append(std::string(1000, 'a'));
  buf.retrieveAll();

  // Shrink with reserve space
  buf.shrink(100);

  // Buffer should still work
  buf.append("test", 4);
  EXPECT_EQ(buf.readableBytes(), 4);
  EXPECT_EQ(buf.retrieveAllAsString(), "test");
}

TEST_F(BufferTest, WritableBytesCalculation) {
  Buffer buf(100);

  // Initial state
  EXPECT_EQ(buf.readableBytes(), 0);
  EXPECT_GE(buf.writableBytes(), 100);

  buf.append("test", 4);
  EXPECT_EQ(buf.readableBytes(), 4);

  buf.retrieveAll();
  EXPECT_EQ(buf.readableBytes(), 0);
}

TEST_F(BufferTest, ReadFd) {
  // Test the scatter-gather I/O readFd() implementation
  // Create a pipe to simulate socket reading
  int pipefd[2];
  ASSERT_EQ(pipe(pipefd), 0);

  // Write data to the pipe
  const char *testData =
      "This is test data for readFd scatter-gather I/O optimization";
  size_t dataLen = std::strlen(testData);
  ssize_t written = write(pipefd[1], testData, dataLen);
  ASSERT_EQ(written, static_cast<ssize_t>(dataLen));

  // Read using Buffer::readFd()
  Buffer buf;
  int savedErrno = 0;
  ssize_t n = buf.readFd(pipefd[0], &savedErrno);

  EXPECT_EQ(n, static_cast<ssize_t>(dataLen));
  EXPECT_EQ(buf.readableBytes(), dataLen);
  EXPECT_EQ(buf.retrieveAllAsString(), testData);

  // Close pipe
  close(pipefd[0]);
  close(pipefd[1]);
}

TEST_F(BufferTest, ReadFdWithExtrabuf) {
  // Test that readFd uses extrabuf when buffer is small
  int pipefd[2];
  ASSERT_EQ(pipe(pipefd), 0);

  // Small data that fits easily
  std::string testData(5000, 'X'); // 5KB
  ssize_t written = write(pipefd[1], testData.data(), testData.size());
  ASSERT_EQ(written, static_cast<ssize_t>(testData.size()));

  Buffer buf(100); // Very small buffer forces extrabuf usage
  int savedErrno = 0;
  ssize_t n = buf.readFd(pipefd[0], &savedErrno);

  EXPECT_EQ(n, static_cast<ssize_t>(testData.size()));
  EXPECT_EQ(buf.readableBytes(), testData.size());
  EXPECT_EQ(buf.retrieveAllAsString(), testData);

  // Cleanup
  close(pipefd[0]);
  close(pipefd[1]);
}

TEST_F(BufferTest, EmptyBuffer) {
  Buffer buf;
  EXPECT_EQ(buf.readableBytes(), 0);
  EXPECT_EQ(buf.retrieveAllAsString(), "");

  const char *ptr = buf.peek();
  EXPECT_NE(ptr, nullptr);
}

} // namespace test
} // namespace hayai

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
