#include "config.hpp"
#include <catch.hpp>

TEST_CASE("Terminal, ls", "[term][cli]")
{
    auto path = term_path();
    QProcess term;
    term.start(path, {"ls"});
    REQUIRE(term.waitForStarted());
    REQUIRE(term.waitForFinished());
    CHECK(term.exitCode() == 0);
    CHECK(term.readAllStandardOutput().length() > 0);
}

int main(int argc, char *argv[])
{
    return Catch::Session().run(argc, argv);
}
