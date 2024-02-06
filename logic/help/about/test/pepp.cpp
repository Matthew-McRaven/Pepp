#include "help/about/pepp.hpp"
#include <catch.hpp>

TEST_CASE("About Pepp", "[about]")
{
    CHECK_FALSE(about::projectRepoURL().size() == 0);
    CHECK_FALSE(about::maintainers().size() == 0);
    CHECK_FALSE(about::contributors().size() == 0);
    CHECK_FALSE(about::licenseFull().size() == 0);
    CHECK_FALSE(about::licenseNotice().size() == 0);
    CHECK(about::versionString().size() > 1);
}
int main(int argc, char *argv[])
{
    return Catch::Session().run(argc, argv);
}
