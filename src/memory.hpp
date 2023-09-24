#pragma once

#include <functional>
#include <vector>

namespace motts { namespace lox {
    class GC_heap;

    // Every GC-ed object can be marked and trace references
    struct GC_control_block_base {
        bool marked {false};

        virtual ~GC_control_block_base() = default;
        virtual void trace_refs(GC_heap&) = 0;
    };

    // To avoid requiring user types to inherit from something,
    // allow them to trace references through a type trait
    template<typename T>
        void trace_refs_trait(GC_heap&, const T&) {
        }

    // Generate control block types for each user type to hold the value
    // together with the base marked flag, and to invoke the correct type trait
    template<typename T>
        struct GC_control_block : GC_control_block_base {
            T value;

            GC_control_block(T&& value_arg)
                : value {std::move(value_arg)}
            {}

            void trace_refs(GC_heap& gc_heap) override {
                trace_refs_trait(gc_heap, value);
            }
        };

    // A user-friendly wrapper around control block pointers,
    // with same size and cost as a regular pointer
    template<typename T>
        struct GC_ptr {
            GC_control_block<T>* control_block {};

            // Pointer semantics to access the contained value,
            // so it feels like you have a pointer to the value rather than control block
            const T& operator*() const {
               return control_block->value;
            }

            const T* operator->() const {
                return &control_block->value;
            }

            // Boolean pointer semantics
            operator bool() const {
               return !!control_block;
            }
        };

    // The comparison of GC_ptrs is the comparison of the underlying pointers
    template<typename T>
        auto operator==(const GC_ptr<T>& lhs, const GC_ptr<T>& rhs) {
            return lhs.control_block == rhs.control_block;
        }

    class GC_heap {
        std::vector<GC_control_block_base*> all_ptrs_;
        std::vector<GC_control_block_base*> gray_worklist_;

        public:
            std::vector<std::function<void()>> on_mark_roots;

            template<typename T>
                auto make(T&& value) {
                    const auto control_block = new GC_control_block<T>{std::move(value)};
                    all_ptrs_.push_back(control_block);
                    return GC_ptr<T>{control_block};
                }

            ~GC_heap();

            void mark(GC_control_block_base*);

            template<typename T>
                void mark(const GC_ptr<T>& ptr) {
                    mark(ptr.control_block);
                }

            void collect_garbage();
    };
}}
