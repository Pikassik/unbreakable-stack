/*!
 * @file Stack class with protectors, checks and dump in stdin
 */

#pragma once

#include <UnbreakableStackFunctions.h>
#include <cxxabi.h>
#include <cstdio>
#include <string>
#include <memory>
#include <cassert>
#include <cstring>
#include <type_traits>

#ifndef IS_NOT_FATAL
#define IS_NOT_FATAL true
#endif

#ifndef NDEBUG
#define VERIFIED(boolean_arg) {\
  if (!boolean_arg) {\
    Dump(__FILE__, __LINE__, __PRETTY_FUNCTION__);\
    assert(IS_NOT_FATAL);\
  }\
}
#else
#define VERIFIED(x)
#endif

#ifdef TESTING
#undef VERIFIED
#define VERIFIED(boolean_arg) {assert(boolean_arg);}
#endif

enum : size_t {
  CANARY_POISON        = 0xDEADBEEFCACED426,
  POISON_BYTE          = 0xFC,
  DEFAULT_STORAGE_SIZE = 8
};

class Static;
class Dynamic;

struct TestUnbreakableStack;

/*!
 * @brief Stack class which checks his state in start and end of every method
 * and dumps if it's not successfull
 *
 * @tparam T Value type
 * @tparam StorageType Dynamic or Static
 * @tparam DumpT Functor to dump T object
 * @tparam storage_size Max size of storage for static version
 */
template <typename T,
          typename StorageType,
          typename DumpT = DefaultDump<T>,
          size_t storage_size = DEFAULT_STORAGE_SIZE>
class UnbreakableStack;


/*!
 * @brief Static version (template specialization)
 *
 * @tparam T Value type
 * @tparam DumpT Functor to dump T object
 * @tparam storage_size Max size of storage for static version
 */
template<typename T,
         typename DumpT,
         size_t storage_size>
class UnbreakableStack<T, Static, DumpT, storage_size> {
  friend TestUnbreakableStack;

 public:
  UnbreakableStack();
  UnbreakableStack(const UnbreakableStack&) = delete;
  UnbreakableStack& operator=(const UnbreakableStack&) = delete;
  ~UnbreakableStack();

  void Push(const T& value);
  void Push(T&& value);

  template <typename... Args>
  void Emplace(Args&&... args);

  void Pop();

  const T& Top() const noexcept;

  size_t Size() const noexcept;

  const T& Data(size_t index) const;

 private:

#ifndef NDEBUG
  size_t begin_canary_                  = CANARY_POISON;
#endif

  size_t size_                          = 0;
  char buffer_[sizeof(T) * storage_size] = {};

#ifndef NDEBUG
  mutable size_t check_sum_             = 0;
  size_t end_canary_                    = CANARY_POISON;
#endif

#ifndef NDEBUG
  bool Ok() const;
  void Dump(const char* filename, int line, const char* function_name) const;
  void FillWithPoison(size_t index);
  bool IsPoison(size_t index) const;
  size_t CalculateCheckSum() const;
#endif
};


/*!
 * @brief fill buffer with poison and calculates check sum
 */
template<typename T,
         typename DumpT,
         size_t storage_size>
UnbreakableStack<T, Static, DumpT, storage_size>::UnbreakableStack() {
#ifndef NDEBUG
  for (size_t i = 0; i < storage_size; ++i) {
    FillWithPoison(i);
  }

  check_sum_ = CalculateCheckSum();
#endif
}


/*!
 * @brief destroys every elem in [0, size_)
 */
template<typename T,
         typename DumpT,
         size_t storage_size>
UnbreakableStack<T, Static, DumpT, storage_size>::~UnbreakableStack() {
  VERIFIED(Ok())

  for (size_t i = 0; i < size_; ++i) {
    reinterpret_cast<T*>(buffer_)[i].~T();
  }
  begin_canary_ = ~CANARY_POISON;
}


/*!
 * @brief checks stack state, constructs elem in size_ pos from value checks
 * again and calculates again check sum
 * @param value &value must be not nullptr
 */
template<typename T,
         typename DumpT,
         size_t storage_size>
void UnbreakableStack<T, Static, DumpT, storage_size>::Push(const T& value) {
  assert(&value != nullptr);
  VERIFIED(Ok())

  new (reinterpret_cast<T*>(buffer_) + size_) T(value);
  ++size_;

#ifndef NDEBUG
  check_sum_ = CalculateCheckSum();
#endif

  VERIFIED(Ok())
}

