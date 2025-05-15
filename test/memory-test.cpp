#define BOOST_TEST_MODULE Memory Tests

#include <sstream>

#include <boost/test/unit_test.hpp>

#include "../src/memory.hpp"

struct Destruct_tracer
{
    std::ostream* os;
    const char* name;

    Destruct_tracer(std::ostream& os_arg, const char* name_arg = "Destruct_tracer")
        : os{&os_arg},
          name{name_arg}
    {
    }

    // Skip tracing of moved-from temporaries.
    Destruct_tracer(Destruct_tracer&& other)
        : os{other.os},
          name{other.name}
    {
        other.os = nullptr;
    }

    ~Destruct_tracer()
    {
        if (os) {
            *os << '~' << name << '\n';
        }
    }
};

// Tell the garbage collector how to trace references of a `Destruct_tracer`.
template<>
void motts::lox::trace_refs_trait(motts::lox::GC_heap&, const Destruct_tracer& tracer)
{
    *tracer.os << tracer.name << "::trace_refs\n";
}

BOOST_AUTO_TEST_CASE(control_block_wraps_value_with_marked_flag)
{
    motts::lox::GC_control_block<int> control_block_int{42};

    BOOST_TEST(control_block_int.value == 42);
    BOOST_TEST(control_block_int.marked == false);
}

BOOST_AUTO_TEST_CASE(gc_ptr_wraps_control_block)
{
    motts::lox::GC_control_block<std::string> control_block_str{"Hello, World!"};
    motts::lox::GC_ptr<std::string> gc_ptr_str{&control_block_str};

    BOOST_TEST(*gc_ptr_str == control_block_str.value);
    BOOST_TEST(gc_ptr_str->data() == control_block_str.value.data());
    BOOST_TEST(gc_ptr_str.control_block == &control_block_str);
    BOOST_TEST(static_cast<bool>(gc_ptr_str) == true);

    motts::lox::GC_ptr<std::string> gc_ptr_str_same{&control_block_str};

    BOOST_TEST(gc_ptr_str == gc_ptr_str_same);

    motts::lox::GC_ptr<std::string> null_gc_ptr_str;

    BOOST_TEST(static_cast<bool>(null_gc_ptr_str) == false);
}

BOOST_AUTO_TEST_CASE(gc_heap_will_make_and_own_control_block_gc_ptr)
{
    std::ostringstream os;
    {
        motts::lox::GC_heap gc_heap;
        auto gc_ptr_tracer = gc_heap.make<Destruct_tracer>({os});
        static_cast<void>(gc_ptr_tracer); // Suppress unused variable error.

        BOOST_TEST(os.str() == "");
    } // GC_heap and all owned pointers destroyed.

    BOOST_TEST(os.str() == "~Destruct_tracer\n");
}

BOOST_AUTO_TEST_CASE(gc_heap_collect_will_invoke_mark_roots_callbacks)
{
    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    gc_heap.on_mark_roots.push_back([&] { os << "on_mark_roots\n"; });

    BOOST_TEST(os.str() == "");

    gc_heap.collect_garbage();

    BOOST_TEST(os.str() == "on_mark_roots\n");
}

BOOST_AUTO_TEST_CASE(control_blocks_and_gc_ptrs_can_be_marked)
{
    motts::lox::GC_heap gc_heap;
    auto gc_ptr_int = gc_heap.make<int>(42);

    BOOST_TEST(gc_ptr_int.control_block->marked == false);

    mark(gc_heap, gc_ptr_int);

    BOOST_TEST(gc_ptr_int.control_block->marked == true);
}

BOOST_AUTO_TEST_CASE(gc_heap_collect_will_invoke_trace_refs_trait)
{
    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    auto gc_ptr_tracer = gc_heap.make<Destruct_tracer>({os});
    gc_heap.on_mark_roots.push_back([&] { mark(gc_heap, gc_ptr_tracer); });

    BOOST_TEST(os.str() == "");

    gc_heap.collect_garbage();

    BOOST_TEST(os.str() == "Destruct_tracer::trace_refs\n");
}

BOOST_AUTO_TEST_CASE(gc_heap_collect_will_destroy_unmarked_objects)
{
    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    auto gc_ptr_tracer_1 = gc_heap.make<Destruct_tracer>({os, "Tracer1"});
    auto gc_ptr_tracer_2 = gc_heap.make<Destruct_tracer>({os, "Tracer2"});

    mark(gc_heap, gc_ptr_tracer_1);
    static_cast<void>(gc_ptr_tracer_2); // Suppress unused variable error.

    BOOST_TEST(os.str() == "");

    gc_heap.collect_garbage();

    BOOST_TEST(os.str() == "Tracer1::trace_refs\n~Tracer2\n");
}

BOOST_AUTO_TEST_CASE(gc_heap_will_report_current_allocation_size)
{
    motts::lox::GC_heap gc_heap;
    auto gc_ptr_1 = gc_heap.make<int>(42);
    auto gc_ptr_2 = gc_heap.make<int>(42);

    BOOST_TEST(gc_heap.size() == sizeof(motts::lox::GC_control_block<int>) * 2);

    mark(gc_heap, gc_ptr_1);
    static_cast<void>(gc_ptr_2); // Suppress unused variable error.

    gc_heap.collect_garbage();

    BOOST_TEST(gc_heap.size() == sizeof(motts::lox::GC_control_block<int>) * 1);
}
