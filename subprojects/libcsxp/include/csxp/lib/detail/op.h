#ifndef CSXP_LIB_DETAIL_OP_H
#define CSXP_LIB_DETAIL_OP_H

#include "csxp/atom.h"

namespace csxp {
struct Env;

namespace lib::detail::op {

patom eq(Env* env, AtomIterator* args);
patom neq(Env* env, AtomIterator* args);
patom not_(Env* env, AtomIterator* args);
patom and_(Env* env, AtomIterator* args);
patom or_(Env* env, AtomIterator* args);

} // namespace lib::detail::op
} // namespace csxp

#endif // CSXP_LIB_DETAIL_OP_H
