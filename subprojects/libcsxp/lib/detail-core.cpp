#include "csxp/atom_fmt.h"
#include "csxp/env.h"
#include "csxp/lib/detail/core.h"
#include "csxp/lib/detail/util.h"
#include "csxp/lib/lib.h"
#include "rw/logging.h"

#define LOGGER() (rw::logging::get("lib/detail/core"))

using namespace std::literals;

namespace csxp::lib::detail::core {

patom ns(csxp::Env* env, AtomIterator* args)
{
    // note: no eval
    auto sym = util::arg_next<SymName>(args, 0, "core/ns"sv);
    env->currNs(sym->name);

    return Nil;
}

patom require(csxp::Env* env, AtomIterator* args)
{
    util::arg_next(args, 0, "core/require"sv);

    int idx = 1;
    do {
        // we're expecting each req list item to be a list containing quoted
        // symbol or vector.

        auto seq = util::arg_curr<Seq>(args, idx++, "core/require"sv);
        auto it = seq->iterator();
        if (!it->next()) {
            throw lib::LibError("require list not expected to be empty");
        }

        auto sym = get_if<SymName>(it->value());
        if (sym) {
            if (sym->name != "quote") {
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
        if (auto sym = get_if<SymName>(it->value())) {
            // look for namespace in env; if not found, throw.
            if (!env->hasModule(sym->name)) {
                throw lib::LibError(fmt::format("unable to find required namespace {}", sym->name));
            }
            // alias namespace -> namespace
            env->aliasNs(sym->name, sym->name);
        } else if (auto seq = get_if<Seq>(it->value())) {
            // quoted value is a sequence

            // advance to first item in the sequence
            // will be the namespace name.
            it = seq->iterator();
            if (!it->next()) {
                throw lib::LibError("require item expected to contain namespace name");
            }

            // it should have it has one or more items, the first item will be
            // the namespace name
            sym = get_if<SymName>(it->value());
            if (sym) {
                // look for namespace in env; if not found, throw.
                if (!env->hasModule(sym->name)) {
                    throw lib::LibError(fmt::format("unable to find required namespace {}", sym->name));
                }
            } else {
                throw lib::LibError("require item expected to contain namespace name symbol");
            }

            // if we have second arg, it needs to be :as
            if (it->next()) {
                if (auto as = get_if<Keyword>(it->value())) {
                    if (as->name != ":as"sv) {
                        throw lib::LibError(fmt::format("only :as option supported in require {}", sym->name));
                    }
                } else {
                    throw lib::LibError(fmt::format("only keyword options supported in require {}", sym->name));
                }

                // since we had an :as, we need the alias as the third arg
                if (it->next()) {
                    if (auto alias = get_if<SymName>(it->value())) {
                        env->aliasNs(alias->name, sym->name);
                    } else {
                        throw lib::LibError(fmt::format("expected :as alias to be a symbol", sym->name));
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

    return Nil;
}

patom if_(csxp::Env* env, AtomIterator* args)
{
    // todo: throw if too many args

    // evaluate the test
    auto testRes = util::arg_next(env, args, 0, "core/if"sv);

    // find true clause (no eval)
    auto truePart = util::arg_next(args, 1, "core/if"sv);

    // if the test is true, return args.Val.Key
    if (truthy(testRes)) {
        return env->eval(truePart);
    } else {
        // find false clause
        if (args->next()) {
            auto falsePart = args->value();
            return env->eval(falsePart);
        } else {
            return Nil;
        }
    }
}

patom when(csxp::Env* env, AtomIterator* args)
{
    // evaluate the test
    auto testRes = util::arg_next(env, args, 0, "core/when"sv);

    // if the test is true, return eval the body
    auto res = Nil;
    if (truthy(testRes)) {
        while (args->next()) {
            res = env->eval(args->value());
        }
    }
    return res;
}

patom quote(csxp::Env* env, AtomIterator* args)
{
    if (args->next()) {
        auto val = args->value();
        if (!is_nil(val)) {
            return val;
        }
    }

    return Nil;
}

patom def(csxp::Env* env, AtomIterator* args)
{
    auto sym = util::arg_next<SymName>(args, 0, "core/def"sv);
    auto tval = util::arg_next(env, args, 1, "core/def"sv);
    env->setInternal(sym.get(), tval);
    return std::make_shared<atom>(sym);
}

patom do_(csxp::Env* env, AtomIterator* args)
{
    auto res = Nil;
    while (args->next()) {
        res = env->eval(args->value());
    }

    return res;
}

patom let(csxp::Env* env, AtomIterator* args)
{
    // TODO: metadata
    auto vec = util::arg_next<Vec>(args, 0, "core/let"sv);
    if (vec->items.size() % 2) {
        throw lib::LibError("let requires even number of args");
    }

    EnvScope es{env};

    for (std::size_t i = 0; i < vec->items.size(); i += 2) {
        auto binding = vec->items[i];
        // todo: verify binding!
        // data.ValidBinding(binding);

        auto val = env->eval(vec->items[i + 1]);
        env->destructure(binding, val);
    }

    // pass body on to do
    auto res = do_(env, args);
    return res;
}

patom assert_(csxp::Env* env, AtomIterator* args)
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

    auto x = util::arg_next(args, 0, "core/assert"sv);
    auto val = env->eval(x);
    if (!truthy(val)) {
        // do we have a message?
        if (args->next()) {
            throw lib::LibError(fmt::format("assert failed: {}; {}",
                *val, *x));
        } else {
            // no message
            throw lib::LibError(fmt::format("assert failed: {}", *x));
        }
    }

    return Nil;
}

patom count(csxp::Env* env, AtomIterator* args)
{
    auto val = util::arg_next(env, args, 0, "core/count"sv);

    patom res;
    if (auto vec = get_if<Vec>(val)) {
        res = Num::make_atom(vec->items.size());
    } else if (auto lst = get_if<List>(val)) {
        res = Num::make_atom(lst->items.size());
    } else if (auto map = get_if<Map>(val)) {
        res = Num::make_atom(map->items->size());
    } else if (auto seq = get_if<Seq>(val)) {
        int count = 0;
        auto it = seq->iterator();
        while (it->next()) {
            count ++;
        }
        res = Num::make_atom(count);
    } else {
        throw lib::LibError("count arg could not be cast to sequence");
    }

    util::check_no_args(args, "core/count"sv);
    return res;
}

patom comment(csxp::Env* env, AtomIterator* args)
{
    return Nil;
}

patom some(csxp::Env* env, AtomIterator* args)
{
    auto call = util::arg_next<Callable>(env, args, 0, "core/some"sv);
    auto seq = util::arg_next<Seq>(env, args, 1, "core/some"sv);

    auto it = seq->iterator();
    while (it->next()) {
        auto argVec = std::make_shared<Vec>(
                std::vector<patom>{it->value()});
        auto argIt = argVec->iterator();
        auto res = (*call)(env, argIt.get());
        if (truthy(res)) {
            return res;
        }
    }

    return Nil;
}

patom seq(csxp::Env* env, AtomIterator* args)
{
    auto val = util::arg_next(args, 0, "core/seq"sv);

    // note: not lazy
    std::vector<patom> lst;
    if (*val != *Nil)
    {
        if (auto seqArg = get_if<Seq>(env->eval(val))) {
            for (auto val : *seqArg) {
                lst.emplace_back(env->eval(val));
            }
        } else {
            throw lib::LibError("expected arg to be sequence");
        }
    }

    util::check_no_args(args, "core/seq"sv);

    if (!lst.empty()) {
        // todo: check make_atom for other places to std::move
        return List::make_atom(std::move(lst));
    } else {
        return Nil;
    }
}

patom first(csxp::Env* env, AtomIterator* args)
{
    auto res = seq(env, args);
    if (*res != *Nil) {
        // seq always returns Seq or nil, get ok here
        if (auto seqArg = get<Seq>(res)) {
            if (auto it = seqArg->begin(); it != seqArg->end()) {
                return *it;
            }
        }
    }

    return Nil;
}

patom rest(csxp::Env* env, AtomIterator* args)
{
    auto res = seq(env, args);

    std::vector<patom> lst;
    if (*res != *Nil) {
        // seq always returns Seq or nil, get ok here
        if (auto seqArg = get<Seq>(env->eval(args->value()))) {
            auto it = seqArg->iterator();
            it->next(); // skip first item

            while (it->next()) {
                lst.emplace_back(it->value());
            }
        }
    }

    return List::make_atom(std::move(lst));
}

patom next(csxp::Env* env, AtomIterator* args)
{
    auto res = seq(env, args);

    std::vector<patom> lst;
    if (*res != *Nil) {
        // seq always returns Seq or nil, get ok here
        if (auto seqArg = get<Seq>(env->eval(args->value()))) {
            auto it = seqArg->iterator();
            it->next(); // skip first item

            while (it->next()) {
                lst.emplace_back(it->value());
            }
        }
    }

    if (!lst.empty()) {
        return List::make_atom(std::move(lst));
    } else {
        return Nil;
    }
}

patom identity(csxp::Env* env, AtomIterator* args)
{
    auto val = util::arg_next(env, args, 0, "core/identity"sv);
    util::check_no_args(args, "core/identity"sv);

    return val;
}

patom reduce(csxp::Env* env, AtomIterator* args)
{
    auto call = util::arg_next<Callable>(env, args, 0, "core/reduce"sv);
    auto arg2 = util::arg_next(args, 1, "core/reduce"sv);

    patom res;
    std::shared_ptr<AtomIterator> it;

    if (args->next()) {
        auto arg3 = args->value();

        util::check_no_args(args, "core/reduce"sv);

        if (auto seq = get_if<Seq>(env->eval(arg3))) {
            it = seq->iterator();
            res = arg2;
        } else {
            throw lib::LibError("expected arg to be sequence");
        }
    } else {
        if (auto seq = get_if<Seq>(env->eval(arg2))) {
            it = seq->iterator();
            if (it->next()) {
                res = it->value();
            } else {
                // call w/ no args
                auto argvec = std::make_shared<Vec>();
                auto argit = argvec->iterator();
                res = (*call)(env, argit.get());
            }
        } else {
            throw lib::LibError("expected arg to be sequence");
        }
    }

    while (it->next()) {
        // call w/ last res and curr val
        auto argvec = std::make_shared<Vec>(
                std::vector<patom>{res, it->value()});
        auto argit = argvec->iterator();
        res = (*call)(env, argit.get());
    }

    return res;
}

patom take(csxp::Env* env, AtomIterator* args)
{
    // todo: transducer
    auto num = util::arg_next<Num>(env, args, 0, "core/take"sv);
    auto val = util::arg_next(env, args, 1, "core/take"sv);
    util::check_no_args(args, "core/take"sv);

    // todo: this is seriously cheating. take should be lazy!
    std::vector<patom> lst;
    if (*val != *Nil) {
        if (auto seqArg = get_if<Seq>(val)) {
            auto it = seqArg->iterator();
            // val may be negative, so we have to make sure to cast size
            while (static_cast<decltype(num->val)>(lst.size()) < num->val &&
                    it->next()) {
                lst.emplace_back(it->value());
            }
        } else {
            throw lib::LibError("expected core/take arg 1 to be type");
        }
    }

    return List::make_atom(std::move(lst));
}

patom iterate(csxp::Env* env, AtomIterator* args)
{
    auto call = util::arg_next<Callable>(env, args, 0, "core/iterate"sv);
    auto val = util::arg_next(env, args, 1, "core/iterate"sv);
    util::check_no_args(args, "core/iterate"sv);

    // todo: this is broken. iterate MUST be lazy!
    std::vector<patom> lst;
    for (int i = 0; i < 3; i++) {
        auto argVec = std::make_shared<Vec>(
                std::vector<patom>{val});
        auto argIt = argVec->iterator();
        val = (*call)(env, argIt.get());
        // really, need to yield here
        lst.emplace_back(val);
    }

    return List::make_atom(std::move(lst));
}

patom repeat(csxp::Env* env, AtomIterator* args)
{
    // todo
    return Nil;
}

patom repeatedly(csxp::Env* env, AtomIterator* args)
{
    // todo
    return Nil;
}

patom conj(csxp::Env* env, AtomIterator* args)
{
    // todo
    return Nil;
}

patom assoc(csxp::Env* env, AtomIterator* args)
{
    // todo
    return Nil;
}

} // namespace csxp::lib::detail::core
