#include "env.h"
#include "lib/detail-op.h"
#include "lib/lib.h"
#include "logging.h"

#define LOGGER() (logging::get("lib/detail-op"))

namespace lib::detail::op {

atom::patom eq(Env* env, atom::AtomIterator* args)
{
    if (!args->next()) {
        throw lib::LibError("= requires two args");
    }

    auto a = env->eval(args->value());

    if (!args->next()) {
        throw lib::LibError("= requires two args");
    }

    auto b = env->eval(args->value());

    if (*a == *b) {
        return atom::True;
    }

    return atom::False;
}

atom::patom neq(Env* env, atom::AtomIterator* args)
{
    if (!args->next()) {
        throw lib::LibError("not= requires two args");
    }

    auto a = env->eval(args->value());

    if (!args->next()) {
        throw lib::LibError("not= requires two args");
    }

    auto b = env->eval(args->value());

    if (*a != *b) {
        return atom::True;
    }

    return atom::False;
}

atom::patom not_(Env* env, atom::AtomIterator* args)
{
    if (!args->next()) {
        throw lib::LibError("not requires an arg");
    }

    auto val = env->eval(args->value());

    if (!atom::truthy(val)) {
        return atom::True;
    }

    return atom::False;
}

} // namespace lib::detail::op
