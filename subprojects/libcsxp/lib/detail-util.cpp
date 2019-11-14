#include "csxp/env.h"
#include "csxp/lib/detail/util.h"
#include "csxp/lib/lib.h"
#include "rw/logging.h"

#define LOGGER() (rw::logging::get("lib/detail/util"))

using namespace std::literals;

namespace csxp::lib::detail::util {

void check_no_args(AtomIterator* args, std::string_view errFn)
{
    if (args->next()) {
        throw lib::LibError(fmt::format("too many args for {}"sv, errFn));
    }
}

patom arg_next(AtomIterator* args, int errN, std::string_view errFn)
{
    if (!args->next()) {
        throw lib::LibError(fmt::format("wrong number of args ({}) for {}"sv,
                errN, errFn));
    }

    return args->value();
}

patom arg_next(csxp::Env* env, AtomIterator* args, int errN, std::string_view errFn)
{
    return env->eval(arg_next(args, errN, errFn));
}

} // namespace csxp::lib::detail::util
