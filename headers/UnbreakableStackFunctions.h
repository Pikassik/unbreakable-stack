/*!
 * @file Functors for UnbreakableStack
 */

#pragma once

#include <string>

/*!
 * @brief Default dump for Unbreakable Stack class (functor)
 * If T is fundamental, then prints it else returns string of bytes
 * @tparam T typename to be dumped
 * @tparam is_fundamental
 */
template <typename T, bool is_fundamental = std::is_fundamental_v<T>>
struct DefaultDump {
  void operator()(const T& value) const;
};

/*!
 * @brief transforms digit to its char
 * @param digit from 0 to 15
 * @return char of hexadecimal digit
 */
char SymbolFromXDigit(unsigned char digit);

/*!
 * @tparam T not fundamental type
 */
template <typename T>
struct DefaultDump<T, false> {
  void operator()(const T& value) const;
};

/*!
 * @tparam T
 * @param value
 * @return string of T object's bytes
 */
template <typename T>
void DefaultDump<T, false>::operator()(const T& value) const {
  std::string_view value_string =
      std::string_view(reinterpret_cast<const char*>(&value), sizeof(T));

  std::printf("0x");
  for (int64_t i = value_string.size() - 1; i >= 0; --i) {
    unsigned char byte = value_string[i];

    unsigned char left_byte = byte >> 4;
    putchar(SymbolFromXDigit(left_byte));

    unsigned char right_byte = byte - (left_byte << 4);
    putchar(SymbolFromXDigit(right_byte));
  }
}

/*!
 * @tparam T for fundamental type
 */
template <typename T>
struct DefaultDump<T, true> {
  void operator()(const T& value) const;
};

/*!
 *
 * @tparam T fundamental type (int, float, ptr, etc)
 * @param value
 * @return view of fundamental type
 */
template <typename T>
void DefaultDump<T, true>::operator()(const T& value) const {
  if        constexpr (std::is_same_v<signed char, T>) {
    std::printf("%hhd", value);
  } else if constexpr (std::is_same_v<short, T>) {
    std::printf("%hd", value);
  } else if constexpr (std::is_same_v<int, T>) {
    std::printf("%d", value);
  } else if constexpr (std::is_same_v<long, T>) {
    std::printf("%ld", value);
  } else if constexpr (std::is_same_v<long long, T>) {
    std::printf("%lld", value);
  } else if constexpr (std::is_same_v<unsigned char, T>) {
    std::printf("%hhu", value);
  } else if constexpr (std::is_same_v<unsigned short, T>) {
    std::printf("%hu", value);
  } else if constexpr (std::is_same_v<unsigned int, T>) {
    std::printf("%u", value);
  } else if constexpr (std::is_same_v<unsigned long, T>) {
    std::printf("%lu", value);
  } else if constexpr (std::is_same_v<unsigned long long, T>) {
    std::printf("%llu", value);
  } else if constexpr (std::disjunction_v<
                       std::is_same_v<float, T>,
                       std::is_same_v<double, T>>) {
    std::printf("%f", value);
  }  else if constexpr (std::is_same_v<long double, T>) {
    std::printf("%Lf", value);
  } else if constexpr (std::is_pointer_v<T>) {
    std::printf("%p", value);
  }
}
