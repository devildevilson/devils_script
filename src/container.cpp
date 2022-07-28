#include "container.h"

#include <cassert>

#ifndef DEVILS_SCRIPT_INNER_NAMESPACE
#define DEVILS_SCRIPT_INNER_NAMESPACE devils_script
#endif // DEVILS_SCRIPT_INNER_NAMESPACE

#ifdef DEVILS_SCRIPT_OUTER_NAMESPACE
namespace DEVILS_SCRIPT_OUTER_NAMESPACE {
#endif // DEVILS_SCRIPT_OUTER_NAMESPACE

  namespace DEVILS_SCRIPT_INNER_NAMESPACE {
    container::container() noexcept : size(0), offset(0), aligment(0), memory(nullptr) {}
    container::container(const size_t &size, const size_t &aligment) noexcept :
      size(size),
      offset(0),
      aligment(aligment),
      memory(nullptr)
    {
      init(size, aligment);
    }

    container::container(container &&move) noexcept :
      size(move.size),
      offset(move.offset),
      aligment(move.aligment),
      memory(move.memory)
    {
      move.size = 0;
      move.offset = 0;
      move.aligment = 0;
      move.memory = nullptr;
    }

    container::~container() noexcept {
      if (memory != nullptr) ::operator delete[] (memory, std::align_val_t{aligment});
    }

    container & container::operator=(container &&move) noexcept {
      size = move.size;
      offset = move.offset;
      aligment = move.aligment;
      memory = move.memory;
      move.size = 0;
      move.offset = 0;
      move.aligment = 0;
      move.memory = nullptr;
      return *this;
    }

    void container::init(const size_t &size, const size_t &aligment) noexcept {
      assert(memory == nullptr);
      this->size = size;
      offset = 0;
      this->aligment = aligment;
      memory = new (std::align_val_t{aligment}) char[size];
      assert(memory != nullptr);
    }

    size_t container::mem_size() const {
      return size;
    }
  }

#ifdef DEVILS_SCRIPT_OUTER_NAMESPACE
}
#endif // DEVILS_SCRIPT_OUTER_NAMESPACE
