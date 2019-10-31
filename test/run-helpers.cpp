#include "atom-fmt.h"
#include "catch.hpp"
#include "env.h"
#include "lib/lib.h"
#include "run-helpers.h"
#include "reader.h"

atom::patom runString(std::string_view str)
{
    // set up basic env
    auto env = createEnv();
    lib::addCore(env.get());
    lib::addMath(env.get());

    atom::patom res = atom::Nil;
    for (auto val : reader(str, "internal-test")) {
        res = val;
    }

    return res;
}

atom::patom testStringTrue(const clostringtest& test)
{
    INFO(test.name);
    auto res = runString(test.code);
    REQUIRE(atom::truthy(res));
    return res;
}

atom::patom testStringsTrue(const std::vector<clostringtest>& tests)
{
    atom::patom res;
    for (auto& test : tests) {
        res = testStringTrue(test);
    }
    return res;
}
