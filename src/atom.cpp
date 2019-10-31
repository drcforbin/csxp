#include "atom.h"

using namespace std::literals;

namespace atom {

const patom Nil = Const::make_atom("nil"sv);
const patom True = Const::make_atom("true"sv);
const patom False = Const::make_atom("false"sv);

atom::atom(variant_type v) :
    p(v)
{}

atom::atom(const atom& other) :
    p(other.p)
{}

atom::atom(atom&& other) noexcept :
    p(std::move(other.p))
{}

atom& atom::operator=(const atom& other)
{
    return *this = atom(other);
}

atom& atom::operator=(atom&& other) noexcept
{
    std::swap(p, other.p);
    return *this;
}

bool operator==(const atom& lhs, const atom& rhs)
{
    return std::visit(
            [](auto&& lhs, auto&& rhs) -> bool {
                using LT = std::decay_t<decltype(lhs)>;
                using RT = std::decay_t<decltype(rhs)>;

                if constexpr (std::is_same_v<LT, std::monostate> &&
                              std::is_same_v<RT, std::monostate>) {
                    return true;
                } else if constexpr (std::is_same_v<LT, RT>) {
                    if constexpr (std::is_same_v<LT, std::shared_ptr<Callable>>) {
                        // when both callables are null, they're equal,
                        // other callables are never equal (I'm lazy)
                        if (!lhs && !rhs) {
                            return true;
                        }
                        return false;
                    } else {
                        // compare anything else, no casts needed
                        return atom_vals_eq(lhs, rhs);
                    }
                } else {
                    // types differ
                    return false;
                }
            },
            lhs.p, rhs.p);

    return true;
}

bool operator!=(const atom& lhs, const atom& rhs)
{
    return !(lhs == rhs);
}

bool operator==(const Seq& lhs, const Seq& rhs)
{
    const auto lhsit = lhs.iterator();
    const auto rhsit = rhs.iterator();

    bool hasLeft = lhsit->next();
    bool hasRight = rhsit->next();

    // first item from just one?
    if (hasLeft != hasRight) {
        return false;
    }

    // keep checking, if both have items
    if (hasLeft) {
        do {
            auto lhsval = lhsit->value();
            auto rhsval = rhsit->value();

            // both null or not null?
            if (!lhsval != !rhsval) {
                return false;
            } else if (lhsval) {
                // both have items, they equal?
                if (*lhsval != *rhsval) {
                    return false;
                }
            }

            hasLeft = lhsit->next();
            hasRight = rhsit->next();
        } while (hasLeft && hasRight);

        // does one have more items?
        if (hasLeft || hasRight) {
            return false;
        }
    }

    return true;
}

bool operator!=(const Seq& lhs, const Seq& rhs)
{
    return !(lhs == rhs);
}

SeqIt Seq::begin()
{
    auto it = SeqIt(this);
    return ++it;
}

SeqIt Seq::end()
{
    return SeqIt({});
}

bool operator==(const Const& lhs, const Const& rhs)
{
    return lhs.name == rhs.name;
}

bool operator!=(const Const& lhs, const Const& rhs)
{
    return !(lhs == rhs);
}

bool operator==(const Char& lhs, const Char& rhs)
{
    return lhs.val == rhs.val;
}

bool operator!=(const Char& lhs, const Char& rhs)
{
    return !(lhs == rhs);
}

bool operator==(const Num& lhs, const Num& rhs)
{
    return lhs.val == rhs.val;
}

bool operator!=(const Num& lhs, const Num& rhs)
{
    return !(lhs == rhs);
}

bool operator==(const Str& lhs, const Str& rhs)
{
    return lhs.val == rhs.val;
}

bool operator!=(const Str& lhs, const Str& rhs)
{
    return !(lhs == rhs);
}

bool operator==(const SymName& lhs, const SymName& rhs)
{
    return lhs.name == rhs.name;
}

bool operator!=(const SymName& lhs, const SymName& rhs)
{
    return !(lhs == rhs);
}

struct MapIterator : public AtomIterator
{
public:
    MapIterator(std::shared_ptr<const Map> s) :
        s(s)
    {}

    bool next()
    {
        /* todo: map
        int len = static_cast<int>(s->items.size());
        if (s && !s->items.empty() && idx < len - 1) {
            idx++;
            return true;
        }
        */

        return false;
    }

    patom value() const
    {
        /* todo: map
        int len = static_cast<int>(s->items.size());
        if (s && !s->items.empty() && idx < len) {
            return s->items[idx];
        }
        */

        return {};
    }

private:
    std::shared_ptr<const Map> s;
};

std::shared_ptr<AtomIterator> Map::iterator() const
{
    return std::make_shared<MapIterator>(shared_from_this());
}

template <typename SeqT>
struct ItemsIterator : public AtomIterator
{
public:
    ItemsIterator(std::shared_ptr<const SeqT> s) :
        s(s)
    {}

    bool next()
    {
        int len = static_cast<int>(s->items.size());
        if (s && !s->items.empty() && idx < len - 1) {
            idx++;
            return true;
        }

        return false;
    }

    patom value() const
    {
        int len = static_cast<int>(s->items.size());
        if (s && !s->items.empty() && idx < len) {
            return s->items[idx];
        }

        return {};
    }

private:
    std::shared_ptr<const SeqT> s;
    int idx = -1;
};

std::shared_ptr<AtomIterator> List::iterator() const
{
    return std::make_shared<ItemsIterator<List>>(shared_from_this());
}

std::shared_ptr<AtomIterator> Vec::iterator() const
{
    return std::make_shared<ItemsIterator<Vec>>(shared_from_this());
}

bool is_nil(patom a)
{
    return !a || *a == *Nil || a->p.index() == 0;
}

bool truthy(patom a)
{
    return !is_nil(a) && *a != *False;
}

} // namespace atom
