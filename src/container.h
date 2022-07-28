#ifndef DEVILS_ENGINE_SCRIPT_CONTAINER_H
#define DEVILS_ENGINE_SCRIPT_CONTAINER_H

#include <string>
#include <stdexcept>

#ifndef DEVILS_SCRIPT_INNER_NAMESPACE
#define DEVILS_SCRIPT_INNER_NAMESPACE devils_script
#endif // DEVILS_SCRIPT_INNER_NAMESPACE

#ifdef DEVILS_SCRIPT_OUTER_NAMESPACE
namespace DEVILS_SCRIPT_OUTER_NAMESPACE {
#endif // DEVILS_SCRIPT_OUTER_NAMESPACE

  namespace DEVILS_SCRIPT_INNER_NAMESPACE {
    constexpr size_t align_to(size_t memory, size_t aligment) {
      return (memory + aligment - 1) / aligment * aligment;
    }

    class interface;

    class container {
    public:
      template <typename T>
      class delayed_initialization {
      public:
        delayed_initialization(void* ptr) noexcept : ptr(ptr), inited(false) {}
        delayed_initialization(const delayed_initialization &copy) noexcept = delete;
        delayed_initialization(delayed_initialization &&move) noexcept : ptr(move.ptr), inited(move.inited) { move.ptr = nullptr; move.inited = false; }
        delayed_initialization & operator=(const delayed_initialization &copy) noexcept = delete;
        delayed_initialization & operator=(delayed_initialization &&move) noexcept {
          ptr = move.ptr;
          inited = move.inited;
          move.ptr = nullptr;
          move.inited = false;
          return *this;
        }

        template <typename... Args>
        T* init(Args&& ...args) {
          //if (ptr == nullptr) throw std::runtime_error("Invalid initialization");
          if (ptr == nullptr) return nullptr;
          if (inited) throw std::runtime_error("Memory is already inited");
          auto obj = new (ptr) T(std::forward<Args>(args)...);
          inited = true;
          return obj;
        }

        bool valid() const noexcept { return ptr != nullptr; }
        bool is_inited() const noexcept { return inited; }
      private:
        void* ptr;
        bool inited;
      };

      container() noexcept;
      container(const size_t &size, const size_t &aligment) noexcept;
      container(container &&move) noexcept;
      container(const container &copy) noexcept = delete;
      ~container() noexcept;
      container & operator=(const container &copy) noexcept = delete;
      container & operator=(container &&move) noexcept;

      bool valid() const noexcept { return memory != nullptr; }
      void init(const size_t &size, const size_t &aligment) noexcept;

      template <typename T, typename... Args>
      T* add(Args&& ...args) {
        if (memory == nullptr) return nullptr;
        if (offset + align_to(sizeof(T), aligment) > size)
          throw std::runtime_error("Container overflow (" + std::to_string(offset) + " + " + std::to_string(align_to(sizeof(T), aligment)) + " > " + std::to_string(size) + ")");
        auto ptr = &memory[offset];
        auto obj = new (ptr) T(std::forward<Args>(args)...);
        offset += align_to(sizeof(T), aligment);
        return obj;
      }

      template <typename T>
      size_t add_delayed() {
        if (memory == nullptr) return SIZE_MAX;
        if (offset + align_to(sizeof(T), aligment) > size)
          throw std::runtime_error("Container overflow (" + std::to_string(offset) + " + " + std::to_string(align_to(sizeof(T), aligment)) + " > " + std::to_string(size) + ")");
        const size_t cur = offset;
        offset += align_to(sizeof(T), aligment);
        return cur;
      }

      template <typename T>
      delayed_initialization<T> get_init(const size_t &offset) {
        if (memory == nullptr || offset == SIZE_MAX) return delayed_initialization<T>(nullptr);
        if (offset + align_to(sizeof(T), aligment) > size)
          throw std::runtime_error("Container overflow (" + std::to_string(offset) + " + " + std::to_string(align_to(sizeof(T), aligment)) + " = " + std::to_string(size) + ")");
        auto cur = &memory[offset];
        return delayed_initialization<T>(cur);
      }

      template <typename T>
      void destroy(T* ptr) { ptr->~T(); }

      size_t mem_size() const;
    private:
      size_t size;
      size_t offset;
      size_t aligment;
      char* memory;
    };
  }

#ifdef DEVILS_SCRIPT_OUTER_NAMESPACE
}
#endif // DEVILS_SCRIPT_OUTER_NAMESPACE

#endif
