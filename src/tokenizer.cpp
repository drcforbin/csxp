#include "logging.h"
#include "tokenizer.h"

#include <deque>
#include <regex>
#include <string_view>
#include <variant>

#define LOGGER() (logging::get("tokenizer"))

using namespace std::literals;

// symbols begin with nonumeric, can contain
//   *, +, !, -, _, ', ?, <, >, and =
//   can contain one or more non repeating :
//     symbols beginning or ending w/ : are reserved
//   / namespace indicator
//   . class namespace
// ".." - string; can be multiple lines
// 'form - quote form
// \c - char literal
//    \newline
//    \space
//    \tab
//    \formfeed
//    \backspace
//    \return
// \uNNNN - unicode literal
// nil - nil
// true, false - booleans
// ##Inf, ##-Inf, ##NaN
// keywords can and must begin with :
//  cannot contain . inCurr name part
//  can contain / for ns, which may include .
//  keyword beginning with two colons gets current ns
// (...) - list
// [...] - vec
// {...} - map, must be pairs
// , - whitespace
// todo: map namespace syntax
// #{...} - set
// todo: deftype, defrecord, and constructor calls
// ; - comment
// @ - deref, @form -> (deref form)
// ^ - metadata
//     ^Type → ^{:tag Type}
//     ^:key → ^{:key true}
//       e.g., ^:dynamic ^:private ^:doc ^:const
// ~ - unquote
// ~@ - unquote splicing
//
// #{} - set
// #" - rx pattern
// #' - #'x -> (var x)
// #(...) - (fn [args] (...)), args %, %n, %&
// #_ - ignore next form
//
// ` - for forms other than sym, list, vec, set, map, `x is 'x
//     todo
//
// skipping tagged literals, reader conditional, splicing reader conditional
//
// {...} - map
//

// special forms:
// def
// if
// do
// let
// quote
// var
// fn
// loop
// recurregexp.MustCompile
// throw, try, monitor-enter, monitor-exit, new, set!
//
// - destructuring
//
// & - rest

constexpr bool is_word(char ch)
{
    return ('a' <= ch && ch <= 'z') ||
           ('A' <= ch && ch <= 'Z') ||
           ('0' <= ch && ch <= '9') ||
           ch == '_' || ch == '$';
}

constexpr bool is_sep(char ch)
{
    switch (ch) {
        case ' ':
            [[fallthrough]];
        case '\f':
            [[fallthrough]];
        case '\n':
            [[fallthrough]];
        case '\r':
            [[fallthrough]];
        case '\t':
            [[fallthrough]];
        case '\v':
            [[fallthrough]];
        // todo: unicode support
        // case 0x00a0: [[fallthrough]];
        // case 0x1680: [[fallthrough]];
        // case 0x2000 ... 0x200a: [[fallthrough]];
        // case 0x2028: [[fallthrough]];
        // case 0x2029: [[fallthrough]];
        // case 0x202f: [[fallthrough]];
        // case 0x205f: [[fallthrough]];
        // case 0x3000: [[fallthrough]];
        // case 0xfeff: [[fallthrough]];
        case '[':
            [[fallthrough]];
        case ']':
            [[fallthrough]];
        case '{':
            [[fallthrough]];
        case '}':
            [[fallthrough]];
        case '(':
            [[fallthrough]];
        case ')':
            [[fallthrough]];
        case '\'':
            [[fallthrough]];
        case '"':
            [[fallthrough]];
        case '`':
            [[fallthrough]];
        case ',':
            [[fallthrough]];
        case ';':
            return true;
        default:
            return false;
    }
}

constexpr bool is_digit_base10(char ch)
{
    return '0' <= ch && ch <= '9';
}

constexpr bool is_valid_charesc(char ch)
{
    return true; // todo: actually care.
}

constexpr bool is_hex(char ch)
{
    return ('0' <= ch && ch <= '9') ||
           ('A' <= ch && ch <= 'F') ||
           ('a' <= ch && ch <= 'f');
}

constexpr bool is_octal(char ch)
{
    return '0' <= ch && ch <= '7';
}

