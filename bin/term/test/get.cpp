#include "config.hpp"
#include <catch.hpp>

TEST_CASE("Terminal, get", "[term][cli]")
{
    auto path = term_path();
    SECTION("valid pepp source")
    {
        QProcess term;
        term.start(path, {"get", "--ch", "05", "--fig", "27"});
        wait_return(term, 0);
        CHECK(term.readAllStandardOutput().length() > 0);
    }
    SECTION("invalid pepp source")
    {
        QProcess term;
        term.start(path, {"get", "--ch", "00", "--fig", "x0"});
        wait_return(term, 1);
        auto err = term.readAllStandardError();
        err.replace("\r", "");
        CHECK(err == "Figure 00.x0 does not exist.\n");
    }
    SECTION("valid pepp source, invalid variant")
    {
        QProcess term;
        term.start(path, {"get", "--ch", "05", "--fig", "27", "--type", "not"});
        wait_return(term, 2);
        auto err = term.readAllStandardError();
        err.replace("\r", "");
        CHECK(err == "Figure 05.27 does not contain a \"not\" variant.\n");
    }
}
