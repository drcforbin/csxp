#include "env.h"
#include "lib/detail-core.h"
#include "lib/detail-fn.h"
#include "lib/lib.h"

#include "logging.h"
#define LOGGER() (logging::get("lib/detail-fn"))

namespace lib::detail::fn {

struct CallableFn : public atom::Callable {
    CallableFn(std::shared_ptr<atom::Vec> binding,
    	std::shared_ptr<atom::Vec> body) :
        binding(binding),
        body(body)
    { }

    atom::patom operator()(Env* env, atom::AtomIterator* args)
    {
        EnvScope es{env};

        for (auto binding : binding->items) {
            if (!args->next()) {
                throw new LibError("missing required argument");
            }

            auto value = env->eval(args->value());
            env->destructure(binding, value);
        }

        // pass body on to do
        auto res = core::do_(env, body->iterator().get());
        return res;
    }

    std::shared_ptr<atom::Vec> binding;
	std::shared_ptr<atom::Vec> body;
};

atom::patom fn(Env* env, atom::AtomIterator* args)
{
    if (!args->next()) {
        throw lib::LibError("fn requires an args");
    }

    // TODO: metadata
    auto val = args->value();
    // todo: can nest get_if and cast?
    if (auto seq = std::get_if<std::shared_ptr<atom::Seq>>(&val->p)) {
        if (auto binding = std::dynamic_pointer_cast<atom::Vec>(*seq)) {
            // todo: naming?
            // todo: verify binding!
            // data.ValidBinding(binding);

            auto body = std::make_shared<atom::Vec>();
            while (args->next()) {
                body->items.emplace_back(args->value());
            }

            return std::make_shared<atom::atom>(
                    std::make_shared<CallableFn>(binding, body));
        }
    }

    throw lib::LibError("expected fn arg to be a vec of args");
}

} // namespace lib::detail::fn
