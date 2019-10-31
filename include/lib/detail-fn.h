#ifndef LIB_DETAIL_FN_H
#define LIB_DETAIL_FN_H

#include "atom.h"

namespace lib::detail::fn {

atom::patom fn(Env* env, atom::AtomIterator* args);
atom::patom defn(Env* env, atom::AtomIterator* args);

} // namespace lib::detail::fn

#endif // LIB_DETAIL_FN_H
