#include "csxp/atom_fmt.h"
#include "csxp/env.h"
#include "csxp/lib/detail/fn.h"
#include "csxp/lib/detail/lazy.h"
#include "csxp/lib/detail/util.h"
#include "csxp/lib/lib.h"
#include "rw/logging.h"

#define LOGGER() (rw::logging::get("lib/detail/lazy"))

using namespace std::literals;

namespace csxp::lib::detail::lazy {

struct Cons : public Seq, std::enable_shared_from_this<Cons>
{
    Cons(patom val, std::shared_ptr<Seq> seq) :
        val(val), seq(seq)
    {
    }

    static patom make_atom(patom val, std::shared_ptr<Seq> seq)
    {
        return std::make_shared<atom>(std::make_shared<Cons>(val, seq));
    }

    std::shared_ptr<AtomIterator> iterator() const;

    patom val;
    std::shared_ptr<Seq> seq;
};

struct ConsIterator : public AtomIterator
{
public:
    ConsIterator(std::shared_ptr<const Cons> cons) :
        val(cons->val), it(cons->seq->iterator())
    {
        // we expect these to both be non-null!
        assert(val);
        assert(it);
    }

    bool next()
    {
        if (!it) {
            return false;
        }

        if (!curr) {
            // done with val, set it free.
            curr = val;
            val.reset();
            return true;
        } else {
            if (it->next()) {
                curr = it->value();
                return true;
            } else {
                // we're done done.
                curr.reset();
                it.reset();
                return false;
            }
        }
    }

    patom value() const
    {
        return curr;
    }

private:
    patom val;
    std::shared_ptr<AtomIterator> it;

    patom curr;
};

std::shared_ptr<AtomIterator> Cons::iterator() const
{
    return std::make_shared<ConsIterator>(shared_from_this());
}

patom cons(csxp::Env* env, AtomIterator* args)
{
    auto val = util::arg_next(env, args, 0, "core/cons"sv);
    auto seqArg = util::arg_next(env, args, 1, "core/cons"sv);
    util::check_no_args(args, "core/cons"sv);

    std::shared_ptr<Seq> seq;
    if (*seqArg != *Nil) {
        if (seq = get_if<Seq>(seqArg); !seq) {
            throw lib::LibError("expected core/cons arg 1 to be sequence");
        }
    } else {
        seq = std::make_shared<List>();
    }

    return Cons::make_atom(val, seq);
}

struct LazySeq : public Seq, std::enable_shared_from_this<LazySeq>
{
    // todo: env needs to be shared or passed to next :(
    LazySeq(csxp::Env* env, std::shared_ptr<Callable> call) :
        env(env),
        call(call)
    {
    }

    void runbody() {
        // todo: need empty iterator
        auto empty = std::make_shared<List>();
        auto it = empty->iterator();
        auto res = (*call)(env, it.get());

        // done with these...
        env = nullptr;
        call.reset();

        if (*res != *Nil) {
            if (cache = get_if<Seq>(res); !cache) {
                throw lib::LibError("expected core/lazy-seq arg 1 to be nil or sequence");
            }
        } else {
            // todo: shared empty sequence
            cache = std::make_shared<List>();
        }
    }

    static patom make_atom(csxp::Env* env, std::shared_ptr<Callable> call)
    {
        return std::make_shared<atom>(std::make_shared<LazySeq>(env, call));
    }

    std::shared_ptr<AtomIterator> iterator() const;

    csxp::Env* env;
    std::shared_ptr<Callable> call;

    std::shared_ptr<Seq> cache;
};

struct LazySeqIterator : public AtomIterator
{
public:
    LazySeqIterator(std::shared_ptr<const LazySeq> seq) :
        seq(seq)
    {
    }

    bool next()
    {
        if (seq)
        {
            if (!seq->cache) {
                // really don't like doing this, but iterator
                // should be const all over the place
                const_cast<LazySeq*>(seq.get())->runbody();
            }

            assert(!it);
            it = seq->cache->iterator();

            seq.reset();
        }

        if (it->next()) {
            curr = it->value();
            return true;
        } else {
            // we're done done.
            it.reset();
            curr.reset();
            return false;
        }

        return true;
    }

    patom value() const
    {
        return curr;
    }

private:
    std::shared_ptr<const LazySeq> seq;
    std::shared_ptr<AtomIterator> it;
    patom curr;
};

std::shared_ptr<AtomIterator> LazySeq::iterator() const
{
    return std::make_shared<LazySeqIterator>(shared_from_this());
}

patom lazy_seq(csxp::Env* env, AtomIterator* args)
{
    // todo: need empty vec
    auto emptyvec = std::make_shared<Vec>();
    auto call = fn::makefn(env, emptyvec, args);
    return LazySeq::make_atom(env, call);
}

} // namespace csxp::lib::detail::lazy
