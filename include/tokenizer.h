#ifndef TOKENIZER_H
#define TOKENIZER_H

#include "fmt/format.h"

#include <iterator>
#include <memory>
#include <string>

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

class TokenizerIt;

struct Tokenizer
{
    virtual ~Tokenizer() = default;

    virtual Position position() const noexcept = 0;

    virtual bool next() = 0;
    virtual Token value() const = 0;

    // todo: implement const iterator too
    class TokenizerIt
    {
    private:
        class TokenHolder
        {
        public:
            TokenHolder(Token tk) :
                tk(tk) {}
            Token operator*() { return tk; }

        private:
            Token tk;
        };

    public:
        typedef Token value_type;
        typedef std::ptrdiff_t difference_type;
        typedef Token* pointer;
        typedef Token& reference;
        typedef std::input_iterator_tag iterator_category;

        explicit TokenizerIt(Tokenizer* tz) :
            tz(tz) {}
        Token operator*() const { return curr; }
        const Token* operator->() const { return &curr; }
        bool operator==(const TokenizerIt& other) const { return tz == other.tz; }
        bool operator!=(const TokenizerIt& other) const { return !(*this == other); }

        TokenHolder operator++(int)
        {
            TokenHolder ret(curr);
            ++*this;
            return ret;
        }

        TokenizerIt& operator++()
        {
            if (tz) {
                if (!tz->next()) {
                    tz = nullptr;
                } else {
                    curr = tz->value();
                }
            }
            return *this;
        }

    private:
        Tokenizer* tz;
        Token curr;
    };

    TokenizerIt begin()
    {
        auto it = TokenizerIt(this);
        return ++it;
    }

    TokenizerIt end() { return TokenizerIt(nullptr); }
};

// note: tokenizer does not take ownership of or copy str or name.
// both must exist for the lifetime of the reader
std::shared_ptr<Tokenizer> createTokenizer(std::string_view data,
        std::string_view name);

#endif // TOKENIZER_H
