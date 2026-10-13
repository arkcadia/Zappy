#include <catch2/catch_test_macros.hpp>
#include "../src/map/Vector.hpp"

TEST_CASE("Vector2 default constructor", "[vector]") {
    [[maybe_unused]] Vector2<int> v;
    // default-constructed, values are indeterminate for POD — just check it compiles
    Vector2<int> v2(0, 0);
    REQUIRE(v2.x == 0);
    REQUIRE(v2.y == 0);
}

TEST_CASE("Vector2 parameterized constructor", "[vector]") {
    Vector2i v(3, 7);
    REQUIRE(v.x == 3);
    REQUIRE(v.y == 7);
}

TEST_CASE("Vector2 unary negation", "[vector]") {
    Vector2i v(3, -5);
    Vector2i neg = -v;
    REQUIRE(neg.x == -3);
    REQUIRE(neg.y == 5);
}

TEST_CASE("Vector2 addition", "[vector]") {
    Vector2i a(1, 2);
    Vector2i b(3, 4);
    Vector2i r = a + b;
    REQUIRE(r.x == 4);
    REQUIRE(r.y == 6);
}

TEST_CASE("Vector2 addition with negatives", "[vector]") {
    Vector2i a(-5, 10);
    Vector2i b(3, -7);
    Vector2i r = a + b;
    REQUIRE(r.x == -2);
    REQUIRE(r.y == 3);
}

TEST_CASE("Vector2 subtraction", "[vector]") {
    Vector2i a(10, 20);
    Vector2i b(3, 5);
    Vector2i r = a - b;
    REQUIRE(r.x == 7);
    REQUIRE(r.y == 15);
}

TEST_CASE("Vector2 += operator", "[vector]") {
    Vector2i a(1, 2);
    Vector2i b(3, 4);
    a += b;
    REQUIRE(a.x == 4);
    REQUIRE(a.y == 6);
}

TEST_CASE("Vector2 -= operator", "[vector]") {
    Vector2i a(10, 20);
    Vector2i b(3, 5);
    a -= b;
    REQUIRE(a.x == 7);
    REQUIRE(a.y == 15);
}

TEST_CASE("Vector2 scalar multiplication (vec * scalar)", "[vector]") {
    Vector2i v(3, 4);
    Vector2i r = v * 2;
    REQUIRE(r.x == 6);
    REQUIRE(r.y == 8);
}

TEST_CASE("Vector2 scalar multiplication (scalar * vec)", "[vector]") {
    Vector2i v(3, 4);
    Vector2i r = 3 * v;
    REQUIRE(r.x == 9);
    REQUIRE(r.y == 12);
}

TEST_CASE("Vector2 *= operator", "[vector]") {
    Vector2i v(3, 4);
    v *= 5;
    REQUIRE(v.x == 15);
    REQUIRE(v.y == 20);
}

TEST_CASE("Vector2 division", "[vector]") {
    Vector2i v(10, 20);
    Vector2i r = v / 5;
    REQUIRE(r.x == 2);
    REQUIRE(r.y == 4);
}

TEST_CASE("Vector2 /= operator", "[vector]") {
    Vector2i v(10, 20);
    v /= 2;
    REQUIRE(v.x == 5);
    REQUIRE(v.y == 10);
}

TEST_CASE("Vector2 modulo wrapping positive values", "[vector]") {
    Vector2i v(12, 15);
    Vector2i mod(10, 10);
    Vector2i r = v % mod;
    REQUIRE(r.x == 2);
    REQUIRE(r.y == 5);
}

TEST_CASE("Vector2 modulo wrapping negative values", "[vector]") {
    Vector2i v(-1, -3);
    Vector2i mod(10, 10);
    Vector2i r = v % mod;
    REQUIRE(r.x == 9);
    REQUIRE(r.y == 7);
}

TEST_CASE("Vector2 modulo wrapping exact boundary", "[vector]") {
    Vector2i v(10, 10);
    Vector2i mod(10, 10);
    Vector2i r = v % mod;
    REQUIRE(r.x == 0);
    REQUIRE(r.y == 0);
}

TEST_CASE("Vector2 modulo wrapping zero", "[vector]") {
    Vector2i v(0, 0);
    Vector2i mod(10, 10);
    Vector2i r = v % mod;
    REQUIRE(r.x == 0);
    REQUIRE(r.y == 0);
}

TEST_CASE("Vector2 modulo wrapping large negative", "[vector]") {
    Vector2i v(-25, -37);
    Vector2i mod(10, 10);
    Vector2i r = v % mod;
    REQUIRE(r.x == 5);
    REQUIRE(r.y == 3);
}

TEST_CASE("Vector2 equality", "[vector]") {
    Vector2i a(3, 4);
    Vector2i b(3, 4);
    REQUIRE(a == b);
}

TEST_CASE("Vector2 inequality", "[vector]") {
    Vector2i a(3, 4);
    Vector2i b(5, 6);
    REQUIRE(a != b);
}

TEST_CASE("Vector2 equality partial mismatch", "[vector]") {
    Vector2i a(3, 4);
    Vector2i b(3, 5);
    REQUIRE_FALSE(a == b);
}

TEST_CASE("Vector2 float operations", "[vector]") {
    Vector2f a(1.5f, 2.5f);
    Vector2f b(0.5f, 1.0f);
    Vector2f r = a + b;
    REQUIRE(r.x == 2.0f);
    REQUIRE(r.y == 3.5f);
}

TEST_CASE("Vector2 multiply by zero", "[vector]") {
    Vector2i v(42, 99);
    Vector2i r = v * 0;
    REQUIRE(r.x == 0);
    REQUIRE(r.y == 0);
}