constexpr int to_digit(char ch)
{
    if ('0' <= ch && ch <= '9') {
        return ch - '0';
    }
    if ('A' <= ch && ch <= 'Z') {
        return 10 + (ch - 'A');
    }
    if ('a' <= ch && ch <= 'z') {
        return 10 + (ch - 'a');
    }

    // todo: some other error handling?
    return -1;
}

struct Handler
{
    Handler(std::string_view src, std::string_view name) :
        src(src),
        m_pos{0, name}
    {
    }

    Token findToken();

    struct Normal
    {};
    struct Char
    {};
    // todo: implement (maybe, benchmark. it'll need
    // to emit sym if no match, and may not be worth it)
    // struct Nil { };
    struct String
    {};
    struct Comment
    {};
    struct Keyword
    {};
    struct NamespacedKeyword
    {};
    struct Number
    {
        bool neg = false;
        int curr_int = 0;
        int curr_base = 10;
    };
    // todo: not quite right...decimal doesn't need base
    struct DecimalNumber : public Number
    {};
    struct IntegerNumber : public Number
    {};

    using State = std::variant<
            Normal,
            Char,
            String,
            Comment,
            Keyword, NamespacedKeyword,
            Number, DecimalNumber, IntegerNumber>;

    [[noreturn]] void throwError(const char* msg)
    {
        throw std::runtime_error(fmt::format("({}:{}) tokenizer error: {}",
                m_pos.line, m_pos.name, msg));
    }

    State handle(Normal&& state, char ch)
    {
        switch (ch) {
            case '\n':
                m_pos.line++;
                [[fallthrough]];
            case '\r':
                [[fallthrough]];
            case ' ':
                [[fallthrough]];
            case '\f':
                [[fallthrough]];
            case '\t':
                [[fallthrough]];
            case '\v':
                [[fallthrough]];
            // todo: unicode support
            // case 0x00a0: [[fallthrough]];
            // case 0x1680: [[fallthrough]];
            // case 0x2000 ... 0x200a: [[fallthrough]];
            // case 0x2028: [[fallthrough]];
            // case 0x2029: [[fallthrough]];
            // case 0x202f: [[fallthrough]];
            // case 0x205f: [[fallthrough]];
            // case 0x3000: [[fallthrough]];
            // case 0xfeff: [[fallthrough]];
            case ',':
                // whitespace. emit anything we've accumulated
                // and just ignore this char
                if (!build.empty()) {
                    emit(Tk::sym);
                }
                break;

            case '~':
                // tilde can stand on its own or
                // be followed by @
                assert(build.empty());
                build.push_back(ch);
                if (peek_next() == '@') {
                    build.push_back(get_next());
                }
                emit(Tk::sym);
                break;

            // brackets, curly brackets, and parens are all tokens,
            // as well as separators. emit any symbol we might have,
            // and emit the token next round
            case '[':
                sep_emit(ch, Tk::lbracket);
                break;
            case ']':
                sep_emit(ch, Tk::rbracket);
                break;

            case '{':
                sep_emit(ch, Tk::lcurly);
                break;
            case '}':
                sep_emit(ch, Tk::rcurly);
                break;

            case '(':
                sep_emit(ch, Tk::lparen);
                break;
            case ')':
                sep_emit(ch, Tk::rparen);
                break;

            // quote is a token and a symbol.
            case '\'':
                sep_emit(ch, Tk::quote);
                break;

            // backtick is a separator, so we may need
            // to emit a symbol before emitting the
            // symbol for the backtick itself. the rest
            // of the chars here just emit their symbol.
            case '`':
                if (sep_emit(ch)) {
                    break;
                }
                [[fallthrough]];
            case '^':
                [[fallthrough]];
            case '@':
                [[fallthrough]];
            case '&':
                assert(build.empty());
                build.push_back(ch);
                emit(Tk::sym);
                break;

            // dash may be in a symbol, or lead a number.
            case '-':
                build.push_back(ch);
                if (build.size() == 1) {
                    if (auto ch2 = peek_next(); ch2 &&
                                                is_digit_base10(*ch2)) {
                        // no unget here, just need to set neg flag
                        // todo: move to number ctor that accepts a ch?
                        return Number{.neg = true};
                    }
                }
                break;

            // numbers may be in a symbol or lead a number
            case '0' ... '9':
                if (build.empty()) {
                    // we want to unget, so we can take this
                    // character into account as the lead digit
                    // todo: move to number ctor that accepts a ch?
                    unget_next(ch);
                    return Number{};
                } else {
                    build.push_back(ch);
                }
                break;

            // double quote is a separator and begins a string
            case '"':
                if (!sep_emit(ch)) {
                    return String{};
                }
                break;

            // semicolon is a separator and begins a comment
            case ';':
                if (!sep_emit(ch)) {
                    return Comment{};
                }
                break;

            // colon may begin a keyword, or appear once in a symbol
            case ':':
                if (build.empty()) {
                    // add the colon, begin keyword
                    build.push_back(ch);
                    return Keyword{};
                } else {
                    // todo: this kinda looks like it'll allow multiple
                    // colons in a symbol...that's not right. add test
                    build.push_back(ch);
                }
                break;

            // backslash is a separator and begins a char
            case '\\':
                if (!sep_emit(ch)) {
                    return Char{};
                }
                break;

            // anything else is assumed a sym char
            default:
                build.push_back(ch);
                break;
        }

        return state;
    }

