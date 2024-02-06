#include "help/about/dependencies.hpp"
#include <catch.hpp>

TEST_CASE("About Dependencies", "[about]")
{
    auto deps = about::dependencies();
    CHECK(deps.length() == 10);
    for (const auto &dep : deps)
        CHECK(dep.licenseText.size() != 0);
};

int main(int argc, char *argv[])
{
    return Catch::Session().run(argc, argv);
}
