#include "csxp/env.h"
#include "csxp/atom_fmt.h"
#include "csxp/lib/detail/op.h"
#include "csxp/lib/detail/util.h"
#include "csxp/lib/lib.h"
#include "csxp/logging.h"

#define LOGGER() (logging::get("lib/detail/op"))

using namespace std::literals;

namespace csxp::lib::detail::op {

csxp::patom eq(csxp::Env* env, AtomIterator* args)
{
    auto a = util::arg_next(env, args, 0, "core/="sv);

    // todo: handle single arg
    util::arg_next(args, 1, "core/="sv);
    do {
        auto b = env->eval(args->value());

        if (*a != *b) {
            return False;
        }
    } while (args->next());

    return True;
}

csxp::patom neq(csxp::Env* env, AtomIterator* args)
{
    auto a = util::arg_next(env, args, 0, "core/not="sv);

    // todo: handle single arg
    util::arg_next(args, 1, "core/not="sv);
    do {
        auto b = env->eval(args->value());

        if (*a == *b) {
            return False;
        }
    } while (args->next());

    return True;
}

csxp::patom not_(csxp::Env* env, AtomIterator* args)
{
    auto val = util::arg_next(env, args, 0, "core/not"sv);
    if (!truthy(val)) {
        return True;
    }

    return False;
}

csxp::patom and_(csxp::Env* env, AtomIterator* args)
{
    if (!args->next()) {
        return True;
    }

    csxp::patom res;
    do {
        res = env->eval(args->value());
        if (!truthy(res)) {
            break;
        }
    } while (args->next());

    return res;
}

csxp::patom or_(csxp::Env* env, AtomIterator* args)
{
    if (!args->next()) {
        return Nil;
    }

    csxp::patom res;
    do {
        res = env->eval(args->value());
        if (truthy(res)) {
            break;
        }
    } while (args->next());

    return res;
}

} // namespace csxp::lib::detail::op
