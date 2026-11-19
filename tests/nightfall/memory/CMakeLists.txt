# arena_test.cpp joins SOURCES once nf::Arena implements the interface it tests.
tp_add_test(nf_memory_test
    SOURCES box_test.cpp lifetime_test.cpp support/FakeChunkSource.cpp
    LIBRARIES nf::memory
)
