#include "unique_pointer.h"
#include <cstdio>
#include <vector>
#include <utility>
#include <type_traits>

/*=============================================================================
 *  Test Suite for UniquePointer<T>
 *
 *  Covers:
 *    - Construction (default, nullptr, raw pointer)
 *    - Move semantics (construct, assign, source invariants)
 *    - Self-move-assignment safety
 *    - reset() / reset(T*) / release() / swap()
 *    - Dereference, arrow access, explicit bool
 *    - Compile-time contract (copy deleted, move-only)
 *    - Integration (vector, by-value transfer)
 *============================================================================*/

static int g_passed = 0;
static int g_total  = 0;

#define TEST(name)            do { ++g_total; printf("  [TEST] %-50s ", name); fflush(stdout); } while(0)
#define PASS()                do { ++g_passed; printf("✓  PASS\n"); fflush(stdout); } while(0)
#define FAIL(msg)             do { printf("✗  FAIL: %s\n", msg); fflush(stdout); return 1; } while(0)

/* ── Instrumented type ────────────────────────────────── */
struct Loud {
    static inline int balance = 0;
    int id;
    Loud(int i = 0) : id(i) { ++balance; }
    ~Loud() { --balance; }
    Loud(const Loud& o) : id(o.id) { ++balance; }
    Loud& operator=(const Loud& o) { id = o.id; return *this; }
};

/* ── Compile-time contract ───────────────────────────────
 *  UniquePointer 是 move-only 类型：拷贝必须被禁用。
 *  这比运行时测试更硬 —— 拷贝代码根本编不过。          */
static_assert(std::is_move_constructible<UniquePointer<int>>::value,
              "UniquePointer must be move-constructible");
static_assert(std::is_move_assignable<UniquePointer<int>>::value,
              "UniquePointer must be move-assignable");
static_assert(!std::is_copy_constructible<UniquePointer<int>>::value,
              "UniquePointer must NOT be copy-constructible");
static_assert(!std::is_copy_assignable<UniquePointer<int>>::value,
              "UniquePointer must NOT be copy-assignable");

/*==========================================================================*/

static int test_default_construction() {
    TEST("default ctor – null state");
    UniquePointer<int> p;
    if (p.get() != nullptr) FAIL("get() should be nullptr");
    if (p) FAIL("empty pointer should be falsy");
    PASS();
    return 0;
}

static int test_nullptr_ctor() {
    TEST("explicit nullptr ctor – null state");
    UniquePointer<int> p(nullptr);
    if (p.get() != nullptr) FAIL("get() should be nullptr");
    PASS();
    return 0;
}

static int test_ptr_ctor_ownership() {
    TEST("raw-ptr ctor – takes ownership");
    UniquePointer<Loud> p(new Loud(1));
    if (Loud::balance != 1) FAIL("expected 1 object alive");
    {
        UniquePointer<Loud> inner(new Loud(2));
        if (Loud::balance != 2) FAIL("expected 2 objects alive");
    }
    if (Loud::balance != 1) FAIL("inner should be destroyed at scope exit");
    PASS();
    return 0;
}

static int test_deref_and_get() {
    TEST("operator* / get() – value access");
    UniquePointer<int> p(new int(42));
    if (*p != 42) FAIL("dereference gives wrong value");
    if (p.get() == nullptr) FAIL("get() should be non-null");
    *p = 99;
    if (*p != 99) FAIL("write through operator* failed");

    const UniquePointer<int>& cr = p;
    if (*cr != 99) FAIL("const operator* should work");
    PASS();
    return 0;
}

static int test_move_construct() {
    TEST("move construct – source nulled, ownership transferred");
    UniquePointer<Loud> a(new Loud(30));
    UniquePointer<Loud> b(std::move(a));
    if (b.get() == nullptr) FAIL("moved-to should hold the pointer");
    if (a.get() != nullptr) FAIL("moved-from get() should be nullptr");
    if (Loud::balance != 1) FAIL("object leaked or double-freed");
    PASS();
    return 0;
}

static int test_move_assign() {
    TEST("move assign – old released, source nulled");
    UniquePointer<Loud> a(new Loud(40));
    UniquePointer<Loud> b(new Loud(50));
    if (Loud::balance != 2) FAIL("expected 2 objects alive");

    a = std::move(b);
    if (Loud::balance != 1) FAIL("old Loud(40) should be destroyed");
    if (a.get() == nullptr) FAIL("a should hold Loud(50)");
    if (b.get() != nullptr) FAIL("moved-from get() should be nullptr");
    PASS();
    return 0;
}

static int test_move_assign_self() {
    TEST("move self-assignment – no crash, value intact");
    UniquePointer<int> p(new int(88));
    p = std::move(p);
    if (p.get() == nullptr) FAIL("self-move should not null the pointer");
    if (*p != 88) FAIL("value corrupted after self-move");
    PASS();
    return 0;
}

