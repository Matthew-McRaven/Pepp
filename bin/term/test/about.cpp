#include "config.hpp"
#include <catch.hpp>

TEST_CASE("Terminal, about", "[term][cli]")
{
    auto path = term_path();
    QProcess term;
    term.start(path, {"about"});
    wait_return(term, 0);
    CHECK(term.readAllStandardOutput().length() > 0);
}
