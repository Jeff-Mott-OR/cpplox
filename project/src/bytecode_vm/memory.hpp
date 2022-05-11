#pragma once

#include <functional>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include <gsl/string_span>

namespace motts { namespace lox {
    class GC_heap;

    // Every GC-ed object can be marked and trace references
    struct GC_control_block_base {
        bool marked {false};

        virtual ~GC_control_block_base() = default;
        virtual void trace_refs(GC_heap&) = 0;
        virtual std::size_t size() const = 0;
    };

    // To avoid requiring user types to inherit from something,
    // allow them to trace references through a type trait
    template<typename T>
        void trace_refs_trait(GC_heap&, const T&) {
        }

    // Generate control block types for each user type to hold the value
    // together with a marked flag, and to invoke the correct type trait
    template<typename T>
        struct GC_control_block : GC_control_block_base {
            T value;

            GC_control_block(T&& value_arg) :
                value {std::move(value_arg)}
            {
            }

            void trace_refs(GC_heap& gc_heap) override {
                trace_refs_trait(gc_heap, value);
            }

            std::size_t size() const override {
                return sizeof(*this);
            }
        };

    // A user-friendly wrapper around control block pointers,
    // with same size and cost as a regular pointer
    template<typename T>
        struct GC_ptr {
            GC_control_block<T>* control_block {};

            GC_ptr() = default;

            GC_ptr(GC_control_block<T>* ptr) :
                control_block {ptr}
            {
            }

            // Copy, move construct
            GC_ptr(const GC_ptr<T>& rhs) :
                control_block {rhs.control_block}
            {
            }
            GC_ptr(GC_ptr<T>&& rhs) :
                control_block {rhs.control_block}
            {
            }

            // Copy, move assign
            auto& operator=(const GC_ptr<T>& rhs) {
                control_block = rhs.control_block;
                return *this;
            }
            auto& operator=(GC_ptr<T>&& rhs) {
                control_block = rhs.control_block;
                return *this;
            }

            // Boolean pointer semantics
            operator bool() const {
               return !!control_block;
            }

            // Pointer semantics to access the contained value,
            // so it feels like you have a pointer to the value rather than control block
            auto& operator*() const {
               return control_block->value;
            }
            auto operator->() const {
                return & operator*();
            }
    };
}}

// The hash of a GC_ptr is the hash of the underlying pointer
template<typename T>
    struct std::hash<motts::lox::GC_ptr<T>> {
        auto operator()(const motts::lox::GC_ptr<T>& ptr) const {
            return std::hash<decltype(ptr.control_block)>{}(ptr.control_block);
        }
    };

namespace motts { namespace lox {
    // The comparison of GC_ptrs is the comparison of the underlying pointers
    template<typename T>
        auto operator==(const GC_ptr<T>& a, const GC_ptr<T>& b) {
            return a.control_block == b.control_block;
        }

    class GC_heap {
        std::vector<GC_control_block_base*> all_ptrs_;
        std::vector<GC_control_block_base*> gray_worklist_;
        std::size_t bytes_allocated_ {};

        public:
            std::vector<std::function<void()>> on_mark_roots;
            std::vector<std::function<void(GC_control_block_base&)>> on_destroy_ptr;

            template<typename T>
                GC_ptr<T> make(T&& value) {
                    const auto control_block = new GC_control_block<T>{std::move(value)};
                    all_ptrs_.push_back(control_block);
                    bytes_allocated_ += control_block->size();

                    return control_block;
                }

            ~GC_heap();

            void mark(GC_control_block_base*);

            template<typename T>
                void mark(const GC_ptr<T>& ptr) {
                    mark(ptr.control_block);
                }

            std::size_t size() const;
            void collect_garbage();
    };

    struct Interned_strings : private std::unordered_set<GC_ptr<std::string>> {
        GC_heap& gc_heap;

        Interned_strings(GC_heap&);

        GC_ptr<std::string> get(std::string&&);
        GC_ptr<std::string> get(const gsl::cstring_span<>&);

        // A C-string is implicitly convertible to both std::string and gsl::cstring_span
        // To avoid that ambiguity, explicitly forward to cstring_span
        template<typename T>
            GC_ptr<std::string> get(const T& new_string) {
                return get(gsl::cstring_span<>{new_string});
            }

        using std::unordered_set<GC_ptr<std::string>>::erase;
    };
}}
