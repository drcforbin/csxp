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

    // todo: handle single arg
    if (!args->next()) {
        throw lib::LibError("= requires at least two args");
    }

    do {
        auto b = env->eval(args->value());

        if (*a != *b) {
            return atom::False;
        }
    } while (args->next());

    return atom::True;
}

atom::patom neq(Env* env, atom::AtomIterator* args)
{
    if (!args->next()) {
        throw lib::LibError("not= requires two args");
    }

    // todo: handle single arg
    auto a = env->eval(args->value());

    if (!args->next()) {
        throw lib::LibError("not= requires at least two args");
    }

    do {
        auto b = env->eval(args->value());

        if (*a == *b) {
            return atom::False;
        }
    } while (args->next());

    return atom::True;
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

atom::patom and_(Env* env, atom::AtomIterator* args)
{
    if (!args->next()) {
        return atom::True;
    }

    atom::patom res;
    do {
        res = env->eval(args->value());
        if (!atom::truthy(res)) {
            break;
        }
    } while (args->next());

    return res;
}

atom::patom or_(Env* env, atom::AtomIterator* args)
{
    if (!args->next()) {
        return atom::Nil;
    }

    atom::patom res;
    do {
        res = env->eval(args->value());
        if (atom::truthy(res)) {
            break;
        }
    } while (args->next());

    return res;
}

} // namespace lib::detail::op
