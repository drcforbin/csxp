#ifndef CSXP_ATOM_H
#define CSXP_ATOM_H

#include "pdata/map.h"

namespace csxp {

// forward needed by Callable
struct Env;

// todo: order classes
// todo: use override keyword

struct Callable;
struct Char;
struct Const;
struct Keyword;
struct List; // not in variant_type
struct Map;  // not in variant_type
struct Num;
struct Seq;
struct Str;
struct SymName;
struct Vec; // not in variant_type

// todo; nest in atom?
// note that list, vec, and map are not in the variant; they need to be
// casted from seq to their specific types.
using variant_type = std::variant<
        std::monostate,
        std::shared_ptr<Callable>,
        std::shared_ptr<Char>,
        std::shared_ptr<Const>,
        std::shared_ptr<Keyword>,
        std::shared_ptr<Num>,
        std::shared_ptr<Seq>,
        std::shared_ptr<Str>,
        std::shared_ptr<SymName>>;

struct atom
{
    // todo: using shared_ptr seems lazy...unique_ptr may be a better fit


    atom() = default;
    atom(variant_type v);
    atom(const atom& other);
    atom(atom&& other) noexcept;

    atom& operator=(const atom& other);
    atom& operator=(atom&& other) noexcept;

    variant_type p;
};

bool operator==(const atom& lhs, const atom& rhs);
bool operator!=(const atom& lhs, const atom& rhs);

using patom = std::shared_ptr<atom>;

template <typename T>
bool atom_vals_eq(const T& a, const T& b)
{
    if (!a != !b) {
        // one is null, other not null
        return false;
    }

    if (a) {
        // both not null, compare values
        return *a == *b;
    }

    // both null
    return true;
}

struct AtomIterator
{
    virtual ~AtomIterator() = default;

    virtual bool next() = 0;
    virtual patom value() const = 0;
};

struct SeqIt;

struct Seq
{
    virtual ~Seq() = default;

    SeqIt begin();
    SeqIt end();

    // I'd really like this to be const, but things like
    // lazy-seq need to store the result when next is called,
    // and other seqs want to retain the head
    virtual std::shared_ptr<AtomIterator> iterator() const = 0;
};

inline bool operator==(const Seq& lhs, const Seq& rhs);
inline bool operator!=(const Seq& lhs, const Seq& rhs);

// todo: implement const iterator too
struct SeqIt
{
private:
    class AtomHolder
    {
    public:
        AtomHolder(patom atom) :
            atom(atom) {}
        patom operator*() { return atom; }

    private:
        patom atom;
    };

public:
    typedef patom value_type;
    typedef std::ptrdiff_t difference_type;
    typedef patom* pointer;
    typedef patom& reference;
    typedef std::input_iterator_tag iterator_category;

    explicit SeqIt(Seq* s)
    {
        if (s) {
            it = s->iterator();
        }
    }

    patom operator*() const { return curr; }
    const patom* operator->() const { return &curr; }
    bool operator==(const SeqIt& other) const { return it == other.it; }
    bool operator!=(const SeqIt& other) const { return !(*this == other); }

    AtomHolder operator++(int)
    {
        AtomHolder ret(curr);
        ++*this;
        return ret;
    }

    SeqIt& operator++()
    {
        if (it) {
            if (!it->next()) {
                it.reset();
            } else {
                curr = it->value();
            }
        }
        return *this;
    }

private:
    std::shared_ptr<AtomIterator> it;
    patom curr;
};

struct Callable
{
    virtual ~Callable() = default;

    virtual patom operator()(csxp::Env* env, AtomIterator* args) = 0;
};

struct Const
{
    Const(std::string_view name) :
        name(name) {}

    // todo: should these makes be const?
    // todo: should this be a common template func?
    static patom make_atom(std::string_view name)
    {
        return std::make_shared<atom>(std::make_shared<Const>(name));
    }

    std::string name;
};

bool operator==(const Const& lhs, const Const& rhs);
bool operator!=(const Const& lhs, const Const& rhs);

struct Char
{
    Char(char32_t val) :
        val(val) {}

    static patom make_atom(char32_t val)
    {
        return std::make_shared<atom>(std::make_shared<Char>(val));
    }

    char32_t val;
};

bool operator==(const Char& lhs, const Char& rhs);
bool operator!=(const Char& lhs, const Char& rhs);

struct Keyword
{
    Keyword(std::string_view name) :
        name(name) {}

    static patom make_atom(std::string_view name)
    {
        return std::make_shared<atom>(std::make_shared<Keyword>(name));
    }

