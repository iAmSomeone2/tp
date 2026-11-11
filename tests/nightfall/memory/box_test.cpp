#include <nightfall/memory/box.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <memory_resource>
#include <type_traits>
#include <vector>

namespace {

// Records every allocation, so a test can check that memory went back where it came from with
// the size and alignment it was allocated with.
class CountingResource : public std::pmr::memory_resource {
public:
    struct Block {
        void* p;
        std::size_t size;
        std::size_t align;
    };
    std::vector<Block> live;
    int frees = 0;

private:
    void* do_allocate(std::size_t size, std::size_t align) override {
        void* p = std::pmr::new_delete_resource()->allocate(size, align);
        live.push_back({p, size, align});
        return p;
    }
    void do_deallocate(void* p, std::size_t size, std::size_t align) override {
        for (auto it = live.begin(); it != live.end(); ++it) {
            if (it->p == p) {
                EXPECT_EQ(it->size, size);
                EXPECT_EQ(it->align, align);
                live.erase(it);
                frees++;
                std::pmr::new_delete_resource()->deallocate(p, size, align);
                return;
            }
        }
        ADD_FAILURE() << "freed a pointer this resource did not allocate";
    }
    bool do_is_equal(const memory_resource& o) const noexcept override { return this == &o; }
};

struct Tracked {
    int* destroyed;
    int value;
    Tracked(int* d, int v) : destroyed(d), value(v) {}
    ~Tracked() { ++*destroyed; }
};

struct Base {
    virtual ~Base() = default;
};
struct Derived : Base {
    char extra[32];
};

struct alignas(64) OverAligned {
    char c;
};

TEST(NfBox, ConstructsAndReturnsMemoryToItsResource) {
    CountingResource a, b;
    int destroyed = 0;
    {
        nf::Box<Tracked> x = nf::make_box<Tracked>(a, &destroyed, 7);
        nf::Box<Tracked> y = nf::make_box<Tracked>(b, &destroyed, 8);
        EXPECT_EQ(x->value, 7);
        EXPECT_EQ(y->value, 8);
        ASSERT_EQ(a.live.size(), 1u);
        ASSERT_EQ(b.live.size(), 1u);
        EXPECT_EQ(a.live[0].size, sizeof(Tracked));
        EXPECT_EQ(a.live[0].align, alignof(Tracked));
    }
    EXPECT_EQ(destroyed, 2);
    EXPECT_EQ(a.frees, 1);
    EXPECT_EQ(b.frees, 1);
    EXPECT_TRUE(a.live.empty());
    EXPECT_TRUE(b.live.empty());
}

TEST(NfBox, MoveKeepsTheResource) {
    CountingResource a;
    int destroyed = 0;
    nf::Box<Tracked> moved;
    {
        nf::Box<Tracked> x = nf::make_box<Tracked>(a, &destroyed, 1);
        moved = std::move(x);
    }
    EXPECT_EQ(destroyed, 0);
    EXPECT_EQ(moved.get_deleter().resource, &a);
    moved.reset();
    EXPECT_EQ(destroyed, 1);
    EXPECT_EQ(a.frees, 1);
}

TEST(NfBox, HonoursOverAlignedTypes) {
    CountingResource a;
    nf::Box<OverAligned> x = nf::make_box<OverAligned>(a);
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(x.get()) % 64, 0u);
    EXPECT_EQ(a.live[0].align, 64u);
}

// Deallocating a Derived through Box<Base> would pass sizeof(Base), so the conversion is refused.
static_assert(!std::is_convertible_v<nf::Box<Derived>, nf::Box<Base>>);
static_assert(!std::is_constructible_v<nf::Box<Base>, nf::Box<Derived>&&>);
static_assert(!std::is_copy_constructible_v<nf::Box<int>>);
static_assert(sizeof(nf::Box<int>) == 2 * sizeof(void*));

}  // namespace
