#ifndef CSXP_LIB_DETAIL_FN_H
#define CSXP_LIB_DETAIL_FN_H

#include "csxp/atom.h"

#include <memory>

namespace csxp {
struct Env;

namespace lib::detail::fn {

patom fn(Env* env, AtomIterator* args);
patom defn(Env* env, AtomIterator* args);

// for convenience...
std::shared_ptr<Callable> makefn(Env* env,
        std::shared_ptr<Vec> binding, AtomIterator* body);

} // namespace lib::detail::fn
} // namespace csxp

#endif // CSXP_LIB_DETAIL_FN_H
