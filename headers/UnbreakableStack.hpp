#pragma once

#include <headers/UnbreakableStackFunctions.h>
#include <cxxabi.h>
#include <cstdio>
#include <string>
#include <memory>
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
  DEFAULT_STORAGE_SIZE = 8,
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
  T* buffer_                            = reinterpret_cast<T*>(&char_buffer_);
  std::unique_ptr<size_t> check_sum_    = std::make_unique<size_t>(0);
  size_t end_canary_                    = CANARY_POISON;
  bool Ok() const noexcept;
  void Dump(const char* filename, int line, const char* function_name) const;
  size_t CalculateCheckSum() const;
};

template<typename T,
         typename DumpT,
         size_t storage_size>
void UnbreakableStack<T, Static, DumpT, storage_size>::Push(const T& value) {
  assert(&value != nullptr);
  VERIFIED(Ok());
  new (reinterpret_cast<char*>(buffer_ + size_)) T(value);
  ++size_;
#ifndef NDEBUG
  *check_sum_ = CalculateCheckSum();
#endif
  VERIFIED(Ok());
}

template<typename T,
         typename DumpT,
         size_t storage_size>
UnbreakableStack<T, Static, DumpT, storage_size>::UnbreakableStack() {
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
size_t  UnbreakableStack<T, Static,
DumpT, storage_size>::CalculateCheckSum() const {
  return reinterpret_cast<size_t>(this) +
  std::hash<std::string_view>()(
      std::string_view(reinterpret_cast<const char*>(this),
                     sizeof(UnbreakableStack<T, Static, DumpT, storage_size>)));
}

template<typename T,
         typename DumpT,
         size_t storage_size>
bool UnbreakableStack<T, Static, DumpT, storage_size>::Ok() const noexcept {

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
void UnbreakableStack<T, Static, DumpT, storage_size>::
    Dump(const char* filename, int line, const char* function_name) const {
  std::printf(
      "Ok failed! from %s (%d)\n%s:\n", filename, line, function_name
      );
  fflush(stdin);
  std::printf(
      "UnbreakableStack<T, StorageType, storage_size> with [T = %s; "
      "StorageType = Static; size_t storage_size = %llu] [%p] (%s) {\n",
      abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr),
      storage_size, this, this == nullptr ? "ERROR" : "Ok"
      );
  fflush(stdin);
  if (!this) {
    std::printf(
      "}\n"
      );
    fflush(stdin);
    return;
  }
  std::printf(
      "    errno = %d (%s)\n", errno, errno != 0 ? "ERROR" : "Ok"
      );
  fflush(stdin);
  printf(
      "    size_t begin_canary = %llu (%s)\n", begin_canary_,
      begin_canary_ != CANARY_POISON ? "ERROR" : "Ok"
      );
  fflush(stdin);
  std::printf(
      "    size_t size_ = %llu (%s)\n", size_, size_ >= storage_size ?
      (size_ == storage_size ? "Full" : "OVERFLOW") : "Ok"
      );
  fflush(stdin);
  std::printf(
      "    char[] char_buffer_[%llu] [%p] =\n", storage_size, &char_buffer_
      );
  fflush(stdin);
  for (size_t i = 0; i < size_; ++i) {
    std::printf(
      "       *[%llu] = %s\n", i, DumpT()(buffer_[i]).c_str()
      );
    fflush(stdin);
  }
  for (size_t i = size_; i < storage_size; ++i) {
      printf(
      "        [%llu] = %s (%s)\n", i, DumpT()(buffer_[i]).c_str(),
      DefaultPoison<T>()() ==
          std::string_view(reinterpret_cast<const char*>(&buffer_[i]),sizeof(T))
          ? "poison" : "NOT poison"
      );
    fflush(stdin);
  }
  printf(
      "T* buffer_ = [%p] (%s)\n", buffer_,
      reinterpret_cast<const char*>(buffer_) !=
      reinterpret_cast<const char*>(&char_buffer_)
      ? "ERROR" : "Ok"
      );
  fflush(stdin);
  printf(
      "size_t check_sum_ = %llu (%s)\n",
      *check_sum_, *check_sum_ != CalculateCheckSum() ? "ERROR" : "Ok"
      );
  fflush(stdin);
  printf(
      "size_t end_canary = %llu (%s)\n", end_canary_, end_canary_ != CANARY_POISON ? "ERROR" : "Ok"
      );
  fflush(stdin);
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
  assert(&value != nullptr);
  VERIFIED(Ok());
  assert(size_ != storage_size ||
  (({Dump(__FILE__, __LINE__, __PRETTY_FUNCTION__);}), false));
  new (reinterpret_cast<char*>(buffer_ + size_)) T(std::move(value));
  ++size_;
#ifndef NDEBUG
  *check_sum_ = CalculateCheckSum();
#endif
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
  assert(size_ != storage_size ||
  (({Dump(__FILE__, __LINE__, __PRETTY_FUNCTION__);}), false));
  new (reinterpret_cast<char*>(buffer_ + size_)) T(std::forward(args...));
  ++size_;
#ifndef NDEBUG
  *check_sum_ = CalculateCheckSum();
#endif
  VERIFIED(Ok());
}

template<typename T,
         typename DumpT,
         size_t storage_size>
void UnbreakableStack<T, Static, DumpT, storage_size>::Pop() {
  VERIFIED(Ok());
  assert(size_ != 0 ||
  (({Dump(__FILE__, __LINE__, __PRETTY_FUNCTION__);}), false));

#ifndef NDEBUG
  buffer_[size_ - 1].~T();
  std::string poison = DefaultPoison<T>()();
  for (size_t j = 0; j < sizeof(T); ++j) {
    reinterpret_cast<char*>(&buffer_[size_ - 1])[j] = poison[j];
  }
#endif
  --size_;

  VERIFIED(Ok());
  assert(size_ >= 0 ||
  (({Dump(__FILE__, __LINE__, __PRETTY_FUNCTION__);}), false));
}

template<typename T,
         typename DumpT,
         size_t storage_size>
const T& UnbreakableStack<T,
                          Static,
                          DumpT,
                          storage_size>::Top() const noexcept {
  VERIFIED(Ok());
  assert(size_ != 0 ||
  (({Dump(__FILE__, __LINE__, __PRETTY_FUNCTION__);}), false));
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