    std::string name;
};

inline bool operator==(const Keyword& lhs, const Keyword& rhs)
{
    return lhs.name == rhs.name;
}
inline bool operator!=(const Keyword& lhs, const Keyword& rhs)
{
    return !(lhs == rhs);
}

struct List : public Seq, std::enable_shared_from_this<List>
{
    List() = default;
    // todo: bench this vs const ref vs move
    List(const std::vector<patom>& items) :
        items(items) {}
    List(std::vector<patom>&& items) :
        items(items) {}

    static patom make_atom()
    {
        return std::make_shared<atom>(std::make_shared<List>());
    }

    static patom make_atom(const std::vector<patom>& items)
    {
        return std::make_shared<atom>(std::make_shared<List>(items));
    }

    static patom make_atom(std::vector<patom>&& items)
    {
        return std::make_shared<atom>(std::make_shared<List>(items));
    }

    std::shared_ptr<AtomIterator> iterator() const;

    // cheating, I think
    std::vector<patom> items;
};

struct Map : public Seq, std::enable_shared_from_this<Map>
{
    Map() = default;
    Map(std::vector<std::pair<patom, patom>>& pairs)
    {
        //for (auto& item : items) {
        //    items->assoc
        //}
    }

    static patom make_atom()
    {
        return std::make_shared<atom>(std::make_shared<Map>());
    }

    static patom make_atom(std::vector<std::pair<patom, patom>> items)
    {
        return std::make_shared<atom>(std::make_shared<Map>(items));
    }

    std::shared_ptr<AtomIterator> iterator() const;

    std::shared_ptr<pdata::map_base<patom, patom>> items =
            std::make_shared<pdata::persistent_map<patom, patom>>();
};

struct Num
{
    Num(int val) :
        val(val) {}

    static patom make_atom(int val)
    {
        return std::make_shared<atom>(std::make_shared<Num>(val));
    }

    int val;
};

bool operator==(const Num& lhs, const Num& rhs);
bool operator!=(const Num& lhs, const Num& rhs);

struct Str
{
    Str(std::string_view val) :
        val(val) {}

    static patom make_atom(std::string_view val)
    {
        return std::make_shared<atom>(std::make_shared<Str>(val));
    }

    std::string val;
};

bool operator==(const Str& lhs, const Str& rhs);
bool operator!=(const Str& lhs, const Str& rhs);

struct SymName
{
    SymName(std::string_view name) :
        name(name) {}

    static patom make_atom(std::string_view name)
    {
        return std::make_shared<atom>(std::make_shared<SymName>(name));
    }

    std::string name;
};

bool operator==(const SymName& lhs, const SymName& rhs);
bool operator!=(const SymName& lhs, const SymName& rhs);

struct Vec : public Seq, std::enable_shared_from_this<Vec>
{
    Vec() = default;
    // todo: do for map
    Vec(const std::vector<patom>& items) :
        items(items) {}
    Vec(std::vector<patom>&& items) :
        items(items) {}

    static patom make_atom()
    {
        return std::make_shared<atom>(std::make_shared<Vec>());
    }

    static patom make_atom(const std::vector<patom>& items)
    {
        return std::make_shared<atom>(std::make_shared<Vec>(items));
    }

    static patom make_atom(std::vector<patom>&& items)
    {
        return std::make_shared<atom>(std::make_shared<Vec>(items));
    }

    std::shared_ptr<AtomIterator> iterator() const;

