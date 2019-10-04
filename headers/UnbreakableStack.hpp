#pragma once

#include <cxxabi.h>
#include <cstdio>
#include <string>
#include <vector>
#include <memory>
#include <unistd.h>
#include <cassert>
#include <type_traits>

#define __IS_NOT_FATAL__ true

#ifndef NDEBUG
#define VERIFIED(boolean_arg) {\
  if (!boolean_arg) {\
    Dump(__FILE__, __LINE__, __PRETTY_FUNCTION__);\
    assert(__IS_NOT_FATAL__);\
  }\
}
#endif


enum : size_t {
  CANARY_POISON        = 0xDEADBEEFCACED426,
  VALUE_POISON         = 0xDED1CDEAAAAAAAAD,
  DEFAULT_STORAGE_SIZE = 8,
  MULTIPLIER           = 7
};


constexpr size_t ConstexprRandom() {
  const char time[] = __TIME__;
  size_t result = 0;
  size_t multiplier = 1;
  for (auto letter: time) {
    result += multiplier * letter;
    multiplier *= MULTIPLIER;
  }

  return result;
}

template <class T>
struct DefaultPoison {
  std::string operator()() {
    std::string poison;
    for (size_t i = 0; i < sizeof(T); ++i) {
      poison.push_back(static_cast<char>(i) == 0 ? 'a' : i % ('z' - 'a' + 1) + 'a');
    }
    return poison;
  }
};

char SymbolFromXDigit(unsigned char digit) {
  if (digit < 10) {
    return digit + '0';
  } else {
    return digit - 10 + 'A';
  }
}

template <typename T, bool is_arithmetic = std::is_arithmetic_v<T>>
struct DefaultDump;

template <typename T>
struct DefaultDump<T, false> {
  std::string operator()(const T& value) {
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
};

template <typename T>
struct DefaultDump<T, true> {
  std::string operator()(const T& value) {
    return std::to_string(value);
  }
};

class Static;
class Dynamic;

template <typename T,
          typename StorageType,
          typename DumpT = DefaultDump<T>,
          size_t storage_size = DEFAULT_STORAGE_SIZE>
class UnbreakableStack;

template<typename T,
         typename DumpT,
         size_t storage_size>
class UnbreakableStack<T, Static, DumpT, storage_size> {
 public:
  UnbreakableStack();
  void Push(const T& value);
  void Push(T&& value);

  template <typename... Args>
  void Emplace(Args... args);

  void Pop();

  const T& Top() const noexcept;
  ~UnbreakableStack();
 private:

