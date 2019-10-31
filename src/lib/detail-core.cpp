#include "atom-fmt.h"
#include "env.h"
#include "lib/detail-core.h"
#include "lib/lib.h"
#include "logging.h"

#define LOGGER() (logging::get("lib/detail-core"))

using namespace std::literals;

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
            if (vec->items.size() % 2) {
                throw lib::LibError("let requires even number of args");
            }

            EnvScope es{env};

            for (std::size_t i = 0; i < vec->items.size(); i += 2) {
                auto binding = vec->items[i];
                // todo: verify binding!
                // data.ValidBinding(binding);

                auto value = vec->items[i + 1];
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
        // do we have a message?
        if (args->next()) {
            throw lib::LibError(fmt::format("assert failed: {}; {}",
                *(args->value()), *x));
        } else {
            // no message
            throw lib::LibError(fmt::format("assert failed: {}", *x));
        }
    }

    return atom::Nil;
}

atom::patom require(Env* env, atom::AtomIterator* args)
{
    // get first item in req list
    if (!args->next()) {
        throw lib::LibError("require missing arg");
    }

    do {
        // we're expecting each req list item to be a list containing quoted
        // symbol or vector.

        std::shared_ptr<atom::AtomIterator> it;
        if (auto seq = std::get_if<std::shared_ptr<atom::Seq>>(&args->value()->p)) {
            it = (*seq)->iterator();
            if (!it->next()) {
                throw lib::LibError("require list not expected to be empty");
            }
        } else {
            throw lib::LibError("expected require args to be quoted list");
        }

        auto sym = std::get_if<std::shared_ptr<atom::SymName>>(&it->value()->p);
        if (sym) {
            if ((*sym)->name != "quote") {
                throw lib::LibError("expected require args to be quoted");
            }
        } else {
            throw lib::LibError("expected require args to be quoted");
        }

        // advance to next arg
        if (!it->next()) {
            throw lib::LibError("expected require item to contain a value");
        }

        // first check whether the quoted value is a symbol
        if (auto sym = std::get_if<std::shared_ptr<atom::SymName>>(&it->value()->p)) {
            LOGGER()->info("req got sym {}", **sym);
            // look for namespace in env; if not found, throw.
            if (!env->hasModule((*sym)->name)) {
                throw lib::LibError(fmt::format("unable to find required namespace {}", (*sym)->name));
            }
            // alias namespace -> namespace
            // todo: handle where
            env->aliasNs("", (*sym)->name, (*sym)->name);
        } else if (auto seq = std::get_if<std::shared_ptr<atom::Seq>>(&it->value()->p)) {
            // quoted value is a sequence

            // advance to first item in the sequence
            // will be the namespace name.
            it = (*seq)->iterator();
            if (!it->next()) {
                throw lib::LibError("require item expected to contain namespace name");
            }

            // it should have it has one or more items, the first item will be
            // the namespace name
            sym = std::get_if<std::shared_ptr<atom::SymName>>(&it->value()->p);
            if (sym) {
                // look for namespace in env; if not found, throw.
                if (!env->hasModule((*sym)->name)) {
                    throw lib::LibError(fmt::format("unable to find required namespace {}", (*sym)->name));
                }
            } else {
                throw lib::LibError("require item expected to contain namespace name symbol");
            }

            // if we have second arg, it needs to be :as
            if (it->next()) {
                if (auto as = std::get_if<std::shared_ptr<atom::Keyword>>(&it->value()->p)) {
                    if ((*as)->name != ":as"sv) {
                        throw lib::LibError(fmt::format("only :as option supported in require {}", (*sym)->name));
                    }
                } else {
                    throw lib::LibError(fmt::format("only keyword options supported in require {}", (*sym)->name));
                }

                // since we had an :as, we need the alias as the third arg
                if (it->next()) {
                    if (auto alias = std::get_if<std::shared_ptr<atom::SymName>>(&it->value()->p)) {
                        // todo: handle where
                        LOGGER()->info("req alias {} -> {}", (*alias)->name, (*sym)->name);
                        env->aliasNs("", (*alias)->name, (*sym)->name);
                    } else {
                        throw lib::LibError(fmt::format("expected :as alias to be a symbol", (*sym)->name));
                    }
                } else {
                    throw lib::LibError(fmt::format("expected an alias to follow :as"));
                }
            }
        } else {
            throw lib::LibError("expected require item to contain a symbol or sequence");
        }

        // get next item in req list
    } while (args->next());

    return atom::Nil;
}

} // namespace lib::detail::core