    std::vector<patom> items;
};

// get a value from a patom; throws bad_variant_access if wrong type or null
template <typename T>
std::shared_ptr<T> get(patom atom)
{
    if (atom) {
        if (auto p = std::get<std::shared_ptr<T>>(atom->p)) {
            return p;
        }
    }
    throw std::bad_variant_access();
}

// get a value from a patom; throws bad_variant_access if wrong type or null.
template <> std::shared_ptr<Map> get<Map>(patom atom);
// get a value from a patom; throws bad_variant_access if wrong type or null.
template <> std::shared_ptr<List> get<List>(patom atom);
// get a value from a patom; throws bad_variant_access if wrong type or null.
template <> std::shared_ptr<Vec> get<Vec>(patom atom);

// get a value from a patom; returns null if wrong type or null
template <typename T>
std::shared_ptr<T> get_if(patom atom) noexcept
{
    if (atom) {
        if (auto p = std::get_if<std::shared_ptr<T>>(&atom->p)) {
            return *p;
        }
    }
    return {};
}

// get a value from a patom; returns null if wrong type or null
template <> std::shared_ptr<Map> get_if<Map>(patom atom) noexcept;
// get a value from a patom; returns null if wrong type or null
template <> std::shared_ptr<List> get_if<List>(patom atom) noexcept;
// get a value from a patom; returns null if wrong type or null
template <> std::shared_ptr<Vec> get_if<Vec>(patom atom) noexcept;

struct unexpected_atom_type {};

// visits the supplied visitor if it is callable for whatever
// type the atom contains. does not throw or error if the atom
// contains something that cannot be passed to the visitor, unless
// it contains some type unexpected by visit_if itself. Note that
// a runtime cost is associated with Map, List, and Vec visitors
// (as the Seq has to be dynamically casted to check whether
// that's what the Seq is). to handle the case where the atom
// contains something other than can be visited, have visitor accept
// unexpected_atom_type.
template <typename Visitor>
inline auto visit_if(Visitor&& vis, patom atom) -> auto {
    std::visit(
            [&vis](auto&& val) -> auto {
                using T = std::decay_t<decltype(val)>;
                using I = std::is_invocable<Visitor, decltype(val)>;
                using U = std::is_invocable<Visitor, unexpected_atom_type>;

                if constexpr (std::is_same_v<T, std::monostate>) {
                    // noop
                } else if constexpr (std::is_same_v<T, std::shared_ptr<Callable>>) {
                    if constexpr (I::value) {
                        return vis(val);
                    } else if constexpr (U::value) {
                        return vis(unexpected_atom_type{});
                    }
                } else if constexpr (std::is_same_v<T, std::shared_ptr<Char>>) {
                    if constexpr (I::value) {
                        return vis(val);
                    } else if constexpr (U::value) {
                        return vis(unexpected_atom_type{});
                    }
                } else if constexpr (std::is_same_v<T, std::shared_ptr<Const>>) {
                    if constexpr (I::value) {
                        return vis(val);
                    } else if constexpr (U::value) {
                        return vis(unexpected_atom_type{});
                    }
                } else if constexpr (std::is_same_v<T, std::shared_ptr<Keyword>>) {
                    if constexpr (I::value) {
                        return vis(val);
                    } else if constexpr (U::value) {
                        return vis(unexpected_atom_type{});
                    }
                } else if constexpr (std::is_same_v<T, std::shared_ptr<Num>>) {
                    if constexpr (I::value) {
                        return vis(val);
                    } else if constexpr (U::value) {
                        return vis(unexpected_atom_type{});
                    }
                } else if constexpr (std::is_same_v<T, std::shared_ptr<Str>>) {
                    if constexpr (I::value) {
                        return vis(val);
                    } else if constexpr (U::value) {
                        return vis(unexpected_atom_type{});
                    }
                } else if constexpr (std::is_same_v<T, std::shared_ptr<SymName>>) {
                    if constexpr (I::value) {
                        return vis(val);
                    } else if constexpr (U::value) {
                        return vis(unexpected_atom_type{});
                    }
                } else if constexpr (std::is_same_v<T, std::shared_ptr<Seq>>) {
                    if constexpr (std::is_invocable<Visitor, std::shared_ptr<Map>&&>::value) {
                        if (auto m = std::dynamic_pointer_cast<Map>(val)) {
                            return vis(std::move(m));
                        }
                    }

                    if constexpr (std::is_invocable<Visitor, std::shared_ptr<List>&&>::value) {
                        if (auto l = std::dynamic_pointer_cast<List>(val)) {
                            return vis(std::move(l));
                        }
                    }

                    if constexpr (std::is_invocable<Visitor, std::shared_ptr<Vec>&&>::value) {
                        if (auto v = std::dynamic_pointer_cast<Vec>(val)) {
                            return vis(std::move(v));
                        }
                    }

                    if constexpr (I::value) {
                        return vis(val);
                    } else if constexpr (U::value) {
                        return vis(unexpected_atom_type{});
                    }
                } else {
                    // todo: add type to msg
                    throw std::runtime_error("unexpected type in atom");
                }
            },
            atom->p);
}

template <typename Fn>
patom make_callable(Fn fn)
{
    struct Thunk : public Callable
    {
        Thunk(Fn fn) :
            fn(fn)
        {}

        patom operator()(csxp::Env* env, AtomIterator* args)
        {
            return fn(env, args);
        }

        Fn fn;
    };

    return std::make_shared<atom>(
            std::make_shared<Thunk>(fn));
}

bool is_nil(patom a);
bool truthy(patom a);

// Global nil constant
extern const patom Nil;

// Global true constant
extern const patom True;

// Global false constant
extern const patom False;

} // namespace csxp

#endif // CSXP_ATOM_H
