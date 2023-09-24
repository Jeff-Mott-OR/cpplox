#define BOOST_TEST_MODULE Memory Tests
#include <boost/test/unit_test.hpp>

#include <sstream>

#include "../src/memory.hpp"

// Not exported (internal linkage)
namespace {
    struct Tracer_destroy {
        std::ostream& os;
        bool owner {true};

        Tracer_destroy(std::ostream& os_arg)
            : os {os_arg}
        {}

        ~Tracer_destroy() {
            if (owner) {
                os << "~Tracer\n";
            }
        }

        Tracer_destroy(Tracer_destroy&& other)
            : os {other.os}
        {
            other.owner = false;
        }
    };

    struct Tracer_refs {
        std::ostream& os;
    };
}

template<>
    void motts::lox::trace_refs_trait(motts::lox::GC_heap&, const Tracer_refs& tracer) {
        tracer.os << "Tracer -> trace_refs\n";
    }

BOOST_AUTO_TEST_CASE(control_block_wraps_value_with_marked_flag) {
    motts::lox::GC_control_block<int> control_block_int {42};

    BOOST_TEST(control_block_int.value == 42);
    BOOST_TEST(control_block_int.marked == false);
}

BOOST_AUTO_TEST_CASE(gc_ptr_wraps_control_block) {
    motts::lox::GC_ptr<int> null_gc_ptr_int;
    BOOST_TEST(static_cast<bool>(null_gc_ptr_int) == false);

    motts::lox::GC_control_block<int> control_block_int {42};
    motts::lox::GC_ptr<int> gc_ptr_int {&control_block_int};
    BOOST_TEST(static_cast<bool>(gc_ptr_int) == true);
    BOOST_TEST(gc_ptr_int.control_block == &control_block_int);
    BOOST_TEST(*gc_ptr_int == control_block_int.value);

    motts::lox::GC_ptr<int> gc_ptr_int_same {&control_block_int};
    BOOST_TEST(gc_ptr_int == gc_ptr_int_same);
}

BOOST_AUTO_TEST_CASE(gc_heap_will_make_and_own_control_block_gc_ptr) {
    std::ostringstream os;
    {
        motts::lox::GC_heap gc_heap;

        auto gc_ptr_tracer = gc_heap.make<Tracer_destroy>({os});
        static_cast<void>(gc_ptr_tracer); // suppress unused variable error

        BOOST_TEST(os.str() == "");
    }
    BOOST_TEST(os.str() == "~Tracer\n");
}

BOOST_AUTO_TEST_CASE(control_blocks_and_gc_ptrs_can_be_marked) {
    motts::lox::GC_heap gc_heap;
    auto gc_ptr_int = gc_heap.make<int>(42);
    BOOST_TEST(gc_ptr_int.control_block->marked == false);
    gc_heap.mark(gc_ptr_int);
    BOOST_TEST(gc_ptr_int.control_block->marked == true);
}

BOOST_AUTO_TEST_CASE(gc_heap_collect_will_destroy_unmarked_objects) {
    motts::lox::GC_heap gc_heap;

    std::ostringstream os;
    auto gc_ptr_tracer = gc_heap.make<Tracer_destroy>({os});
    static_cast<void>(gc_ptr_tracer); // suppress unused variable error

    BOOST_TEST(os.str() == "");
    gc_heap.collect_garbage();
    BOOST_TEST(os.str() == "~Tracer\n");
}

BOOST_AUTO_TEST_CASE(gc_heap_collect_will_invoke_mark_roots_callbacks) {
    motts::lox::GC_heap gc_heap;

    std::ostringstream os;
    auto gc_ptr_tracer = gc_heap.make<Tracer_destroy>({os});

    gc_heap.on_mark_roots.push_back([&] {
        gc_heap.mark(gc_ptr_tracer);
    });
    gc_heap.collect_garbage();

    BOOST_TEST(os.str() == "");
}

BOOST_AUTO_TEST_CASE(gc_heap_collect_will_invoke_trace_refs_trait) {
    motts::lox::GC_heap gc_heap;

    std::ostringstream os;
    auto gc_ptr_tracer = gc_heap.make<Tracer_refs>({os});

    gc_heap.on_mark_roots.push_back([&] {
        gc_heap.mark(gc_ptr_tracer);
    });
    gc_heap.collect_garbage();

    BOOST_TEST(os.str() == "Tracer -> trace_refs\n");
}
