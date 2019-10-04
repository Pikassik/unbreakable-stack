#include <iostream>
#include <vector>
#include <headers/UnbreakableStack.hpp>

using std::vector;

int main() {
  UnbreakableStack<std::vector<int>, Static, DefaultDump<std::vector<int>>, 100> st;
  for (size_t i = 0 ; i < 100; ++i) {
    st.Push({static_cast<int>(i)});
    st.Top();
  }
//  *((size_t*)&st + 10) = 100500;
//  auto x = const_cast<vector<int>&>(st.Top());
//  x.~vector<int>();
  return 0;
}
