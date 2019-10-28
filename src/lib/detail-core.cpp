#include "env.h"
#include "lib/detail-core.h"
#include "lib/lib.h"
#include "logging.h"

#define LOGGER() (logging::get("lib/detail-core"))

namespace lib::detail::core {

atom::patom if_(Env* env, atom::AtomIterator* args)
{
    if (!args->next()) {
        throw lib::LibError("if missing predicate");
    }

    // evaluate the test
    auto testRes = env->eval(args->value());

    if (!args->next()) {
        throw lib::LibError("if missing true clause");
    }

    // find true clause
    auto truePart = args->value();

    // if the test is true, return args.Val.Key
    if (atom::truthy(testRes)) {
        return env->eval(truePart);
    } else {
        // find false clause
        if (args->next()) {
            auto falsePart = args->value();
            return env->eval(falsePart);
        } else {
            return atom::Nil;
        }
    }
}

atom::patom quote(Env* env, atom::AtomIterator* args)
{
    if (args->next()) {
        auto val = args->value();
        if (!atom::is_nil(val)) {
            return val;
        }
    }

    return atom::Nil;
}

atom::patom def(Env* env, atom::AtomIterator* args)
{
    if (!args->next()) {
        throw lib::LibError("unexpected nil args for def");
    }

    auto val = args->value();
    if (auto sym = std::get_if<std::shared_ptr<atom::SymName>>(&val->p)) {
        // advance to next arg
        if (args->next()) {
            auto tval = env->eval(args->value());
            env->setInternal(sym->get(), tval);
            return std::make_shared<atom::atom>(*sym);
        } else {
            throw lib::LibError("def must have at least two args");
        }
    } else {
        throw lib::LibError("expected def arg to be a symbol");
    }
}

atom::patom do_(Env* env, atom::AtomIterator* args)
{
    auto res = atom::Nil;
    while (args->next()) {
        res = env->eval(args->value());
    }

    return res;
}

atom::patom let(Env* env, atom::AtomIterator* args)
{
    if (!args->next()) {
        throw lib::LibError("let requires an args");
    }

    // TODO: metadata
    auto val = args->value();
    // todo: can nest get_if and cast in one if? e.g., combine to a get_vec func?
    if (auto seq = std::get_if<std::shared_ptr<atom::Seq>>(&val->p)) {
        if (auto vec = std::dynamic_pointer_cast<atom::Vec>(*seq)) {
            if (vec->items.size()%2) {
                throw lib::LibError("let requires even number of args");
            }

            EnvScope es{env};

            for (std::size_t i = 0; i < vec->items.size(); i += 2) {
                auto binding = vec->items[i];
                // todo: verify binding!
                // data.ValidBinding(binding);

                auto value = vec->items[i+1];
                value = env->eval(value);

                env->destructure(binding, value);
            }

            // pass body on to do
            auto res = do_(env, args);
            return res;
        }
    }

    throw lib::LibError("expected let arg to be a vec of args");
}

atom::patom assert_(Env* env, atom::AtomIterator* args)
{
    // Evaluates expr and throws an exception if it does not evaluate to
    // logical true.
    /*
      ([x]
         (when *assert*
           `(when-not ~x
              (throw (new AssertionError (str "Assert failed: " (pr-str '~x)))))))
      ([x message]
         (when *assert*
           `(when-not ~x
              (throw (new AssertionError (str "Assert failed: " ~message "\n" (pr-str '~x))))))))
    */

    if (!args->next()) {
        throw lib::LibError("assert requires an arg");
    }

    auto x = args->value();
    auto value = env->eval(x);
    if (!atom::truthy(value)) {
        throw lib::LibError("assert failed");
        /*
        todo: we should format as follows when we can fmt::format a variant_type
        // do we have a message?
        if (args->next()) {
            throw lib::LibError(fmt.Sprintf("assert failed: {}; {}",
                args->value().String(), x.String()))
        } else {
            // no message
            throw lib::LibError(fmt::format("assert failed: {}", x.String()))
        }
        */
    }

    return atom::Nil;
}

} // namespace lib::detail::core
