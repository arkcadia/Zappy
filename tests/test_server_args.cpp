#include <catch2/catch_test_macros.hpp>
#include "../src/server/Server.hpp"

TEST_CASE("Server parses all required flags", "[server]") {
    std::vector<std::string> args = {
        "-p", "4242", "-x", "10", "-y", "10",
        "-n", "team1", "team2", "-c", "3"
    };
    Server s(args);
    REQUIRE(s.done == true);
}

TEST_CASE("Server parses with frequency flag", "[server]") {
    std::vector<std::string> args = {
        "-p", "4242", "-x", "10", "-y", "10",
        "-n", "team1", "-c", "3", "-f", "50"
    };
    Server s(args);
    REQUIRE(s.done == true);
}

TEST_CASE("Server throws on missing required flag", "[server]") {
    // Missing -c
    std::vector<std::string> args = {
        "-p", "4242", "-x", "10", "-y", "10",
        "-n", "team1"
    };
    REQUIRE_THROWS_AS(Server(args), std::invalid_argument);
}

TEST_CASE("Server throws on duplicate flag", "[server]") {
    std::vector<std::string> args = {
        "-p", "4242", "-p", "5555",
        "-x", "10", "-y", "10",
        "-n", "team1", "-c", "3"
    };
    REQUIRE_THROWS_AS(Server(args), std::invalid_argument);
}

TEST_CASE("Server throws on unknown flag", "[server]") {
    std::vector<std::string> args = {
        "-p", "4242", "-x", "10", "-y", "10",
        "-n", "team1", "-c", "3", "--unknown"
    };
    REQUIRE_THROWS_AS(Server(args), std::invalid_argument);
}

TEST_CASE("Server help flag does not throw", "[server]") {
    std::vector<std::string> args = {"-h"};
    REQUIRE_NOTHROW(Server(args));
}

TEST_CASE("Server long flags work", "[server]") {
    std::vector<std::string> args = {
        "--port", "4242", "--width", "10", "--height", "10",
        "--names", "team1", "team2", "--clients", "3", "--freq", "200"
    };
    Server s(args);
    REQUIRE(s.done == true);
}
