# arena_test.cpp joins SOURCES once nf::Arena implements the interface it tests.
tp_add_test(nf_memory_test
        SOURCES box_test.cpp block_table_test.cpp lifetime_test.cpp
        LIBRARIES nf::memory
)
