#ifndef CSXP_LIB_DETAIL_MATH_H
#define CSXP_LIB_DETAIL_MATH_H

#include "csxp/atom.h"

namespace csxp {
struct Env;

namespace lib::detail::math {

patom add(Env* env, AtomIterator* args);
patom sub(Env* env, AtomIterator* args);
patom mul(Env* env, AtomIterator* args);
patom inc(Env* env, AtomIterator* args);

patom lt(Env* env, AtomIterator* args);
patom lteq(Env* env, AtomIterator* args);
patom gt(Env* env, AtomIterator* args);
patom gteq(Env* env, AtomIterator* args);

} // namespace lib::detail::math
} // namespace csxp

#endif // CSXP_LIB_DETAIL_MATH_H
