#pragma once

#include <cxxabi.h>
#include <cstdio>
#include <string>
#include <vector>
#include <memory>
#include <unistd.h>
#include <cassert>
#include <type_traits>

#ifndef NDEBUG
#define VERIFIED(boolean_arg) {\
  if (!boolean_arg) {\
    Dump(__FILE__, __LINE__, __PRETTY_FUNCTION__);\
    assert(true);\
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

template <typename T, bool IsClass = std::disjunction_v<std::is_class<T>, std::is_union<T>>>
struct DefaultDump;

template <typename T>
struct DefaultDump<T, true> {
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
struct DefaultDump<T, false> {
  std::string operator()(const T& value) {
    return std::to_string(value);
  }
};
//template <typename T>


class Static;
class Dynamic;

template <typename T,
          typename StorageType,
          typename DumpT = DefaultDump<T>,
          typename ValuePoison = DefaultPoison<T>,
          size_t storage_size = DEFAULT_STORAGE_SIZE>
class UnbreakableStack;

template<typename T, typename DumpT, typename ValuePoison, size_t storage_size>
class UnbreakableStack<T, Static, DumpT, ValuePoison, storage_size> {
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

  struct Wrapper {
    char begin_wrapper_canary[ConstexprRandom() % 100] = {};
    T value;
    char end_wrapper_canary  [ConstexprRandom() % 100] = {};
  };

  size_t begin_canary_                  = CANARY_POISON;
  size_t size_                          = 0;
  char char_buffer_[sizeof(Wrapper) * storage_size] = {};
  Wrapper* buffer_                             = reinterpret_cast<Wrapper*>(&char_buffer_);
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
         typename ValuePoison,
         size_t storage_size>
void UnbreakableStack<T,
                      Static,
                      DumpT,
                      ValuePoison,
                      storage_size>::Push(const T& value) {
  VERIFIED(Ok());
  new (reinterpret_cast<char*>(buffer_ + size_) + sizeof(Wrapper::begin_wrapper_canary)) T(value);
  ++size_;
  *check_sum_ = CalculateCheckSum();
  VERIFIED(Ok());
}

template<typename T,
         typename DumpT,
         typename ValuePoison,
         size_t storage_size>
UnbreakableStack<T,
                 Static,
                 DumpT,
                 ValuePoison,
                 storage_size>::UnbreakableStack() {
#ifndef NDEBUG
  std::string poison = ValuePoison()();
  for (size_t i = 0; i < storage_size; ++i) {
    for (size_t j = 0; j < sizeof(T); ++j) {
      reinterpret_cast<char*>(&buffer_[i].value)[j] = poison[j];
    }
  }
  *check_sum_ = CalculateCheckSum();
#endif
}

template<typename T,
    typename DumpT,
    typename ValuePoison,
    size_t storage_size>
size_t  UnbreakableStack<T,
                         Static,
                         DumpT,
                         ValuePoison,
                         storage_size>::CalculateCheckSum() const {
  return reinterpret_cast<size_t>(this) +
  std::hash<std::string>()(std::string(reinterpret_cast<const char*>(this),
                                       sizeof(UnbreakableStack<T,
                                                               Static,
                                                               DumpT,
                                                               ValuePoison,
                                                               storage_size>)));
}

template<typename T,
         typename DumpT,
         typename ValuePoison,
         size_t storage_size>
bool UnbreakableStack<T,
                      Static,
                      DumpT,
                      ValuePoison,
                      storage_size>::Ok() {

  if (this                == nullptr)             return false;
  if (begin_canary_       != CANARY_POISON)       return false;
  if (end_canary_         != CANARY_POISON)       return false;
  if (size_               >  storage_size)        return false;

  std::string poison = ValuePoison()();
  for (size_t i = size_; i < storage_size; ++i) {
    if (std::string(reinterpret_cast<const char*>(&buffer_[i].value), sizeof(T))
                          != poison)              return false;
  }
  auto x = CalculateCheckSum();
  if (*check_sum_         != CalculateCheckSum()) return false;

  return true;
}

template<typename T,
         typename DumpT,
         typename ValuePoison,
         size_t storage_size>
void UnbreakableStack<T,
                      Static,
                      DumpT,
                      ValuePoison,
                      storage_size>::Dump(const char* filename,
                                          int line,
                                          const char* function_name) {
  std::printf(
      "Ok failed! from %s (%d)\n%s:\n", filename, line, function_name
      );
  std::printf(
      "UnbreakableStack<T, StorageType, storage_size> with T = %s, "
      "StorageType = Static, storage_size = %llu [%p] (%s) {\n",
      abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr),
      storage_size, this, this == nullptr ? "ERROR" : "Ok"
      );
  std::printf(
      "    errno = %d (%s)\n", errno, errno == 0 ? "Ok" : "ERROR"
      );
  std::printf(
      "    size_ = %llu\n", size_
      );
  std::printf(
      "    buffer_[%llu] [%p] =\n", storage_size, buffer_
      );
  for (size_t i = 0; i < size_; ++i) {
    std::printf(
      "       *[%llu] = %s\n", i, DumpT()(buffer_[i].value).c_str()
      );
  }
  for (size_t i = size_; i < storage_size; ++i) {
    std::string value =
        std::string(reinterpret_cast<const char*>(&buffer_[i].value), sizeof(T));
    if (value == DefaultPoison<T>()()) {
      printf(
      "        [%llu] = %s (%s)\n", i,
      (value.size() < 10 ? value : value.substr(value.size()) + "...").c_str(), "poison"
     );
    } else {
      printf(
      "        [%llu] = %s (%s)\n", i, DumpT()(buffer_[i].value).c_str(),
      std::string(
          reinterpret_cast<const char*>(&buffer_[i].value), sizeof(T)) == ValuePoison()() ?
      "poison" : "NOT poison"
      );
    }
  }
  printf(
      "}"
      );
  fflush(stdin);
}
template<typename T,
         typename DumpT,
         typename ValuePoison,
         size_t storage_size>
void UnbreakableStack<T,
                      Static,
                      DumpT,
                      ValuePoison,
                      storage_size>::Push(T&& value) {
  VERIFIED(Ok());
  assert(size_ != storage_size);
  new (reinterpret_cast<char*>(buffer_ + size_) + sizeof(Wrapper::begin_wrapper_canary))
                                                            T(std::move(value));
  ++size_;
  *check_sum_ = CalculateCheckSum();
  VERIFIED(Ok());
}

template<typename T,
         typename DumpT,
         typename ValuePoison,
         size_t storage_size>
template<typename... Args>
void UnbreakableStack<T,
                      Static,
                      DumpT,
                      ValuePoison,
                      storage_size>::Emplace(Args... args) {
  VERIFIED(Ok());
  new (reinterpret_cast<char*>(buffer_ + size_) + sizeof(Wrapper::begin_wrapper_canary)) T(std::forward(args...));
  ++size_;
  *check_sum_ = CalculateCheckSum();
  VERIFIED(Ok());
}

template<typename T,
         typename DumpT,
         typename ValuePoison,
         size_t storage_size>
void UnbreakableStack<T, Static, DumpT, ValuePoison, storage_size>::Pop() {
  VERIFIED(Ok() && size_ != 0);

#ifndef NDEBUG
  buffer_[size_ - 1].value.~T();
  std::string poison = ValuePoison()();
  for (size_t j = 0; j < sizeof(T); ++j) {
    reinterpret_cast<char*>(&buffer_[size_ - 1].value)[j] = poison[j];
  }
#endif
  --size_;

  VERIFIED(Ok() && size_ >= 0);
}

template<typename T, typename DumpT, typename ValuePoison, size_t storage_size>
const T& UnbreakableStack<T,
                          Static,
                          DumpT,
                          ValuePoison,
                          storage_size>::Top() const noexcept {
  VERIFIED(Ok() && size_ != 0);
  return buffer_[size_ - 1].value;
}

template<typename T, typename DumpT, typename ValuePoison, size_t storage_size>
UnbreakableStack<T,
                 Static,
                 DumpT,
                 ValuePoison,
                 storage_size>::~UnbreakableStack() {
  VERIFIED(Ok());
  for (size_t i = 0; i < size_; ++i) {
    buffer_[i].value.~T();
  }
}