    State handle(Char&& state, char ch)
    {
        // shouldn't have collected anything yet
        assert(build.empty());

        // we're going to go ahead and eat
        // the characters we're checking against,
        // and throw if no match
        try {
            // if the next char is a separator,
            // we have the single char we want
            auto nextch = peek_next();
            if (!nextch || is_sep(*nextch)) {
                // no more chars, or followed by a sep. emit.
                build.push_back(ch);
                emit(Tk::chr);
                return Normal{};
            }

            // otherwise, see if it's unicode or octal
            if (ch == 'u') { // unicode val?
                // todo: this is a hack, really need to encode as utf8
                build.push_back(ch);
                ch = get_next();
                if (!is_hex(ch)) {
                    throwError("invalid unicode character");
                }
                build.push_back(ch);
                ch = get_next();
                if (!is_hex(ch)) {
                    throwError("invalid unicode character");
                }
                build.push_back(ch);
                ch = get_next();
                if (!is_hex(ch)) {
                    throwError("invalid unicode character");
                }
                build.push_back(ch);
                ch = get_next();
                if (!is_hex(ch)) {
                    throwError("invalid unicode character");
                }
                build.push_back(ch);
                emit(Tk::chr);
                return Normal{};
            }

            if (ch == 'o') { // octal value?
                // todo: this is a hack, really need to encode as utf8
                build.push_back(ch);
                ch = get_next();
                if (!is_octal(ch)) {
                    throwError("invalid unicode character");
                }
                build.push_back(ch);
                ch = get_next();
                if (!is_octal(ch)) {
                    throwError("invalid unicode character");
                }
                build.push_back(ch);
                ch = get_next();
                if (!is_octal(ch)) {
                    throwError("invalid unicode character");
                }
                build.push_back(ch);
                emit(Tk::chr);
                return Normal{};
            }

            // if none of those, then we should see
            // whether we have one of the 'long' chars

            const char* tgt = nullptr;
            char result = 0;

            switch (ch) {
                case 'n': { // newline?
                    tgt = "ewline";
                    result = '\n';
                    break;
                }
                case 's': { // space?
                    tgt = "pace";
                    result = ' ';
                    break;
                }
                case 't': { // tab?
                    tgt = "ab";
                    result = '\t';
                    break;
                }
                case 'f': { // formfeed?
                    tgt = "ormfeed";
                    result = '\f';
                    break;
                }
                case 'b': { // backspace?
                    tgt = "ackspace";
                    result = '\b';
                    break;
                }
                case 'r': { // return?
                    tgt = "eturn";
                    result = '\r';
                    break;
                }
                default:
                    throwError("unsupported character");
            }

            // eat and test the rest of the string
            for (; *tgt; tgt++) {
                if (*tgt != get_next()) {
                    throwError("unsupported character");
                }
            }

            // we should be out of chars
            if (auto nextch = peek_next(); !nextch || !is_sep(*nextch)) {
                throwError("unsupported character");
            }

            build.push_back(result);
            emit(Tk::chr);
            return Normal{};
        } catch (std::out_of_range&) {
            throwError("unsupported character");
        }

        // really, should never get here
        assert(false);
        return state;
    }

