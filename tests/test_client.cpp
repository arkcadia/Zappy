#include <catch2/catch_test_macros.hpp>

// We need to test Client::next_line and Client::send without real sockets.
// We can directly manipulate read_buf/write_buf since they're public.
#include "../src/server/Client.hpp"

TEST_CASE("Client next_line returns nullopt when no newline", "[client]") {
    Client c(-1);
    c.read_buf = "partial data";
    REQUIRE_FALSE(c.next_line().has_value());
}

TEST_CASE("Client next_line extracts line on \\n", "[client]") {
    Client c(-1);
    c.read_buf = "hello\nworld\n";
    auto line1 = c.next_line();
    REQUIRE(line1.has_value());
    REQUIRE(*line1 == "hello");
    auto line2 = c.next_line();
    REQUIRE(line2.has_value());
    REQUIRE(*line2 == "world");
}

TEST_CASE("Client next_line strips \\r\\n", "[client]") {
    Client c(-1);
    c.read_buf = "hello\r\n";
    auto line = c.next_line();
    REQUIRE(line.has_value());
    REQUIRE(*line == "hello");
}

TEST_CASE("Client next_line handles empty line", "[client]") {
    Client c(-1);
    c.read_buf = "\n";
    auto line = c.next_line();
    REQUIRE(line.has_value());
    REQUIRE(*line == "");
}

TEST_CASE("Client next_line leaves remainder in buffer", "[client]") {
    Client c(-1);
    c.read_buf = "first\nsecond";
    auto line = c.next_line();
    REQUIRE(line.has_value());
    REQUIRE(*line == "first");
    // "second" has no newline — should return nullopt
    REQUIRE_FALSE(c.next_line().has_value());
    REQUIRE(c.read_buf == "second");
}

TEST_CASE("Client send appends newline", "[client]") {
    Client c(-1);
    c.send("hello");
    REQUIRE(c.write_buf == "hello\n");
}

TEST_CASE("Client send does not double newline", "[client]") {
    Client c(-1);
    c.send("hello\n");
    REQUIRE(c.write_buf == "hello\n");
}

TEST_CASE("Client send accumulates", "[client]") {
    Client c(-1);
    c.send("first");
    c.send("second");
    REQUIRE(c.write_buf == "first\nsecond\n");
}

TEST_CASE("Client has_pending_write", "[client]") {
    Client c(-1);
    REQUIRE_FALSE(c.has_pending_write());
    c.send("data");
    REQUIRE(c.has_pending_write());
}

TEST_CASE("Client type checks", "[client]") {
    Client c(-1);
    REQUIRE_FALSE(c.is_ai());
    REQUIRE_FALSE(c.is_gui());
    c.type = ClientType::AI;
    REQUIRE(c.is_ai());
    c.type = ClientType::GUI;
    REQUIRE(c.is_gui());
}

TEST_CASE("Client multiple next_line calls drain buffer", "[client]") {
    Client c(-1);
    c.read_buf = "a\nb\nc\n";
    REQUIRE(*c.next_line() == "a");
    REQUIRE(*c.next_line() == "b");
    REQUIRE(*c.next_line() == "c");
    REQUIRE_FALSE(c.next_line().has_value());
    REQUIRE(c.read_buf.empty());
}
