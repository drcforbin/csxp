#ifndef CSXP_LIB_DETAIL_ENV_H
#define CSXP_LIB_DETAIL_ENV_H

#include "csxp/atom.h"

namespace csxp {
struct Env;

// todo: rename?
namespace lib::detail::env {

patom load_file(Env* env, AtomIterator* args);

} // namespace lib::detail::fn
} // namespace csxp

#endif // CSXP_LIB_DETAIL_ENV_H