    State handle(String&& state, char ch)
    {
        if (ch == '"') {
            // end of string
            emit(Tk::str);
            return Normal{};
        } else if (ch == '\\') {
            // todo: do we want to actually eval the
            // escaped character?
            build.push_back(ch);
            if (auto ch2 = peek_next(); ch2 &&
                                        is_valid_charesc(*ch2)) {
                build.push_back(get_next());
            } else {
                throwError("invalid escape");
            }
        } else {
            build.push_back(ch);
        }

        return state;
    }

    State handle(Comment&& state, char ch)
    {
        if (ch == '\n') {
            m_pos.line++;
            return Normal{};
        }

        if (ch == '\r') {
            // check for and eat the \n if it follows
            if (auto ch2 = peek_next(); ch2 && *ch2 == '\n') {
                get_next();
            }

            m_pos.line++;
            return Normal{};
        }

        return state;
    }

    State handle(Keyword&& state, char ch)
    {
        if (is_word(ch) ||
                (ch == ':' && build.size() == 1)) {
            // allow word chars or another colon (only at beginning)
            build.push_back(ch);
        } else if (ch == '/' && build.size() > 1) {
            // need at least one char beyond the colon
            // before adding a namespace
            build.push_back(ch);
            return NamespacedKeyword{};
        } else if (build.size() > 1) {
            // we're done here.
            unget_next(ch);
            emit(Tk::keyword);
            return Normal{};
        } else {
            // can't end a keyword with just a colon
            throwError("invalid keyword");
        }

        return state;
    }

    State handle(NamespacedKeyword&& state, char ch)
    {
        if (is_word(ch)) {
            build.push_back(ch);
        } else if (!build.empty()) {
            unget_next(ch);
            emit(Tk::keyword);
            return Normal{};
        } else {
            throwError("invalid keyword");
        }

        return state;
    }

    State handle(Number&& state, char ch)
    {
        if ('0' <= ch && ch <= '9') {
            // todo: handle octal!
            if (!build.empty()) {
                state.curr_int *= state.curr_base;
            }
            state.curr_int += to_digit(ch);
            build.push_back(ch);
        } else if (ch == '.') {
            return DecimalNumber{state};
        } else if (ch == 'r' || ch == 'R') {
            state.curr_base = state.curr_int;
            state.curr_int = 0;
            // todo: verify base 2-32 (check docs to make sure it's 32)
            build.push_back(ch);
            return IntegerNumber{state};
        } else if (ch == 'x' || ch == 'X') {
            state.curr_base = 16;
            state.curr_int = 0;
            build.push_back(ch);
            return IntegerNumber{state};
        } else if (is_sep(ch)) {
            unget_next(ch);
            emit(Tk::num);
            return Normal{};
        } else {
            throwError("invalid number");
        }

        return state;
    }

    State handle(DecimalNumber&& state, char ch)
    {
        // todo: add decimal unit tests
        // todo: implement decimals
        throwError("decimals not implemented");
    }

    State handle(IntegerNumber&& state, char ch)
    {
        if (('0' <= ch && ch <= '9') ||
                ('A' <= ch && ch <= 'Z') ||
                ('a' <= ch && ch <= 'z')) {
            int dig = to_digit(ch);
            // because of check above, dig should never be -1
            assert(dig != -1);
            if (dig < state.curr_base) {
                if (!build.empty()) {
                    state.curr_int *= state.curr_base;
                }
                state.curr_int += dig;
                build.push_back(ch);
            } else {
                throwError("invalid integer");
            }
        } else if (is_sep(ch)) {
            unget_next(ch);
            emit(Tk::num);
            return Normal{};
        } else {
            throwError("invalid integer");
        }

        return state;
    }

    char get_next()
    {
        if (buf) {
            char ch = *buf;
            buf = std::nullopt;
            return ch;
        } else if (!src.empty()) {
            char ch = src.front();
            src = src.substr(1);
            return ch;
        } else {
            // note: this exception should not be able to exit this class;
            // all calls to get_next follow checks w/ peek or explicit src
            // length checks, except for reading Char, which catches and
            // rethrows a custom exception
            throw std::out_of_range{"attempted read past end of src"};
        }
    }

