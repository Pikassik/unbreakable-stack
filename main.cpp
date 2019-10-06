#include <iostream>
#include <vector>
#include <headers/UnbreakableStack.hpp>

using std::vector;

int main() {
  UnbreakableStack<int, Static, DefaultDump<int>, 100> st;
  for (size_t i = 0 ; i < 101; ++i) {
    st.Push({static_cast<int>(i)});
    st.Top();
  }
//  st.Dump(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  return 0;
}
