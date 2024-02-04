#include "config.hpp"
#include <catch.hpp>

TEST_CASE("Terminal, ls", "[term][cli]")
{
    auto path = term_path();
    QProcess term;
    term.start(path, {"ls"});
    wait_return(term, 0);
    CHECK(term.readAllStandardOutput().length() > 0);
}
