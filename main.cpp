#include <iostream>
#include <headers/UnbreakableStack.hpp>

class A {
 public:
  A() {
    std::cout << "gav";
  }
};

int main() {
  UnbreakableStack<std::vector<int>, Static> st;
  st.Push({1});
  st.Push({1, 2});
  st.Push({1, 2, 3});
  st.Push({1, 2, 3, 4});
  st.Push({1, 2, 3, 4, 5});
  st.Push({1, 2, 3, 4, 5, 6});
  st.Push({1, 2, 3, 4, 5, 6, 7});
  st.Push({1, 2, 3, 4, 5, 6, 7, 8});
//  st.Push({1, 2, 3, 4, 5, 6, 7, 8, 9});
}
