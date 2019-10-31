#ifndef RUN_HELPERS_H
#define RUN_HELPERS_H

#include "atom.h"

#include <string>
#include <string_view>
#include <vector>

struct clostringtest
{
    std::string name;
    std::string code;
};

atom::patom runString(std::string_view str);

atom::patom testStringTrue(const clostringtest& test);
atom::patom testStringsTrue(const std::vector<clostringtest>& tests);

#endif // RUN_HELPERS_H
