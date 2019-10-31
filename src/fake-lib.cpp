#include "env.h"
#include "lib/detail-core.h"
#include "logging.h"

#include <string_view>

using namespace std::literals;

atom::patom dummy(Env* env, atom::AtomIterator* args)
{
    // todo
    return atom::Nil;
}

void init_rwte_util(Env* env, std::string_view nsname)
{
    auto module = env->createModule(nsname);
    module->addMembers({{"truecolor"sv, atom::make_callable(dummy)}});
}

void init_rwte_logging(Env* env, std::string_view nsname)
{
    auto module = env->createModule(nsname);
    module->addMembers({{"set-level"sv, atom::make_callable(dummy)},
            {"get"sv, atom::make_callable(dummy)},
            {"trace"sv, atom::make_callable(dummy)},
            {"debug"sv, atom::make_callable(dummy)},
            {"info"sv, atom::make_callable(dummy)},
            {"warn"sv, atom::make_callable(dummy)},
            {"error"sv, atom::make_callable(dummy)},
            {"fatal"sv, atom::make_callable(dummy)}});
}

void init_rwte_window(Env* env, std::string_view nsname)
{
    auto module = env->createModule(nsname);
    module->addMembers({{"bind-event"sv, atom::make_callable(dummy)},
            {"paste-clip"sv, atom::make_callable(dummy)},
            {"paste-sel"sv, atom::make_callable(dummy)}});
}

void init_rwte_term(Env* env, std::string_view nsname)
{
    auto module = env->createModule(nsname);
    module->addMembers({{"copy-clip"sv, atom::make_callable(dummy)},
            {"send"sv, atom::make_callable(dummy)},
            {"mode?"sv, atom::make_callable(dummy)}});
}

void init_empty_mod(Env* env, std::string_view nsname)
{
    // only used for keywords
    env->createModule(nsname);
}

void register_rwte(Env* env)
{
    env->registerModule("rwte.util"sv, init_rwte_util);
    env->registerModule("rwte.logging"sv, init_rwte_logging);
    env->registerModule("rwte.window"sv, init_rwte_window);
    env->registerModule("rwte.term"sv, init_rwte_term);

    // empty mods (for keywords)
    env->registerModule("rwte.window.keys"sv, init_empty_mod);
    env->registerModule("rwte.term.modes"sv, init_empty_mod);
    // env->setInternal("assert"sv, atom::make_callable(detail::core::assert_));
    // env->setInternal("require"sv, atom::make_callable(detail::core::require));
}
