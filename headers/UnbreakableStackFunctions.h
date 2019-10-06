/*!
 * @file Functors for UnbreakableStack
 */

#pragma once

#include <string>



/*!
 * @brief Functor to generate poison for any type
 * @tparam T typename of poison
 */
template <class T>
struct Poison {
  std::string operator()() const;
};

/*!
 * @brief Generates string of bytes which contains bytes to copy on object place
 * @tparam T typename of poison
 * @return string of poison
 */
template <typename T>
std::string Poison<T>::operator()() const {
  std::string poison;
  for (size_t i = 0; i < sizeof(T); ++i) {
    poison.push_back(static_cast<char>(i) == 0 ?
                     'a' : i % ('z' - 'a' + 1) + 'a');
  }

  return poison;
}

/*!
 * @brief Default dump for Unbreakable Stack class
 * If T is arithmetic, then returns std::to_string else returns string of bytes
 * @tparam T typename to be dumped
 * @tparam is_arithmetic
 */
template <typename T, bool is_arithmetic = std::is_arithmetic_v<T>>
struct DefaultDump {
  std::string operator()(const T& value) const;
};

/*!
 * @brief transforms digit to its char
 * @param digit from 0 to 15
 * @return char of hexadecimal digit
 */
char SymbolFromXDigit(unsigned char digit);

/*!
 * @tparam T not arithmetical type
 */
template <typename T>
struct DefaultDump<T, false> {
  std::string operator()(const T& value) const;
};

/*!
 * @tparam T
 * @param value
 * @return string of T object's bytes
 */
template <typename T>
std::string DefaultDump<T, false>::operator()(const T& value) const {
  std::string value_string =
      std::string(reinterpret_cast<const char*>(&value), sizeof(T));
  std::string dump = "0x";
  for (int64_t i = value_string.size() - 1; i >= 0; --i) {
    unsigned char byte = value_string[i];

    unsigned char left_byte = byte >> 4;
    dump.push_back(SymbolFromXDigit(left_byte));

    unsigned char right_byte = byte - (left_byte << 4);
    dump.push_back(SymbolFromXDigit(right_byte));
  }

  return dump;
}

/*!
 * @tparam T for aritmtetical type
 */
template <typename T>
struct DefaultDump<T, true> {
  std::string operator()(const T& value) const;
};

/*!
 *
 * @tparam T aritmetical type (int, float, etc)
 * @param value
 * @return view of aritmetical type
 */
template <typename T>
std::string DefaultDump<T, true>::operator()(const T& value) const {
  return std::to_string(value);
}
