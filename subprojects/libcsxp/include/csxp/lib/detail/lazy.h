#ifndef CSXP_LIB_DETAIL_LAZY_H
#define CSXP_LIB_DETAIL_LAZY_H

#include "csxp/atom.h"

namespace csxp {
struct Env;

namespace lib::detail::lazy {

patom cons(Env* env, AtomIterator* args);
patom lazy_seq(Env* env, AtomIterator* args);

} // namespace lib::detail::lazy
} // namespace csxp

#endif // CSXP_LIB_DETAIL_LAZY_H
