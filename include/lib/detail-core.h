#ifndef LIB_DETAIL_CORE_H
#define LIB_DETAIL_CORE_H

namespace lib::detail::core {

atom::patom if_(Env* env, atom::AtomIterator* args);
atom::patom quote(Env* env, atom::AtomIterator* args);
atom::patom def(Env* env, atom::AtomIterator* args);
atom::patom do_(Env* env, atom::AtomIterator* args);
atom::patom let(Env* env, atom::AtomIterator* args);
atom::patom assert_(Env* env, atom::AtomIterator* args);

} // namespace lib::detail::core

#endif // LIB_DETAIL_CORE_H
