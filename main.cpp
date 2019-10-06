#include <iostream>
#include <vector>
#include <headers/UnbreakableStack.hpp>

using std::vector;

int main() {
  UnbreakableStack<int, Static, DefaultDump<int>, 100> st;
  for (size_t i = 0 ; i < 100; ++i) {
    st.Push({static_cast<int>(i)});
    std::cout << st.Top() << std::endl;
  }
  st.Size();
  for (size_t i = 0 ; i < 100; ++i) {
    std::cout << st.Top() << std::endl;
    st.Pop();
  }

  return 0;
}
