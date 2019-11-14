#include "csxp/env.h"
#include "csxp/lib/detail/core.h"
#include "csxp/lib/detail/env.h"
#include "csxp/lib/detail/fn.h"
#include "csxp/lib/detail/lazy.h"
#include "csxp/lib/detail/math.h"
#include "csxp/lib/detail/op.h"

using namespace std::literals;

namespace csxp::lib {

void addCore(Env* env)
{
    env->setInternal("ns"sv, make_callable(detail::core::ns));
    env->setInternal("require"sv, make_callable(detail::core::require));

    env->setInternal("if"sv, make_callable(detail::core::if_));
    env->setInternal("when"sv, make_callable(detail::core::when));
    env->setInternal("quote"sv, make_callable(detail::core::quote));
    env->setInternal("def"sv, make_callable(detail::core::def));
    env->setInternal("do"sv, make_callable(detail::core::do_));
    env->setInternal("let"sv, make_callable(detail::core::let));
    env->setInternal("assert"sv, make_callable(detail::core::assert_));
    env->setInternal("count"sv, make_callable(detail::core::count));
    env->setInternal("comment"sv, make_callable(detail::core::comment));
    env->setInternal("some"sv, make_callable(detail::core::some));
    env->setInternal("seq"sv, make_callable(detail::core::seq));
    env->setInternal("first"sv, make_callable(detail::core::first));
    env->setInternal("rest"sv, make_callable(detail::core::rest));
    env->setInternal("next"sv, make_callable(detail::core::next));
    env->setInternal("identity"sv, make_callable(detail::core::identity));
    env->setInternal("take"sv, make_callable(detail::core::take));
    env->setInternal("iterate"sv, make_callable(detail::core::iterate));
    env->setInternal("reduce"sv, make_callable(detail::core::reduce));
    env->setInternal("repeatedly"sv, make_callable(detail::core::repeatedly));

    env->setInternal("conj"sv, make_callable(detail::core::conj));
    env->setInternal("assoc"sv, make_callable(detail::core::assoc));

    env->setInternal("fn", make_callable(detail::fn::fn));
    env->setInternal("defn", make_callable(detail::fn::defn));

    env->setInternal("cons"sv, make_callable(detail::lazy::cons));
    env->setInternal("lazy-seq"sv, make_callable(detail::lazy::lazy_seq));
    env->setInternal("repeat"sv, make_callable(detail::lazy::repeat));

    env->setInternal("="sv, make_callable(detail::op::eq));
    env->setInternal("not="sv, make_callable(detail::op::neq));
    env->setInternal("not"sv, make_callable(detail::op::not_));
    env->setInternal("and"sv, make_callable(detail::op::and_));
    env->setInternal("or"sv, make_callable(detail::op::or_));
}

void addEnv(Env* env)
{
    env->setInternal("load-file"sv, make_callable(detail::env::load_file));
}

void addMath(Env* env)
{
    env->setInternal("+"sv, make_callable(detail::math::add));
    env->setInternal("-"sv, make_callable(detail::math::sub));
    env->setInternal("*"sv, make_callable(detail::math::mul));
    env->setInternal("inc"sv, make_callable(detail::math::inc));

    env->setInternal("<"sv, make_callable(detail::math::lt));
    env->setInternal("<="sv, make_callable(detail::math::lteq));
    env->setInternal(">"sv, make_callable(detail::math::gt));
    env->setInternal(">="sv, make_callable(detail::math::gteq));
}

void addLib(Env* env, std::string_view name)
{
    auto args = std::make_shared<Vec>(
            std::vector<patom>{
            Str::make_atom(name)});
    auto it = args->iterator();
    detail::env::load_file(env, it.get());
}

} // namespace csxp::lib
