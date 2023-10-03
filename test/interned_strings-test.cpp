#define BOOST_TEST_MODULE Interned Strings Tests
#include <boost/test/unit_test.hpp>

#include <string>

#include "../src/interned_strings.hpp"

BOOST_AUTO_TEST_CASE(interned_string_views_make_owning_copy_and_dedup)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings {gc_heap};

    std::string_view str_view {"hello"};
    const auto interned_str_gc_ptr = interned_strings.get(str_view);

    BOOST_TEST(*interned_str_gc_ptr == str_view);
    BOOST_TEST(reinterpret_cast<const void*>(&*(interned_str_gc_ptr->cbegin())) != reinterpret_cast<const void*>(&*(str_view.cbegin())));

    std::string_view str_view_2 {"hello"};
    const auto interned_str_gc_ptr_2 = interned_strings.get(str_view_2);

    BOOST_TEST(interned_str_gc_ptr_2 == interned_str_gc_ptr);
}

BOOST_AUTO_TEST_CASE(interned_strings_are_weakref_and_delete_if_collected)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings {gc_heap};

    const auto gc_ptr_not_marked = interned_strings.get(std::string_view{"hello"});
    const auto gc_ptr_not_marked_dup = interned_strings.get(std::string_view{"hello"});

    BOOST_TEST(gc_ptr_not_marked == gc_ptr_not_marked_dup);

    gc_heap.collect_garbage();

    // The expectation is that interned strings will allocate a brand new "hello" string,
    // but we might still get the same pointer address just by luck.
    interned_strings.get(std::string_view{"dummy"});

    const auto gc_ptr_after_collect = interned_strings.get(std::string_view{"hello"});

    BOOST_TEST(gc_ptr_after_collect != gc_ptr_not_marked);
}