  size_t begin_canary_                  = CANARY_POISON;
  size_t size_                          = 0;
  char char_buffer_[sizeof(T) * storage_size] = {};
  T* buffer_                             = reinterpret_cast<T*>(&char_buffer_);
  std::unique_ptr<size_t> check_sum_    = std::make_unique<size_t>(0);
  size_t end_canary_                    = CANARY_POISON;
  bool Ok();
  void Dump(const char* filename,
            int line,
            const char* function_name);
  size_t CalculateCheckSum() const;
};

template<typename T,
         typename DumpT,
         size_t storage_size>
void UnbreakableStack<T,
                      Static,
                      DumpT,
                      storage_size>::Push(const T& value) {
  VERIFIED(Ok());
  assert(size_ != storage_size || (({Dump(__FILE__, __LINE__, __PRETTY_FUNCTION__);}), false));
  new (reinterpret_cast<char*>(buffer_ + size_)) T(value);
  ++size_;
  *check_sum_ = CalculateCheckSum();
  VERIFIED(Ok());
}

template<typename T,
         typename DumpT,
         size_t storage_size>
UnbreakableStack<T,
                 Static,
                 DumpT,
                 storage_size>::UnbreakableStack() {
#ifndef NDEBUG
  std::string poison = DefaultPoison<T>()();
  for (size_t i = 0; i < storage_size; ++i) {
    for (size_t j = 0; j < sizeof(T); ++j) {
      reinterpret_cast<char*>(&buffer_[i])[j] = poison[j];
    }
  }
  *check_sum_ = CalculateCheckSum();
#endif
}

template<typename T,
         typename DumpT,
         size_t storage_size>
size_t  UnbreakableStack<T,
                         Static,
                         DumpT,
                         storage_size>::CalculateCheckSum() const {
  return reinterpret_cast<size_t>(this) +
  std::hash<std::string>()(std::string(reinterpret_cast<const char*>(this),
                                       sizeof(UnbreakableStack<T,
                                                               Static,
                                                               DumpT,
                                                               storage_size>)));
}

template<typename T,
         typename DumpT,
         size_t storage_size>
bool UnbreakableStack<T,
                      Static,
                      DumpT,
                      storage_size>::Ok() {

  if (this                == nullptr)             return false;
  if (begin_canary_       != CANARY_POISON)       return false;
  if (end_canary_         != CANARY_POISON)       return false;
  if (size_               >  storage_size)        return false;

  std::string poison = DefaultPoison<T>()();
  for (size_t i = size_; i < storage_size; ++i) {
    if (std::string(reinterpret_cast<const char*>(&buffer_[i]), sizeof(T))
                          != poison)              return false;
  }
  auto x = CalculateCheckSum();
  if (*check_sum_         != CalculateCheckSum()) return false;

  return true;
}

template<typename T,
         typename DumpT,
         size_t storage_size>
void UnbreakableStack<T,
                      Static,
                      DumpT,
                      storage_size>::Dump(const char* filename,
                                          int line,
                                          const char* function_name) {
  std::printf(
      "Ok failed! from %s (%d)\n%s:\n", filename, line, function_name
      );
  std::printf(
      "UnbreakableStack<T, StorageType, storage_size> with [T = %s; "
      "StorageType = Static; size_t storage_size = %llu] [%p] (%s) {\n",
      abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr),
      storage_size, this, this == nullptr ? "ERROR" : "Ok"
      );
  if (!this) {
    std::printf(
      "}\n"
      );
    return;
  }
  std::printf(
      "    errno = %d (%s)\n", errno, errno != 0 ? "ERROR" : "Ok"
      );
  printf(
      "    size_t begin_canary = %llu (%s)\n", begin_canary_, begin_canary_ != CANARY_POISON ? "ERROR" : "Ok"
      );
  std::printf(
      "    size_t size_ = %llu (%s)\n", size_, size_ >= storage_size ? (size_ == storage_size ? "Full" : "OVERFLOW") : "Ok"
      );
  std::printf(
      "    char[] char_buffer_[%llu] [%p] =\n", storage_size, &char_buffer_
      );
  for (size_t i = 0; i < size_; ++i) {
    std::printf(
      "       *[%llu] = %s\n", i, DumpT()(buffer_[i]).c_str()
      );
  }
  for (size_t i = size_; i < storage_size; ++i) {
    std::string value =
        std::string(reinterpret_cast<const char*>(&buffer_[i]), sizeof(T));
    if (value == DefaultPoison<T>()()) {
      printf(
      "        [%llu] = %s (%s)\n", i,
      (value.size() < 20 ? value : value.substr(value.size()) + "...").c_str(), "poison"
      );
    } else {
      printf(
      "        [%llu] = %s (%s)\n", i, DumpT()(buffer_[i]).c_str(), "NOT poison"
      );
    }
  }
  printf(
      "T* buffer_ = [%p] (%s)\n", buffer_,
      reinterpret_cast<char*>(buffer_) != reinterpret_cast<char*>(&char_buffer_) ? "ERROR" : "Ok"
      );
  printf(
      "size_t check_sum_ = %llu (%s)\n", *check_sum_, *check_sum_ != CalculateCheckSum() ? "ERROR" : "Ok"
      );
  printf(
      "size_t end_canary = %llu (%s)\n", end_canary_, end_canary_ != CANARY_POISON ? "ERROR" : "Ok"
      );
  printf(
      "}\n"
      );
  fflush(stdin);
}
template<typename T,
         typename DumpT,
         size_t storage_size>
void UnbreakableStack<T,
                      Static,
                      DumpT,
                      storage_size>::Push(T&& value) {
  VERIFIED(Ok());
  assert(size_ != storage_size || (({Dump(__FILE__, __LINE__, __PRETTY_FUNCTION__);}), false));
  new (reinterpret_cast<char*>(buffer_ + size_))
                                                            T(std::move(value));
  ++size_;
  *check_sum_ = CalculateCheckSum();
  VERIFIED(Ok());
}

template<typename T,
         typename DumpT,
         size_t storage_size>
template<typename... Args>
void UnbreakableStack<T,
                      Static,
                      DumpT,
                      storage_size>::Emplace(Args... args) {
  VERIFIED(Ok());
  assert(size_ != storage_size || (({Dump(__FILE__, __LINE__, __PRETTY_FUNCTION__);}), false));
  new (reinterpret_cast<char*>(buffer_ + size_)) T(std::forward(args...));
  ++size_;
  *check_sum_ = CalculateCheckSum();
  VERIFIED(Ok());
}

template<typename T,
         typename DumpT,
         size_t storage_size>
void UnbreakableStack<T, Static, DumpT, storage_size>::Pop() {
  VERIFIED(Ok() && size_ != 0);
  assert(size_ != 0 || Dump(__FILE__, __LINE__, __PRETTY_FUNCTION__));

#ifndef NDEBUG
  buffer_[size_ - 1].~T();
  std::string poison = DefaultPoison<T>()();
  for (size_t j = 0; j < sizeof(T); ++j) {
    reinterpret_cast<char*>(&buffer_[size_ - 1])[j] = poison[j];
  }
#endif
  --size_;

  VERIFIED(Ok() && size_ >= 0);
}

template<typename T,
         typename DumpT,
         size_t storage_size>
const T& UnbreakableStack<T,
                          Static,
                          DumpT,
                          storage_size>::Top() const noexcept {
  VERIFIED(Ok());
  assert(size_ != 0 || Dump(__FILE__, __LINE__, __PRETTY_FUNCTION__));
  return buffer_[size_ - 1];
}

template<typename T,
         typename DumpT,
         size_t storage_size>
UnbreakableStack<T,
                 Static,
                 DumpT,
                 storage_size>::~UnbreakableStack() {
  VERIFIED(Ok());
  for (size_t i = 0; i < size_; ++i) {
    buffer_[i].~T();
  }
}