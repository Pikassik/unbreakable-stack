#pragma once

#include <string>

template <class T>
struct DefaultPoison {
  std::string operator()() const;
};

template <typename T>
std::string DefaultPoison<T>::operator()() const {
  std::string poison;
  for (size_t i = 0; i < sizeof(T); ++i) {
    poison.push_back(static_cast<char>(i) == 0 ?
                     'a' : i % ('z' - 'a' + 1) + 'a');
  }

  return poison;
}

template <typename T, bool is_arithmetic = std::is_arithmetic_v<T>>
struct DefaultDump {
  std::string operator()(const T& value) const;
};

char SymbolFromXDigit(unsigned char digit);

template <typename T>
struct DefaultDump<T, false> {
  std::string operator()(const T& value) const;
};

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


template <typename T>
struct DefaultDump<T, true> {
  std::string operator()(const T& value) const;
};
,
template <typename T>
std::string DefaultDump<T, true>::operator()(const T& value) const {
  return std::to_string(value);
}
