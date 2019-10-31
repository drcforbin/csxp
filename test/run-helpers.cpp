#include "atom-fmt.h"
#include "catch.hpp"
#include "env.h"
#include "lib/lib.h"
#include "parser.h"
#include "run-helpers.h"
#include "tokenizer.h"

atom::patom runString(std::string_view str)
{
    // set up basic env
    auto env = createEnv();
    lib::addCore(env.get());
    lib::addMath(env.get());

    auto tz = createTokenizer(str, "internal-test");
    auto p = createParser(tz);

    atom::patom res = atom::Nil;
    while (p->next()) {
        res = env->eval(p->value());
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
