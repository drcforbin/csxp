#include "env.h"
#include "lib/detail-core.h"
#include "lib/detail-fn.h"
#include "lib/detail-math.h"
#include "lib/detail-op.h"

using namespace std::literals;

namespace lib {

void addCore(Env* env)
{
    env->setInternal("if"sv, atom::make_callable(detail::core::if_));
    env->setInternal("quote"sv, atom::make_callable(detail::core::quote));
    env->setInternal("def"sv, atom::make_callable(detail::core::def));
    env->setInternal("do"sv, atom::make_callable(detail::core::do_));
    env->setInternal("let"sv, atom::make_callable(detail::core::let));
    env->setInternal("assert"sv, atom::make_callable(detail::core::assert_));

    env->setInternal("fn", atom::make_callable(detail::fn::fn));

    // todo: rename op_?
    env->setInternal("="sv, atom::make_callable(detail::op::eq));
    env->setInternal("not="sv, atom::make_callable(detail::op::neq));
    env->setInternal("not"sv, atom::make_callable(detail::op::not_));
}

void addMath(Env* env)
{
    env->setInternal("+"sv, atom::make_callable(detail::math::add));
    env->setInternal("*"sv, atom::make_callable(detail::math::mul));
    env->setInternal("inc"sv, atom::make_callable(detail::math::inc));
}

} // namespace lib
