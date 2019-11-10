#include "csxp/atom_fmt.h"
#include "csxp/env.h"
#include "csxp/lib/detail/core.h"
#include "csxp/lib/detail/fn.h"
#include "csxp/lib/detail/util.h"
#include "csxp/lib/lib.h"
#include "csxp/logging.h"

#include <map>

#define LOGGER() (logging::get("lib/detail/fn"))

using namespace std::literals;

namespace csxp::lib::detail::fn {

struct CallableFn : public Callable
{
    // todo: move?
    CallableFn(Scope scope,
            std::shared_ptr<Vec> binding,
            std::shared_ptr<Vec> body) :
        scope(scope),
        binding(binding),
        body(body)
    {}

    patom operator()(csxp::Env* env, AtomIterator* args)
    {
        // eval our args before creating the stack frame
        // and doing the destructuring
        std::vector<patom> evaledArgs;
        for (std::size_t i = 0; i < binding->items.size(); ++i) {
            if (!args->next()) {
                throw new LibError("missing required argument");
            }

            auto val = env->eval(args->value());
            evaledArgs.push_back(val);
        }

        if (args->next()) {
            // todo: fn name in message
            throw new LibError("too many arguments for fn");
        }

        EnvFrame es{env, scope};
        for (std::size_t i = 0; i < binding->items.size(); ++i) {
            env->destructure(binding->items[i], evaledArgs[i]);
        }

        // pass body on to do
        auto res = core::do_(env, body->iterator().get());
        return res;
    }

    Scope scope;
    std::shared_ptr<Vec> binding;
    std::shared_ptr<Vec> body;
};

// this is probably oversimplistic
static void resolveLocal(csxp::Env* env, patom a,
        Scope& resolved)
{
    // try to resolve them, only in lexical scope
    if (auto sym = get_if<SymName>(a)) {
        if (auto val = env->lexResolve(sym.get())) {
            resolved[sym->name] = val;
        }
    } else if (auto seq = get_if<Seq>(a)) {
        auto it = seq->iterator();
        while (it->next()) {
            resolveLocal(env, it->value(), resolved);
        }
    }
}

std::shared_ptr<Callable> makefn(csxp::Env* env,
        std::shared_ptr<Vec> binding, AtomIterator* bodyit)
{
    // TODO: metadata

    // todo: verify binding!
    // data.ValidBinding(binding);

    Scope scope;
    auto body = std::make_shared<Vec>();
    while (bodyit->next()) {
        resolveLocal(env, bodyit->value(), scope);
        body->items.emplace_back(bodyit->value());
    }

    // todo: include fn info, like METADATA, for call stack?!?!!!1
    return std::make_shared<CallableFn>(scope, binding, body);
}

patom fn(csxp::Env* env, AtomIterator* args)
{
    // TODO: metadata
    auto binding = util::arg_next<Vec>(args, 0, "core/fn"sv);
    return std::make_shared<atom>(makefn(env, binding, args));
}

patom defn(csxp::Env* env, AtomIterator* args)
{
    auto name = util::arg_next(args, 0, "core/defn"sv);

    auto f = fn(env, args);

    auto defargs = std::make_shared<Vec>();
    defargs->items.emplace_back(name);
    defargs->items.emplace_back(f);

    return core::def(env, defargs->iterator().get());
}

} // namespace lib::detail::fn
