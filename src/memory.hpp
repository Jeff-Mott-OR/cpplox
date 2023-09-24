#pragma once

#include <functional>
#include <vector>

namespace motts { namespace lox
{
    class GC_heap;

    // Every garbage collect-able object can be polymorphically "marked" and trace references.
    struct GC_control_block_base
    {
        bool marked {false};

        virtual ~GC_control_block_base() = default;
        virtual void trace_refs(GC_heap&) = 0;
    };

    // I don't want user code to be required to extend some GC class to implement my virtual methods, so instead
    // I'll let them specialize a function template, and I'll implement `trace_refs` for them to invoke the correct specialization.
    template<typename User_value_type>
        void trace_refs_trait(GC_heap&, const User_value_type&)
        {
            // Default is no-op.
        }

    // Generate a concrete derived control block type for each user value type.
    // It will hold the user value together with the base marked flag,
    // and it will invoke the correct `trace_refs_trait` specialization.
    template<typename User_value_type>
        struct GC_control_block : GC_control_block_base
        {
            User_value_type value;

            GC_control_block(User_value_type&& value_arg)
                : value {std::move(value_arg)}
            {
            }

            void trace_refs(GC_heap& gc_heap) override
            {
                trace_refs_trait(gc_heap, value);
            }
        };

    // In user code, a concrete derived control block would annoyingly require a lot of `value` accesses,
    // such as `foo_cb_ptr->value.bar_cb_ptr->value.baz_cb_ptr->value`.
    // So this is a user-friendly wrapper around control block pointers with the same size and cost as a regular pointer.
    // With this wrapper, user code can instead write `*foo_gc_ptr->bar_gc_ptr->baz_gc_ptr`.
    template<typename User_value_type>
        struct GC_ptr
        {
            GC_control_block<User_value_type>* control_block {};

            const User_value_type& operator*() const
            {
               return control_block->value;
            }

            User_value_type& operator*()
            {
               return control_block->value;
            }

            const User_value_type* operator->() const
            {
                return &control_block->value;
            }

            User_value_type* operator->()
            {
                return &control_block->value;
            }

            operator bool() const
            {
               return control_block != nullptr;
            }
        };

    // The comparison of GC_ptrs is the comparison of the underlying control block pointers.
    template<typename User_value_type>
        auto operator==(GC_ptr<User_value_type> lhs, GC_ptr<User_value_type> rhs)
        {
            return lhs.control_block == rhs.control_block;
        }

    class GC_heap
    {
        std::vector<GC_control_block_base*> all_ptrs_;
        std::vector<GC_control_block_base*> gray_worklist_;

        public:
            // When we mark-and-sweep, we need to start marking somewhere.
            // Add a callback to this list to mark your roots, whatever they may be.
            std::vector<std::function<void()>> on_mark_roots;

            GC_heap() = default;

            // This is a resource owning class, so disable copying.
            GC_heap(const GC_heap&) = delete;

            // Move your value into a heap allocated and tracked control block.
            template<typename User_value_type>
                GC_ptr<User_value_type> make(User_value_type&& value)
                {
                    auto* control_block = new GC_control_block<User_value_type>{std::move(value)};
                    all_ptrs_.push_back(control_block);
                    return {control_block};
                }

            ~GC_heap();

            // Mark as reachable, and queue to trace references.
            void mark(GC_control_block_base*);

            // Mark all roots, trace all references, and delete anything that isn't reachable.
            void collect_garbage();
    };

    template<typename User_value_type>
        void mark(GC_heap& gc_heap, GC_ptr<User_value_type> gc_ptr)
        {
            gc_heap.mark(gc_ptr.control_block);
        }
}}
