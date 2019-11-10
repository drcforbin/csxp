#include "doctest.h"
#include "csxp/env.h"
#include "csxp/lib/lib.h"
#include "csxp/reader.h"
#include "run-helpers.h"

csxp::patom runExp(csxp::patom exp)
{
    // set up basic env
    auto env = csxp::createEnv();
    csxp::lib::addCore(env.get());
    csxp::lib::addMath(env.get());

    return env->eval(exp);
}

csxp::patom runString(std::string_view str)
{
    // set up basic env
    auto env = csxp::createEnv();
    csxp::lib::addCore(env.get());
    csxp::lib::addMath(env.get());

    csxp::patom res = csxp::Nil;
    for (auto val : csxp::reader(str, "internal-test")) {
        res = env->eval(val);
    }

    return res;
}

csxp::patom parseString(std::string_view str)
{
    auto r = csxp::reader(str, "internal-test");
    auto it = r.begin();
    REQUIRE(it != r.end());
    auto res = *it++;
    REQUIRE(it == r.end());
    return res;
}

csxp::patom testExpTrue(const cloexptest& test)
{
    INFO(test.name);
    auto res = runExp(test.exp);
    REQUIRE(csxp::truthy(res));
    return res;
}

csxp::patom testStringTrue(const clostringtest& test)
{
    INFO(test.name);
    auto res = runString(test.code);
    REQUIRE(csxp::truthy(res));
    return res;
}

csxp::patom testStringsTrue(const std::vector<clostringtest>& tests)
{
    csxp::patom res;
    for (auto& test : tests) {
        res = testStringTrue(test);
    }
    return res;
}

void testStringLibError(const clostringtest& test)
{
    INFO(test.name);
    CHECK_THROWS_AS(runString(test.code), csxp::lib::LibError);
}

void testStringsLibError(const std::vector<clostringtest>& tests)
{
    for (auto& test : tests) {
        testStringLibError(test);
    }
}