    void unget_next(char ch)
    {
        // there should only ever be one char peeked at; any
        // call to this should be followed by a get_next (which
        // resets buf) before another unget_next
        assert(!buf);

        buf = ch;
    }

    std::optional<char> peek_next()
    {
        if (buf) {
            return *buf;
        } else if (!src.empty()) {
            return src.front();
        }

        return std::nullopt;
    }

    // emit a single token
    void emit(Tk t)
    {
        this->t = t;
    }

    // emit a symbol if one has accumulated
    [[nodiscard]] bool sep_emit(char ch)
    {
        // this is called for any separator char
        // to emit any accumulated symbol
        if (!build.empty()) {
            unget_next(ch);
            emit(Tk::sym);
            return true;
        } else {
            return false;
        }
    }

    // emit a symbol if one has accumulated, or emit a
    // token if not (this can be called twice, to flush
    // then emit some single char token)
    void sep_emit(char ch, Tk t)
    {
        if (!sep_emit(ch)) {
            build.push_back(ch);
            emit(t);
        }
    }

    std::string_view src;
    Position m_pos;

    std::optional<char> buf;
    std::vector<char> build;

    State state;
    Tk t = Tk::none;
};

Token Handler::findToken()
{
    while (!src.empty() || buf) {
        char ch = get_next();
        state = std::visit(
                [this, ch](auto&& state) -> State {
                    return handle(std::move(state), ch);
                },
                state);

        if (t != Tk::none) {
            Token tk{t, std::string(build.data(), build.size())};
            build.clear();
            t = Tk::none;

            // cheating?
            if (tk.s == "nil"sv) {
                tk.t = Tk::nil;
            }

            return tk;
        }
    }

    // stuff left to emit?
    if (!build.empty()) {
        return std::visit(
                [this](auto&& state) -> Token {
                    using T = std::decay_t<decltype(state)>;
                    std::string str{build.data(), build.size()};
                    build.clear();

                    if constexpr (std::is_same_v<T, Normal>) {
                        // cheating?
                        if (str == "nil"sv) {
                            return {Tk::nil, str};
                        } else {
                            return {Tk::sym, str};
                        }
                    } else if constexpr (std::is_same_v<T, Char>) {
                        // char finishes itself or throws. if we
                        // have chars left, something bad has happened
                        assert(false);
                        throwError("unsupported character");
                    } else if constexpr (std::is_same_v<T, String>) {
                        return {Tk::str, str};
                    } else if constexpr (std::is_same_v<T, Comment>) {
                        // we shouldn't accumulate comments
                        assert(str.empty());
                        return {Tk::none};
                    } else if constexpr (
                            std::is_same_v<T, Keyword> ||
                            std::is_same_v<T, NamespacedKeyword>) {
                        return {Tk::keyword, str};
                    } else if constexpr (
                            std::is_same_v<T, Number> ||
                            std::is_same_v<T, DecimalNumber> ||
                            std::is_same_v<T, IntegerNumber>) {
                        return {Tk::num, str};
                    }
                },
                state);
    }

    return {Tk::eof, {}};
}

class TokenizerImpl : public Tokenizer
{
public:
    TokenizerImpl(std::string_view data, std::string_view name) :
        handler(data, name)
    {
    }

    Position position() const noexcept { return handler.m_pos; }

    // todo: perfect use for c++20 coroutines
    bool next()
    {
        if (m_tk.t == Tk::eof) {
            return false;
        }

        m_tk = handler.findToken();
        return m_tk.t != Tk::eof;
    }

    Token value() const { return m_tk; }

private:
    Handler handler;

    Token m_tk;
};

/*

switch state

case number_decimal
    assert !tk.empty and c != '-' // should have handled leading - in number_base10
    if tk.empty
        curr_decimal = curr_int
    must handle inner .
    case '0' ... '9':
        if !tk.empty
            curr_int *= curr_base
        curr_int += dig(c)
        if neg
            curr_decimal = -curr_decimal
    default:
        if isseparator(c):
            if neg
                curr_decimal = -curr_decimal
            emit
        else
            throw invalid number

*/

std::shared_ptr<Tokenizer> createTokenizer(std::string_view data,
        std::string_view name)
{
    return std::make_shared<TokenizerImpl>(data, name);
}
