#ifndef CSXP_LIB_DETAIL_UTIL_H
#define CSXP_LIB_DETAIL_UTIL_H

#include "csxp/atom.h"
#include "csxp/env.h"

#include <memory>
#include <variant>

namespace csxp {
namespace lib {
struct LibError;
} // namespace atom

// todo: create a "context stack" kinda class, to be passed to calls.
// if !null, push "context" to stack; this will contain the func name,
// which will allow formatting error messages and stack traces!

namespace lib::detail::util {

// throws if there are any more args
void check_no_args(AtomIterator* args, std::string_view errFn);

// gets the next item from args; throws message about wrong # of args (where
// errN is the number of args the func was given. E.g., if calling to get first
// arg, and it's not present, pass 0 to have the message say 0 args were given.
patom arg_next(AtomIterator* args, int errN,
        std::string_view errFn);

// gets and evals the next item from args; throws message about wrong # of args.
// see arg_next without env arg for details.
patom arg_next(Env* env, AtomIterator* args, int errN,
        std::string_view errFn);

// gets a typed next item from args; see default specialization arg_next
template <typename T>
std::shared_ptr<T> arg_next(AtomIterator* args, int errN,
        std::string_view errFn)
{
    auto val = arg_next(args, errN, errFn);
    if (val) {
        if (auto p = get_if<T>(val)) {
            return p;
        }
    }

    // todo: make 'type' the kind of thing; sequence, vec, num, str, sym, etc.
    throw LibError(fmt::format("expected {} arg {} to be type",
                errFn, errN+1));
}

// gets a typed next item from args; see default specialization arg_next
template <typename T>
std::shared_ptr<T> arg_next(Env* env, AtomIterator* args, int errN,
        std::string_view errFn)
{
    auto val = arg_next(env, args, errN, errFn);
    if (val) {
        if (auto p = get_if<T>(val)) {
            return p;
        }
    }

    // todo: make 'type' the kind of thing; sequence, num, etc.
    throw lib::LibError(fmt::format("expected {} arg {} to be type",
                errFn, errN+1));
}

// gets the current typed item from args, throws if not correct type
template <typename T>
std::shared_ptr<T> arg_curr(AtomIterator* args, int errN,
        std::string_view errFn)
{
    auto val = args->value();
    if (auto p = get_if<T>(val)) {
        return p;
    }

    // todo: make 'type' the kind of thing; sequence, num, etc.
    throw lib::LibError(fmt::format("expected {} arg {} to be type",
                errFn, errN+1));
}

// gets and evals the current typed item from args, throws if not correct type
template <typename T>
std::shared_ptr<T> arg_curr(Env* env, AtomIterator* args, int errN,
        std::string_view errFn)
{
    auto val = env->eval(args->value());
    if (auto p = get_if<T>(val)) {
        return p;
    }

    // todo: make 'type' the kind of thing; sequence, num, etc.
    throw lib::LibError(fmt::format("expected {} arg {} to be type",
                errFn, errN+1));
}

} // namespace lib::detail::util
} // namespace csxp

#endif // CSXP_LIB_DETAIL_UTIL_H
