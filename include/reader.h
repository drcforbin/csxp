#ifndef READER_H
#define READER_H

#include "atom.h"

#include <stdexcept>

// todo: move to .cpp?
enum class Tk
{
    none,
    lparen,
    rparen,
    lbracket,
    rbracket,
    lcurly,
    rcurly,
    sym,
    str,
    chr,
    nil,
    num,
    quote,
    keyword,
    eof
};

template <>
struct fmt::formatter<Tk>
{
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const Tk& tk, FormatContext& ctx)
    {
        switch (tk) {
            case Tk::none:
                return format_to(ctx.out(), "tk::none");
                break;
            case Tk::lparen:
                return format_to(ctx.out(), "tk::lparen");
                break;
            case Tk::lbracket:
                return format_to(ctx.out(), "tk::lbracket");
                break;
            case Tk::lcurly:
                return format_to(ctx.out(), "tk::lcurly");
                break;
            case Tk::rparen:
                return format_to(ctx.out(), "tk::rparen");
                break;
            case Tk::rbracket:
                return format_to(ctx.out(), "tk::rbracket");
                break;
            case Tk::rcurly:
                return format_to(ctx.out(), "tk::rcurly");
                break;
            case Tk::sym:
                return format_to(ctx.out(), "tk::sym");
                break;
            case Tk::str:
                return format_to(ctx.out(), "tk::str");
                break;
            case Tk::keyword:
                return format_to(ctx.out(), "tk::keyword");
                break;
            case Tk::chr:
                return format_to(ctx.out(), "tk::chr");
                break;
            case Tk::nil:
                return format_to(ctx.out(), "tk::nil");
                break;
            case Tk::num:
                return format_to(ctx.out(), "tk::num");
                break;
            case Tk::quote:
                return format_to(ctx.out(), "tk::quote");
                break;
            case Tk::eof:
                return format_to(ctx.out(), "tk::eof");
                break;
            default:
                return format_to(ctx.out(), "tk::<unknown {}>", (int) tk);
        }
    }
};

struct Position
{
    int line = 0;
    std::string_view name;
};

struct Token
{
    Tk t = Tk::none;
    std::string s;
    Position pos;
};

template <>
struct fmt::formatter<Token>
{
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const Token& t, FormatContext& ctx)
    {
        switch (t.t) {
            case Tk::none:
                [[fallthrough]];
            case Tk::lparen:
                [[fallthrough]];
            case Tk::lbracket:
                [[fallthrough]];
            case Tk::lcurly:
                [[fallthrough]];
            case Tk::rparen:
                [[fallthrough]];
            case Tk::rbracket:
                [[fallthrough]];
            case Tk::rcurly:
                [[fallthrough]];
            case Tk::nil:
                [[fallthrough]];
            case Tk::quote:
                [[fallthrough]];
            case Tk::eof:
                return format_to(ctx.out(), "({})", t.t);

            case Tk::sym:
                [[fallthrough]];
            case Tk::str:
                [[fallthrough]];
            case Tk::keyword:
                [[fallthrough]];
            case Tk::chr:
                [[fallthrough]];
            case Tk::num:
                return format_to(ctx.out(), "({} '{}')", t.t, t.s);

            default:
                return format_to(ctx.out(), "<unknown tk {}, '{}'>",
                        (int) t.t, t.s);
        }
    }
};

class ReaderError : public std::runtime_error
{
public:
    explicit ReaderError(const std::string& what_arg) :
        std::runtime_error(what_arg)
    {}

    explicit ReaderError(const char* what_arg) :
        std::runtime_error(what_arg)
    {}
};

struct Reader
{
    virtual ~Reader() = default;

    virtual bool next() = 0;
    virtual atom::patom value() = 0;
};

// note: reader does not take ownership of or copy str or name.
// both must exist for the lifetime of the reader
std::shared_ptr<Reader> createReader(std::string_view str,
        std::string_view name);

class reader_iterator
{
private:
    class AtomHolder
    {
    public:
        AtomHolder(atom::patom atom) :
            atom(atom) {}
        atom::patom operator*() { return atom; }

    private:
        atom::patom atom;
    };

public:
    typedef atom::patom value_type;
    typedef std::ptrdiff_t difference_type;
    typedef atom::patom* pointer;
    typedef atom::patom& reference;
    typedef std::input_iterator_tag iterator_category;

    explicit reader_iterator(std::string_view str, std::string_view name) :
        p(createReader(str, name)) {}
    explicit reader_iterator() {}

    atom::patom operator*() const { return curr; }
    const atom::patom* operator->() const { return &curr; }
    bool operator==(const reader_iterator& other) const { return p == other.p; }
    bool operator!=(const reader_iterator& other) const { return !(*this == other); }

    AtomHolder operator++(int)
    {
        AtomHolder ret(curr);
        ++*this;
        return ret;
    }

    reader_iterator& operator++()
    {
        if (p) {
            if (!p->next()) {
                p = nullptr;
            } else {
                curr = p->value();
            }
        }
        return *this;
    }

private:
    std::shared_ptr<Reader> p;
    atom::patom curr;
};

struct reader
{
    reader(std::string_view str, std::string_view name) :
        str(str), name(name) {}

    reader_iterator begin()
    {
        auto it = reader_iterator(str, name);
        return ++it;
    }

    reader_iterator end() { return reader_iterator(); }
private:
    std::string_view str;
    std::string_view name;
};

#endif //  READER_H
