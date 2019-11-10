#ifndef CSXP_LIB_DETAIL_CORE_H
#define CSXP_LIB_DETAIL_CORE_H

#include "csxp/atom.h"

namespace csxp {
struct Env;

namespace lib::detail::core {

patom ns(Env* env, AtomIterator* args);
patom require(Env* env, AtomIterator* args);
patom if_(Env* env, AtomIterator* args);
patom when(Env* env, AtomIterator* args);
patom quote(Env* env, AtomIterator* args);
patom def(Env* env, AtomIterator* args);
patom do_(Env* env, AtomIterator* args);
patom let(Env* env, AtomIterator* args);
patom assert_(Env* env, AtomIterator* args);
patom count(Env* env, AtomIterator* args);
patom comment(Env* env, AtomIterator* args);
patom some(Env* env, AtomIterator* args);
patom seq(Env* env, AtomIterator* args);
patom first(Env* env, AtomIterator* args);
patom rest(Env* env, AtomIterator* args);
patom next(Env* env, AtomIterator* args);
patom identity(Env* env, AtomIterator* args);
patom take(Env* env, AtomIterator* args);
patom iterate(Env* env, AtomIterator* args);
patom reduce(Env* env, AtomIterator* args);
patom repeat(Env* env, AtomIterator* args);
patom repeatedly(Env* env, AtomIterator* args);

patom conj(Env* env, AtomIterator* args);
patom assoc(Env* env, AtomIterator* args);

} // namespace csxp::lib::detail::core
} // namespace csxp

#endif // CSXP_LIB_DETAIL_CORE_H
