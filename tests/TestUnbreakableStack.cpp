/*!
 * @file Tests for UnbreakableStack class
 */

#include <gtest/gtest.h>
#include <headers/UnbreakableStack.hpp>

struct TestUnbreakableStack : ::testing::Test {
 protected:
  void SetUp() override {}
  void TearDown() override {}
  bool OkInt() { return int_st.Ok(); }
  bool OkVec() { return vec_st.Ok(); }
  size_t SumInt () { return int_st.CalculateCheckSum(); }
  size_t SumVec () { return vec_st.CalculateCheckSum(); }
  UnbreakableStack<int, Static, DefaultDump<int>, 100> int_st;
  UnbreakableStack<std::vector<int>,
  Static, DefaultDump<std::vector<int>>, 100> vec_st;
};

TEST_F(TestUnbreakableStack, Overflow0) {
  for (size_t i = 0; i < 100; ++i) {
    int_st.Push(i);
  }
  EXPECT_DEATH(int_st.Push(1), ".*");
}

TEST_F(TestUnbreakableStack, Pop0) {
  EXPECT_DEATH(int_st.Pop(), ".*");
}

TEST_F(TestUnbreakableStack, Canary0) {
  size_t tmp = *(size_t*)&int_st;
  *(size_t*)&int_st = 100500;
  EXPECT_FALSE(OkInt());
  *(size_t*)&int_st = tmp;
}

TEST_F(TestUnbreakableStack, Canary1) {
  size_t tmp = *(size_t*)((char*)&int_st + sizeof(int_st) - 1);
  *(size_t*)((char*)&int_st + sizeof(int_st) - 1) = 100500;
  EXPECT_FALSE(OkInt());
  *(size_t*)((char*)&int_st + sizeof(int_st) - 1) = tmp;
}

TEST_F(TestUnbreakableStack, OutOfRange0) {
  size_t tmp = *(size_t*)((char*)&int_st + 100);
  *(size_t*)((char*)&int_st + 100) = 100500;
  EXPECT_FALSE(OkInt());
  *(size_t*)((char*)&int_st + 100) = tmp;
}


