#include "csxp/env.h"
#include "csxp/lib/detail/math.h"
#include "csxp/lib/detail/util.h"
#include "csxp/lib/lib.h"

using namespace std::literals;

namespace csxp::lib::detail::math {

patom add(csxp::Env* env, AtomIterator* args)
{
    int sum = 0;

    if (!args->next()) {
        return Num::make_atom(0);
    }

    int idx = 0;
    do {
        auto num = util::arg_curr<Num>(env, args, idx, "core/+"sv);
        sum += num->val;
        idx++;
    } while (args->next());

    return Num::make_atom(sum);
}

patom sub(csxp::Env* env, AtomIterator* args)
{
    auto num = util::arg_next<Num>(env, args, 0, "core/-"sv);

    int sum = 0;
    // if we don't have a next arg, just negate the arg
    if (!args->next()) {
        return Num::make_atom(-num->val);
    } else {
        sum = num->val;
    }

    int idx = 1;
    do {
        num = util::arg_curr<Num>(env, args, idx, "core/-"sv);
        sum -= num->val;
        idx++;
    } while (args->next());

    return Num::make_atom(sum);
}

patom mul(csxp::Env* env, AtomIterator* args)
{
    int total = 1;

    int idx = 0;
    while (args->next()) {
        auto num = util::arg_curr<Num>(env, args, idx, "core/*"sv);
        total *= num->val;
        idx++;
    }

    return Num::make_atom(total);
}

patom inc(csxp::Env* env, AtomIterator* args)
{
    int total = 0;

    auto num = util::arg_next<Num>(env, args, 0, "core/inc"sv);
    total = num->val + 1;

    util::check_no_args(args, "core/inc"sv);

    return Num::make_atom(total);
}

patom lt(csxp::Env* env, AtomIterator* args)
{
    // don't throw if there's only one arg
    auto a = util::arg_next(env, args, 0, "core/<"sv);
    if (auto num1 = get_if<Num>(a)) {
        if (args->next()) {
            auto num2 = util::arg_curr<Num>(env, args, 1, "core/<"sv);
            if (num1->val < num2->val) {
                return True;
            } else {
                return False;
            }
        }
    }
    return True;
}

patom lteq(csxp::Env* env, AtomIterator* args)
{
    // don't throw if there's only one arg
    auto a = util::arg_next(env, args, 0, "core/<="sv);
    if (auto num1 = get_if<Num>(a)) {
        if (args->next()) {
            auto num2 = util::arg_curr<Num>(env, args, 1, "core/<="sv);
            if (num1->val <= num2->val) {
                return True;
            } else {
                return False;
            }
        }
    }
    return True;
}

patom gt(csxp::Env* env, AtomIterator* args)
{
    // don't throw if there's only one arg
    auto a = util::arg_next(env, args, 0, "core/>"sv);
    if (auto num1 = get_if<Num>(a)) {
        if (args->next()) {
            auto num2 = util::arg_curr<Num>(env, args, 1, "core/>"sv);
            if (num1->val > num2->val) {
                return True;
            } else {
                return False;
            }
        }
    }
    return True;
}

patom gteq(csxp::Env* env, AtomIterator* args)
{
    // don't throw if there's only one arg
    auto a = util::arg_next(env, args, 0, "core/>="sv);
    if (auto num1 = get_if<Num>(a)) {
        if (args->next()) {
            auto num2 = util::arg_curr<Num>(env, args, 1, "core/>="sv);
            if (num1->val >= num2->val) {
                return True;
            } else {
                return False;
            }
        }
    }
    return True;
}

} // namespace csxp::lib::detail::math