static int test_reset_null() {
    TEST("reset() – releases ownership, null state");
    UniquePointer<Loud> a(new Loud(60));
    a.reset();
    if (a.get() != nullptr) FAIL("get() should be null after reset");
    if (a) FAIL("empty pointer should be falsy");
    if (Loud::balance != 0) FAIL("object not destroyed on reset");
    PASS();
    return 0;
}

static int test_reset_ptr() {
    TEST("reset(T*) – replaces ownership");
    UniquePointer<Loud> a(new Loud(70));
    a.reset(new Loud(80));
    if (Loud::balance != 1) FAIL("old object should be gone");
    if (a->id != 80) FAIL("new object not adopted");

    a.reset();
    if (Loud::balance != 0) FAIL("reset to null should destroy Loud(80)");
    PASS();
    return 0;
}

static int test_release() {
    TEST("release() – surrenders ownership, caller deletes");
    UniquePointer<Loud> a(new Loud(90));
    Loud* raw = a.release();
    if (a.get() != nullptr) FAIL("get() should be null after release");
    if (raw == nullptr) FAIL("release() should return the raw pointer");
    if (Loud::balance != 1) FAIL("object must survive release");
    delete raw;                              /* caller's responsibility now */
    if (Loud::balance != 0) FAIL("object not destroyed by caller");
    PASS();
    return 0;
}

static int test_swap() {
    TEST("swap() – exchanges ownership");
    UniquePointer<Loud> a(new Loud(1));
    UniquePointer<Loud> b(new Loud(2));
    a.swap(b);
    if (a->id != 2 || b->id != 1) FAIL("ownership not exchanged");
    if (Loud::balance != 2) FAIL("no object should be destroyed by swap");
    PASS();
    return 0;
}

static int test_operator_arrow() {
    TEST("operator-> – member access");
    UniquePointer<Loud> p(new Loud(999));
    if (p->id != 999) FAIL("arrow access failed");
    p->id = 1000;
    if (p->id != 1000) FAIL("write through arrow failed");
    PASS();
    return 0;
}

static int test_operator_bool() {
    TEST("operator bool – truthiness");
    UniquePointer<int> empty;
    UniquePointer<int> full(new int(1));
    if (empty) FAIL("empty should be false");
    if (!full) FAIL("non-null should be true");
    full.reset();
    if (full) FAIL("reset pointer should be false");
    PASS();
    return 0;
}

static int test_vector_of_pointers() {
    TEST("vector<UniquePointer> – batch lifecycle");
    std::vector<UniquePointer<Loud>> vec;
    vec.reserve(5);
    for (int i = 0; i < 5; ++i)
        vec.emplace_back(new Loud(i + 100));

    if (Loud::balance != 5) FAIL("expected 5 objects alive");
    if (vec[0]->id != 100) FAIL("first element wrong");

    auto moved = std::move(vec[2]);
    if (vec[2].get() != nullptr) FAIL("moved-from slot should be null");
    if (moved->id != 102) FAIL("moved element wrong");

    vec.clear();
    if (Loud::balance != 1) FAIL("only `moved` should remain alive");
    PASS();
    return 0;
}

static int test_transfer_by_value() {
    TEST("by-value transfer – move-only function call");
    auto make = [](int v) {
        UniquePointer<Loud> p(new Loud(v));
        return p;                            /* NRVO or move out */
    };
    UniquePointer<Loud> p = make(555);
    if (p->id != 555) FAIL("value lost on return");
    if (Loud::balance != 1) FAIL("expected 1 object alive");
    PASS();
    return 0;
}

/*==========================================================================*/

int main() {
    printf("╔═══════════════════════════════════════════════════╗\n");
    printf("║   UniquePointer<T>  —  Test Suite                 ║\n");
    printf("╚═══════════════════════════════════════════════════╝\n\n");

    int failed = 0;
    failed += test_default_construction();
    failed += test_nullptr_ctor();
    failed += test_ptr_ctor_ownership();
    failed += test_deref_and_get();
    failed += test_move_construct();
    failed += test_move_assign();
    failed += test_move_assign_self();
    failed += test_reset_null();
    failed += test_reset_ptr();
    failed += test_release();
    failed += test_swap();
    failed += test_operator_arrow();
    failed += test_operator_bool();
    failed += test_vector_of_pointers();
    failed += test_transfer_by_value();

    printf("\n╔═══════════════════════════════════════════════════╗\n");
    printf("║   Results:  %2d / %2d tests passed", g_passed, g_total);
    if (failed) printf("          ║\n║   ⚠️   %d test(s) FAILED", g_total - g_passed);
    else        printf("          ║\n║   ✅  All tests passed!   ");
    printf("       ║\n");
    printf("╚═══════════════════════════════════════════════════╝\n");
    return failed;
}
