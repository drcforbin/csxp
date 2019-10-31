#ifndef ATOM_H
#define ATOM_H

#include "pdata/map.h"

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

// forward needed by Callable
struct Env;

namespace atom {

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

struct atom
{
    // todo: using shared_ptr seems lazy...unique_ptr may be a better fit

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

    virtual patom operator()(Env* env, AtomIterator* args) = 0;
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
    List(std::vector<patom> items) :
        items(items) {}

    static patom make_atom()
    {
        return std::make_shared<atom>(std::make_shared<List>());
    }

    static patom make_atom(std::vector<patom> items)
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
    // todo: bench this vs const ref vs move
    Vec(std::vector<patom> items) :
        items(items) {}

    static patom make_atom()
    {
        return std::make_shared<atom>(std::make_shared<Vec>());
    }

    static patom make_atom(std::vector<patom> items)
    {
        return std::make_shared<atom>(std::make_shared<Vec>(items));
    }

    std::shared_ptr<AtomIterator> iterator() const;

    std::vector<patom> items;
};

template <typename T>
patom make_callable(T fn)
{
    struct Thunk : public Callable
    {
        Thunk(T fn) :
            fn(fn)
        {}

        patom operator()(Env* env, AtomIterator* args)
        {
            return fn(env, args);
        }

        T fn;
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

} // namespace atom

#endif // ATOM_H
