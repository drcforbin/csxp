#ifndef LIB_DETAIL_OP_H
#define LIB_DETAIL_OP_H

#include "atom.h"

namespace lib::detail::op {

atom::patom eq(Env* env, atom::AtomIterator* args);
atom::patom neq(Env* env, atom::AtomIterator* args);
atom::patom not_(Env* env, atom::AtomIterator* args);
atom::patom and_(Env* env, atom::AtomIterator* args);
atom::patom or_(Env* env, atom::AtomIterator* args);

} // namespace lib::detail::op

#endif // LIB_DETAIL_OP_H
