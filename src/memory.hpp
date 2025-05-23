#pragma once

#include <functional>
#include <memory>
#include <ostream>
#include <vector>

namespace motts::lox
{
    class GC_heap;

    // Every garbage collect-able object can be polymorphically marked and trace references.
    struct GC_control_block_base
    {
        bool marked{false};

        virtual ~GC_control_block_base() = default;
        virtual void trace_refs(GC_heap&) const = 0;
        virtual std::size_t size() const = 0;
    };

    // I don't want user code to be required to extend my GC class, so instead I'll let them specialize a
    // function template, and I'll implement the virtual trace_refs for them to invoke the specialization.
    template<typename User_value_type>
    void trace_refs_trait(GC_heap&, const User_value_type&)
    {
        // Default is no-op.
    }

    // Generate a concrete derived control block type for each user value type.
    // It will hold the user value together with the base marked flag,
    // and it will invoke the correct trace_refs_trait specialization.
    template<typename User_value_type>
    struct GC_control_block : GC_control_block_base
    {
        User_value_type value;

        GC_control_block(User_value_type&& value_arg)
            : value{std::move(value_arg)}
        {
        }

        void trace_refs(GC_heap& gc_heap) const override
        {
            trace_refs_trait(gc_heap, value);
        }

        std::size_t size() const override
        {
            return sizeof(*this);
        }
    };

    // In user code, a concrete derived control block would annoyingly require
    // a lot of `value` accesses, such as `foo->value.bar->value.baz->value`.
    // So this is a user-friendly wrapper around control block pointers with the same size and cost
    // as a regular pointer. With this wrapper, user code can instead write `*foo->bar->baz`.
    template<typename User_value_type>
    struct GC_ptr
    {
        GC_control_block<User_value_type>* control_block{};

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

    // Printing GC_ptrs prints the underlying control block pointer.
    template<typename User_value_type>
    std::ostream& operator<<(std::ostream& os, GC_ptr<User_value_type> ptr)
    {
        os << ptr.control_block;
        return os;
    }

    class GC_heap
    {
        std::vector<std::unique_ptr<GC_control_block_base>> all_ptrs_;
        std::vector<GC_control_block_base*> gray_worklist_;
        std::size_t n_allocated_bytes_{0};

      public:
        // When we mark-and-sweep, we need to start marking somewhere.
        // Add a callback to this list to mark your roots, whatever they may be.
        std::vector<std::function<void()>> on_mark_roots;

        // Before we delete a ptr during collection, give others a chance to act on the pending deletion.
        std::vector<std::function<void(const GC_control_block_base&)>> on_destroy_ptr;

        GC_heap() = default;

        // Non-copyable. This is a resource owning class.
        GC_heap(const GC_heap&) = delete;
        GC_heap& operator=(const GC_heap&) = delete;

        // Move your value into a heap allocated and tracked control block.
        template<typename User_value_type>
        GC_ptr<User_value_type> make(User_value_type&& value)
        {
            auto control_block = std::make_unique<GC_control_block<User_value_type>>(std::move(value));
            const GC_ptr<User_value_type> gc_ptr{control_block.get()};

            n_allocated_bytes_ += control_block->size();
            all_ptrs_.push_back(std::move(control_block));

            return gc_ptr;
        }

        // Mark as reachable, and queue to trace references.
        void mark(GC_control_block_base&);

        // Mark all roots, trace all references, and delete anything that isn't reachable.
        void collect_garbage();

        // Report number of bytes allocated by this heap.
        std::size_t size() const;
    };

    template<typename User_value_type>
    void mark(GC_heap& gc_heap, GC_ptr<User_value_type> gc_ptr)
    {
        if (gc_ptr) {
            gc_heap.mark(*gc_ptr.control_block);
        }
    }
}

// The hash of GC_ptrs is the hash of the underlying control block pointers.
template<typename User_value_type>
struct std::hash<motts::lox::GC_ptr<User_value_type>>
{
    std::size_t operator()(motts::lox::GC_ptr<User_value_type> gc_ptr) const
    {
        return std::hash<motts::lox::GC_control_block_base*>{}(gc_ptr.control_block);
    }
};
