#include "env.h"
#include "lib/detail-math.h"
#include "lib/lib.h"

namespace lib::detail::math {

// todo: test (+) == 0
// todo: test (+ 4) == 4
// todo: test (+ -4) == -4
// todo: test (+ 2 -4) == -2
// todo: test (+ -4 2) == -2
// todo: test (+ -4 2 3) == 1
// todo: test (+ 2 3 -4) == 1

atom::patom add(Env* env, atom::AtomIterator* args)
{
    int sum = 0;

    if (!args->next()) {
        return atom::Num::make_atom(0);
    }

    do {
        auto valAtom = env->eval(args->value());

        if (auto num = std::get_if<std::shared_ptr<atom::Num>>(&valAtom->p)) {
            sum += (*num)->val;
        } else {
            throw lib::LibError("+ arg did not evaluate to a num");
        }
    } while (args->next());

    return atom::Num::make_atom(sum);
}

// todo: test (-) == error
// todo: test (- 4) == -4
// todo: test (- 4 3) == 1
// todo: test (- 4 2 1) == 1

atom::patom sub(Env* env, atom::AtomIterator* args)
{
    if (!args->next()) {
        throw lib::LibError(" wrong number of args passed to -");
    }

    auto valAtom = env->eval(args->value());

    int sum = 0;
    if (auto num = std::get_if<std::shared_ptr<atom::Num>>(&valAtom->p)) {
        // if we don't have a next arg, just negate the arg
        if (!args->next()) {
            return atom::Num::make_atom(-(*num)->val);
        } else {
            sum = (*num)->val;
        }
    } else {
        throw lib::LibError("- arg did not evaluate to a num");
    }

    do {
        valAtom = env->eval(args->value());
        if (auto num = std::get_if<std::shared_ptr<atom::Num>>(&valAtom->p)) {
            sum -= (*num)->val;
        } else {
            throw lib::LibError("- arg did not evaluate to a num");
        }
    } while (args->next());

    return atom::Num::make_atom(sum);
}

atom::patom mul(Env* env, atom::AtomIterator* args)
{
    int total = 0;
    bool first = true;

    // todo: is it an error if there are no args?

    while (args->next()) {
        auto valAtom = env->eval(args->value());

        if (auto num = std::get_if<std::shared_ptr<atom::Num>>(&valAtom->p)) {
            if (first) {
                total = (*num)->val;
                first = false;
            } else {
                total *= (*num)->val;
            }
        } else {
            throw lib::LibError("* arg did not evaluate to a num");
        }
    }

    return atom::Num::make_atom(total);
}

atom::patom inc(Env* env, atom::AtomIterator* args)
{
    int total = 0;

    // todo: is it an error if there are no args?
    // todo: how about too many?

    if (args->next()) {
        auto valAtom = env->eval(args->value());

        if (auto num = std::get_if<std::shared_ptr<atom::Num>>(&valAtom->p)) {
            total = (*num)->val + 1;
        } else {
            throw lib::LibError("inc arg did not evaluate to a num");
        }
    } else {
        throw lib::LibError("inc requires a num");
    }

    return atom::Num::make_atom(total);
}

} // namespace lib::detail::math
