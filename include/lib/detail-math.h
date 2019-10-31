#ifndef LIB_DETAIL_MATH_H
#define LIB_DETAIL_MATH_H

#include "atom.h"

namespace lib::detail::math {

atom::patom add(Env* env, atom::AtomIterator* args);
atom::patom sub(Env* env, atom::AtomIterator* args);
atom::patom mul(Env* env, atom::AtomIterator* args);
atom::patom inc(Env* env, atom::AtomIterator* args);

} // namespace lib::detail::math

#endif // LIB_DETAIL_MATH_H
