#ifndef CSXP_RUN_HELPERS_H
#define CSXP_RUN_HELPERS_H

#include "csxp/atom.h"

csxp::patom runExp(csxp::patom exp);

csxp::patom parseString(std::string_view str);
csxp::patom runString(std::string_view str);

struct cloexptest
{
    std::string name;
    csxp::patom exp;
};

csxp::patom testExpTrue(const cloexptest& test);

struct clostringtest
{
    std::string name;
    std::string code;
};

csxp::patom testStringTrue(const clostringtest& test);
csxp::patom testStringsTrue(const std::vector<clostringtest>& tests);

void testStringLibError(const clostringtest& test);
void testStringsLibError(const std::vector<clostringtest>& tests);

namespace builder {

template <typename... AtomTs>
csxp::patom form(std::string_view fn, AtomTs... args)
{
    return csxp::List::make_atom({csxp::SymName::make_atom(fn),
            args...});
}

template <typename... AtomTs>
csxp::patom vec(AtomTs... args)
{
    return csxp::Vec::make_atom({args...});
}

template <typename... AtomTs>
csxp::patom let(std::vector<csxp::patom>&& args, AtomTs... body)
{
    return csxp::List::make_atom({csxp::SymName::make_atom("let"),
            csxp::Vec::make_atom(args),
            body...});
}

inline std::vector<csxp::patom> binding(std::string_view name, csxp::patom val)
{
    return {csxp::SymName::make_atom(name), val};
}

template <typename Fn>
std::vector<csxp::patom> binding(std::string_view name, Fn fn)
{
    return binding(name, csxp::make_callable(fn));
}

} // namespace builder

#endif // CSXP_RUN_HELPERS_H
