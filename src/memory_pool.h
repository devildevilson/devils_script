#ifndef DEVILS_ENGINE_UTILS_MEMORY_POOL_NEW_H
#define DEVILS_ENGINE_UTILS_MEMORY_POOL_NEW_H

#include <cstddef>
#include <algorithm>
#include <cassert>

#ifndef DEVILS_SCRIPT_INNER_NAMESPACE
#  define DEVILS_SCRIPT_INNER_NAMESPACE devils_script
#endif // DEVILS_SCRIPT_INNER_NAMESPACE

#ifdef DEVILS_SCRIPT_OUTER_NAMESPACE
namespace DEVILS_SCRIPT_OUTER_NAMESPACE {
#endif // DEVILS_SCRIPT_OUTER_NAMESPACE

  namespace DEVILS_SCRIPT_INNER_NAMESPACE {
    namespace utils {
      template <typename T, size_t N = 4096>
      class memory_pool {
      public:
        using elem_ptr = T*;

        memory_pool() noexcept : memory(nullptr), current(0), free_memory(nullptr) {}

        memory_pool(const memory_pool &copy) noexcept = delete;
        memory_pool & operator=(const memory_pool &copy) noexcept = delete;
        memory_pool(memory_pool &&move) noexcept : memory(move.memory), current(move.current), free_memory(move.free_memory) {
          move.memory = nullptr;
          move.current = 0;
          move.free_memory = nullptr;
        }

        memory_pool & operator=(memory_pool &&move) noexcept {
          clear();

          memory = move.memory;
          current = move.current;
          free_memory = move.free_memory;
          move.memory = nullptr;
          move.current = 0;
          move.free_memory = nullptr;
          return *this;
        }

        ~memory_pool() noexcept { clear(); }

        template <typename ...Args>
        elem_ptr create(Args&& ...args) {
          auto ptr = allocate();
          elem_ptr p = new (ptr) T(std::forward<Args>(args)...);
          return p;
        }

        void destroy(elem_ptr ptr) noexcept {
          if (ptr == nullptr) return;
          ptr->~T();
          reinterpret_cast<char**>(ptr)[0] = free_memory;
          free_memory = reinterpret_cast<char*>(ptr);
        }

        char* allocate() noexcept {
          if (free_memory != nullptr) {
            auto ptr = free_memory;
            free_memory = reinterpret_cast<char**>(free_memory)[0];
            return ptr;
          }

          constexpr size_t block_size = final_block_size();
          if (memory == nullptr || current + element_size() > block_size) allocate_memory();
          auto ptr = &memory[current];
          current += element_size();
          return ptr;
        }

        void clear() noexcept {
          char* old_mem = memory;
          while (old_mem != nullptr) {
            char* tmp = reinterpret_cast<char**>(old_mem)[0];
            ::operator delete[] (old_mem, std::align_val_t{alignof(T)});
            old_mem = tmp;
          }

          memory = nullptr;
          free_memory = nullptr;
          current = 0;
        }

        size_t blocks_allocated() const noexcept {
          size_t counter = 0;
          for (char* old_mem = memory; old_mem != nullptr; old_mem = reinterpret_cast<char**>(old_mem)[0]) { counter += 1; }
          return counter;
        }

        size_t free_elements_count() const noexcept {
          size_t counter = 0;
          for (char* old_mem = free_memory; old_mem != nullptr; old_mem = reinterpret_cast<char**>(old_mem)[0]) { counter += 1; }
          return counter;
        }

        constexpr static size_t block_elem_count() noexcept {
          return N / element_size();
        }

        constexpr static size_t block_size() noexcept {
          return final_block_size();
        }
      private:
        char* memory;
        size_t current;
        char* free_memory;

        constexpr static size_t align_to(const size_t &mem, const size_t &align) noexcept {
          return (mem + align - 1) / align * align;
        }

        constexpr static size_t element_size() noexcept {
          return std::max(sizeof(T), sizeof(char*));
        }

        constexpr static size_t element_alignment() noexcept {
          return std::max(alignof(T), alignof(char*));
        }

        constexpr static size_t final_block_size() noexcept {
          const size_t elem_size = element_size();
          const size_t elem_align = element_alignment();
          const size_t count = block_elem_count();
          const size_t mem = align_to(count * elem_size, elem_align);
          //const size_t diff = N - mem;
          //return diff < elem_align ? N + elem_align - diff : N;
          return mem + elem_align;
        }

        void allocate_memory() noexcept {
          const size_t block_size = final_block_size();
          char* new_memory = new (std::align_val_t{alignof(T)}) char[block_size];
          assert(new_memory != nullptr);
          reinterpret_cast<char**>(new_memory)[0] = memory;
          memory = new_memory;
          //const size_t offset = N % alignof(T);
          //current_memory = new_memory + element_alignment(); //  + offset
          //last_memory = new_memory + block_size;
          current = element_alignment();
        }
      };
    }
  }

#ifdef DEVILS_SCRIPT_OUTER_NAMESPACE
}
#endif // DEVILS_SCRIPT_OUTER_NAMESPACE

#endif