/*!
 * @brief checks stack state, constructs elem in size_ pos from value checks
 * again and calculates again check sum
 * @param value &value must not be nullptr
 */
template<typename T,
         typename DumpT,
         size_t storage_size>
void UnbreakableStack<T, Static, DumpT, storage_size>::Push(T&& value) {
  VERIFIED(Ok())
  assert(&value != nullptr);
  assert(size_ != storage_size ||
  (({VERIFIED(false);}), false));

  new (reinterpret_cast<T*>(buffer_) + size_) T(std::move(value));
  ++size_;

#ifndef NDEBUG
  check_sum_ = CalculateCheckSum();
#endif

  VERIFIED(Ok())
}

/*!
 * @brief checks stack state, constructs elem in size_ pos from args checks
 * again and calculates again check sum
 * @param args
 */
template<typename T,
         typename DumpT,
         size_t storage_size>
template<typename... Args>
void UnbreakableStack<T, Static, DumpT, storage_size>::Emplace(Args&&... args) {
  VERIFIED(Ok());
  assert(size_ != storage_size ||
  ((VERIFIED(false)), false));

  new (reinterpret_cast<T*>(buffer_) + size_) T(std::forward(args...));
  ++size_;

#ifndef NDEBUG
  check_sum_ = CalculateCheckSum();
#endif

  VERIFIED(Ok());
}

/*!
 * @brief check state, destroys elem in size_ - 1 pos, decrease size_, fill it
 * with poison, calculate new check sum and check state again
 */
template<typename T,
         typename DumpT,
         size_t storage_size>
void UnbreakableStack<T, Static, DumpT, storage_size>::Pop() {
  VERIFIED(Ok());
  assert(size_ != 0 ||
  (({VERIFIED(false)}), false));

  reinterpret_cast<T*>(buffer_)[size_ - 1].~T();
  --size_;

#ifndef NDEBUG
  FillWithPoison(size_);

  check_sum_ = CalculateCheckSum();
#endif

  VERIFIED(Ok());
}

/*!
 * @brief checks state and non-zero size and returns reference to top
 * @return buffer[size_ - 1]
 */

template<typename T,
         typename DumpT,
         size_t storage_size>
const T& UnbreakableStack<T, Static, DumpT, storage_size>::
    Top() const noexcept {
  VERIFIED(Ok());
  assert(size_ != 0 ||
  (({VERIFIED(false);}), false));

  return reinterpret_cast<const T*>(buffer_)[size_ - 1];
}

/*!
 * @brief checks state and returns size_
 * @return size_
 */
template<typename T,
         typename DumpT,
         size_t storage_size>
size_t UnbreakableStack<T, Static, DumpT, storage_size>::Size() const noexcept {
  VERIFIED(Ok());
  return size_;
}

template<typename T, typename DumpT, size_t storage_size>
const T& UnbreakableStack<T,
                          Static,
                          DumpT,
                          storage_size>::Data(size_t index) const {
  VERIFIED(Ok());
  assert(index < size_ ||
  (({VERIFIED(false);}), false));

  return reinterpret_cast<const T*>(buffer_)[index];
}


#ifndef NDEBUG

/*!
 * @brief checks object's state
 * @return is object's state ok
 */
template<typename T,
         typename DumpT,
         size_t storage_size>
bool UnbreakableStack<T, Static, DumpT, storage_size>::Ok() const {

  if (this                == nullptr)             return false;
  if (begin_canary_       != CANARY_POISON)       return false;
  if (end_canary_         != CANARY_POISON)       return false;
  if (size_               >  storage_size)        return false;
  if (size_               == SIZE_MAX)            return false;

  for (size_t i = size_; i < storage_size; ++i) {
    if (!IsPoison(i))                             return false;
  }

  if (check_sum_          != CalculateCheckSum()) return false;

  return true;
}

/*!
 * @brief prints in stdin formatted state of object
 * @param filename of place where it is called
 * @param line of place where it is called
 * @param function_name where it is called
 */
template<typename T,
         typename DumpT,
         size_t storage_size>
