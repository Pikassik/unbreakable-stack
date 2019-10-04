#include <iostream>
#include <headers/UnbreakableStack.hpp>

int main() {
  UnbreakableStack<int, Static, DefaultDump<int>, 100> st;
  for (size_t i = 0 ; i < 10; ++i) {
    st.Push(i);
  }
  *((size_t*)&st + 30) = 100500;
  st.Push(15000);
}