void UnbreakableStack<T, Static, DumpT, storage_size>::
    Dump(const char* filename, int line, const char* function_name) const {
  std::printf(
      "Ok failed! from %s (%d)\n%s:\n", filename, line, function_name
      );
  std::fflush(stdout);
  std::printf(
      "UnbreakableStack<T, StorageType, storage_size> with [T = %s; "
      "StorageType = Static; size_t storage_size = %llu] [%p] (%s) {\n",
      abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr),
      storage_size, this, this == nullptr ? "ERROR" : "Ok"
      );
  std::fflush(stdout);
  if (!this) {
    std::printf(
      "}\n"
      );
    std::fflush(stdout);
    return;
  }
  std::printf(
      "    errno = %d (%s)\n", errno, errno != 0 ? "ERROR" : "Ok"
      );
  std::fflush(stdout);
  std::printf(
      "    size_t begin_canary = %llu (%s)\n", begin_canary_,
      begin_canary_ != CANARY_POISON ? "ERROR" : "Ok"
      );
  std::fflush(stdout);
  std::printf(
      "    size_t size_ = %llu (%s)\n", size_, size_ >= storage_size ?
      (size_ == storage_size ? "Full" : "OVERFLOW") : "Ok"
      );
  std::fflush(stdout);
  std::printf(
      "    size_t check_sum_ = %llu (%s)\n",
      check_sum_, check_sum_ != CalculateCheckSum() ? "ERROR" : "Ok"
      );
  std::fflush(stdout);
  std::printf(
      "    size_t end_canary = %llu (%s)\n", end_canary_,
      end_canary_ != CANARY_POISON ? "ERROR" : "Ok"
      );
  std::fflush(stdout);
  std::printf(
      "    char[] buffer_[%llu] [%p] =\n", storage_size, &buffer_
      );
  std::fflush(stdout);
  for (size_t i = 0; i < size_; ++i) {
    std::printf(
      "       *[%llu] = ", i);
    DumpT()(reinterpret_cast<const T*>(&buffer_)[i]);
    if (IsPoison(i)) {
      std::printf(" (POISON!)");
    }
    std::printf("\n");
    std::fflush(stdout);
  }
  for (size_t i = size_; i < storage_size; ++i) {
    if (IsPoison(i)) {
      std::printf(
      "        [%llu] = poison (poison)\n", i
      );
    } else {
      std::printf(
      "        [%llu] = ");
      if constexpr (std::is_fundamental_v<T>) {
        DumpT()(reinterpret_cast<const T*>(&buffer_)[i]);
      } else {
        std::printf("NOT poison");
      }
      std::printf(" (NOT poison)");
   }
   std::fflush(stdout);
  }
  std::printf(
      "}\n"
      );
  std::fflush(stdout);
}


/*!
 * @brief fills object in index position with poison bytes
 * @param index
 */
template<typename T,
         typename DumpT,
         size_t storage_size>
void UnbreakableStack<T, Static, DumpT, storage_size>::
    FillWithPoison(size_t index) {
  std::memset(reinterpret_cast<char*>(reinterpret_cast<T*>(buffer_) + index),
         POISON_BYTE, sizeof(T));
}

/*!
 * @param index
 * @return is object on index position is poison
 */
template<typename T, typename DumpT, size_t storage_size>
bool UnbreakableStack<T, Static, DumpT, storage_size>::
    IsPoison(size_t index) const {
  const char* value_ptr =
    reinterpret_cast<const char*>(reinterpret_cast<const T*>(buffer_) + index);
  for (size_t i = 0; i < sizeof(T); ++i) {
    if (value_ptr[i] != static_cast<char>(POISON_BYTE)) {
      return false;
    }
  }

  return true;
}

/*!
 *
 * @brief calculates hash from object casted to string
 * @return check sum of object
 */
template<typename T,
         typename DumpT,
         size_t storage_size>
size_t  UnbreakableStack<T, Static,DumpT, storage_size>::
    CalculateCheckSum() const {
  size_t old_check_sum = check_sum_;
  check_sum_ = 0;
  size_t calculated_check_sum = reinterpret_cast<size_t>(this) +
  std::hash<std::string_view>()(
    std::string_view(reinterpret_cast<const char*>(this),
                     sizeof(UnbreakableStack<T, Static, DumpT, storage_size>)));
  check_sum_ = old_check_sum;

  return calculated_check_sum;
}

#endif

