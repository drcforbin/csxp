#include "csxp/atom_fmt.h"
#include "csxp/reader.h"

#include <charconv>
#include <stack>

using namespace std::literals;

namespace csxp {

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

const patom Quote = SymName::make_atom("quote"sv);

constexpr bool is_word(char ch)
{
    return ('a' <= ch && ch <= 'z') ||
           ('A' <= ch && ch <= 'Z') ||
           ('0' <= ch && ch <= '9') ||
           ch == '_' || ch == '$';
}

constexpr bool is_sep(char ch)
{
    // todo: consider collecting stats to order these
    // todo: unicode support
    // 0x00a0
    // 0x1680
    // 0x2000 ... 0x200a
    // 0x2028
    // 0x2029
    // 0x202f
    // 0x205f
    // 0x3000
    // 0xfeff
    return ch == ' ' ||
           ch == ')' || ch == '(' ||
           ch == ']' || ch == '[' ||
           ch == '}' || ch == '{' ||
           ch == '\n' || ch == '\r' ||
           ch == '"' ||
           ch == '\'' ||
           ch == '\t' || ch == '\f' || ch == '\v' ||
           ch == '`' ||
           ch == ',' ||
           ch == ';';
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

[[noreturn]] static void throwError(Position pos, const char* msg)
{
    throw ReaderError(fmt::format("({}:{}) reader error: {}",
            pos.line, pos.name, msg));
}

[[noreturn]] static void throwError(Position pos, const std::string& msg)
{
    throw ReaderError(fmt::format("({}:{}) reader error: {}",
            pos.line, pos.name, msg));
}

struct ReaderFrame
{
    ReaderFrame(patom seq) :
        seq(seq)
    {
    }

    void push(const Position& pos, patom val)
    {
        std::visit(
                [&pos, &val](auto&& s) {
                    using T = std::decay_t<decltype(s)>;
                    if constexpr (std::is_same_v<T, std::shared_ptr<Seq>>) {
                        if (auto m = std::dynamic_pointer_cast<Map>(s)) {
                            // todo: map
                            // we want to capture a key, then value,
                            // then push, repeat. some error handling needed too
                            // m->items.push_back(val);
                        } else if (auto l = std::dynamic_pointer_cast<List>(s)) {
                            l->items.push_back(val);
                        } else if (auto v = std::dynamic_pointer_cast<Vec>(s)) {
                            v->items.push_back(val);
                        } else {
                            throwError(pos, "push to unexpected seq type");
                        }
                    } else {
                        // seq is expected frame to contain seq
                        assert(false);
                        throwError(pos, "push to unexpected stack type");
                    }
                },
                seq->p);
    }

    patom seq;
    bool quoteNext = false;
};

static patom quoteAtom(patom val)
{
    return List::make_atom({Quote, val});
}

struct Handler
{
    Handler(std::string_view src, std::string_view name) :
        src(src),
        m_pos{0, std::string(name)}
    {
    }

    patom findAtom();

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

    using StateAtom = std::pair<State, patom>;

    [[noreturn]] void throwError(const char* msg)
    {
        csxp::throwError(m_pos, msg);
    }

    [[noreturn]] void throwError(const std::string& msg)
    {
        csxp::throwError(m_pos, msg);
    }

    template <typename Atom>
    patom makeAtom(std::string_view str) {
        // no explicit specialization in non-namespace scope? hah.
        if constexpr (std::is_same_v<Atom, SymName>) {
            if (str == "nil"sv) {
                return push_atom(Nil);
            } else {
                return push_atom(SymName::make_atom(str));
            }
        } else if constexpr (std::is_same_v<Atom, Char>) {
            // we know it's a slash + whatever text, min 2 chars
            if (str.size() == 1) {
                return push_atom(csxp::Char::make_atom(str.front()));
            } else if (str.size() == 4 && str.front() == 'o') {
                int base = 8;

                int i = 0;
                auto [p, _] = std::from_chars(
                        str.data() + 1, str.data() + str.size(), i, base);
                if (p != str.data()) {
                    return push_atom(csxp::Char::make_atom(static_cast<char32_t>(i)));
                } else {
                    throwError("unable to parse octal char");
                }
            } else if (str.size() == 5 && str.front() == 'u') {
                int base = 16;

                int i = 0;
                auto [p, _] = std::from_chars(
                        str.data() + 1, str.data() + str.size(), i, base);
                if (p != str.data()) {
                    return push_atom(csxp::Char::make_atom(static_cast<char32_t>(i)));
                } else {
                    throwError("unable to parse unicode char");
                }
            } else {
                throwError("unexpected character size");
            }
        } else if constexpr (std::is_same_v<Atom, Num>) {
            // note: only handling base 10 and 16
            int base = 10;
            std::size_t off = 0;
            if (str.size() > 2) {
                // todo: in c++20, starts_with
                auto prefix = str.substr(0, 2);
                if (prefix == "0x"sv || prefix == "0X"sv) {
                    base = 16;
                    off = 2;
                }
            }
            int i = 0;
            auto [p, _] = std::from_chars(
                    str.data() + off, str.data() + str.size(), i, base);
            if (p != str.data()) {
                return push_atom(Num::make_atom(i));
            } else {
                throwError(
                        fmt::format(
                                "invalid numeric format for '{}'", str));
            }
        } else {
            return push_atom(Atom::make_atom(str));
        }
    }

    template <typename Atom>
    patom makeAtom() {
        auto str = makeAtom<Atom>({build.data(), build.size()});
        build.clear();
        return str;
    }

    StateAtom handle(Normal&& state, char ch)
    {
        patom res;

        switch (ch) {
            case '\n':
                m_pos.line++;
                [[fallthrough]];
            case '\r':
            case ' ':
            case '\f':
            case '\t':
            case '\v':
            // todo: unicode support
            // case 0x00a0:
            // case 0x1680:
            // case 0x2000 ... 0x200a:
            // case 0x2028:
            // case 0x2029:
            // case 0x202f:
            // case 0x205f:
            // case 0x3000:
            // case 0xfeff:
            case ',':
                // whitespace. emit anything we've accumulated
                // and just ignore this char
                if (!build.empty()) {
                    res = makeAtom<SymName>();
                }
                break;

            case '~':
                // todo: unquote, unquote splice
                // tilde can stand on its own or
                // be followed by @
                assert(build.empty());
                build.push_back(ch);
                if (peek_next() == '@') {
                    build.push_back(get_next());
                }
                return {state, makeAtom<SymName>()};

            // brackets, curly brackets, and parens are all tokens,
            // as well as separators. emit any symbol we might have,
            // and emit the token next round
            case '[':
                m_stack.emplace(Vec::make_atom());
                break;
            case ']':
                res = emit_end_frame(ch);
                break;

            case '{':
                m_stack.emplace(Map::make_atom());
                break;
            case '}':
                res = emit_end_frame(ch);
                break;

            case '(':
                m_stack.emplace(List::make_atom());
                break;
            case ')':
                res = emit_end_frame(ch);
                break;

            // quote is a separator and a special symbol.
            case '\'':
                res = emit_quote(ch);
                break;

            // backtick is a separator, so we may need
            // to emit a symbol before emitting the
            // symbol for the backtick itself. the rest
            // of the chars here just emit their symbol.
            case '`':
                // todo: syntax quote
                if (res = sep_emit(ch); res) {
                    break;
                }
                [[fallthrough]];
            case '^':
            case '@':
                // todo: @form -> (deref form)
                [[fallthrough]];
            case '&':
                assert(build.empty());
                build.push_back(ch);
                res = makeAtom<SymName>();
                break;

            // dash may be in a symbol, or lead a number.
            case '-':
                build.push_back(ch);
                if (build.size() == 1) {
                    if (auto ch2 = peek_next(); ch2 &&
                                                is_digit_base10(*ch2)) {
                        // no unget here, just need to set neg flag
                        // todo: move to number ctor that accepts a ch?
                        return {Number{.neg = true}, {}};
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
                    return {Number{}, {}};
                } else {
                    build.push_back(ch);
                }
                break;

            // double quote is a separator and begins a string
            case '"':
                if (res = sep_emit(ch); !res) {
                    return {String{}, {}};
                }
                break;

            // semicolon is a separator and begins a comment
            case ';':
                if (res = sep_emit(ch); !res) {
                    return {Comment{}, {}};
                }
                break;

            // colon may begin a keyword, or appear once in a symbol
            case ':':
                if (build.empty()) {
                    // add the colon, begin keyword
                    build.push_back(ch);
                    return {Keyword{}, {}};
                } else {
                    // todo: this kinda looks like it'll allow multiple
                    // colons in a symbol...that's not right. add test
                    build.push_back(ch);
                }
                break;

            // backslash is a separator and begins a char
            case '\\':
                if (res = sep_emit(ch); !res) {
                    return {Char{}, {}};
                }
                break;

            // # begins reader dispatch
            //case '#':
            //    todo: reader dispatch state
            //    #{...} -> set
            //    #"..." -> ...regex...
            //    #'x -> (var x)
            //    #_ x -> ...nothing...
            //    #(...) -> (fn ...)
            //    #xyz -> ...tagged literals...
            //    #abc/def x -> ...call abc/def with x as arg
            //    #?@ or #? ...reader conditionals
            //    break;

            // anything else is assumed a sym char
            default:
                build.push_back(ch);
                break;
        }

        return {state, res};
    }

    StateAtom handle(Char&& state, char ch)
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
                return {Normal{}, makeAtom<Char>()};
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
                return {Normal{}, makeAtom<Char>()};
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
                return {Normal{}, makeAtom<Char>()};
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
            return {Normal{}, makeAtom<Char>()};
        } catch (std::out_of_range&) {
            throwError("unsupported character");
        }
    }

    StateAtom handle(String&& state, char ch)
    {
        if (ch == '"') {
            // end of string
            return {Normal{}, makeAtom<Str>()};
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

        return {state, {}};
    }

    StateAtom handle(Comment&& state, char ch)
    {
        if (ch == '\n') {
            m_pos.line++;
            return {Normal{}, {}};
        }

        if (ch == '\r') {
            // check for and eat the \n if it follows
            if (auto ch2 = peek_next(); ch2 && *ch2 == '\n') {
                get_next();
            }

            m_pos.line++;
            return {Normal{}, {}};
        }

        return {state, {}};
    }

    StateAtom handle(Keyword&& state, char ch)
    {
        if (is_word(ch) ||
                (ch == ':' && build.size() == 1)) {
            // allow word chars or another colon (only at beginning)
            build.push_back(ch);
        } else if (ch == '/' && build.size() > 1) {
            // need at least one char beyond the colon
            // before adding a namespace
            build.push_back(ch);
            return {NamespacedKeyword{}, {}};
        } else if (build.size() > 1) {
            // we're done here.
            unget_next(ch);
            return {Normal{}, makeAtom<csxp::Keyword>()};
        } else {
            // can't end a keyword with just a colon
            throwError("invalid keyword");
        }

        return {state, {}};
    }

    StateAtom handle(NamespacedKeyword&& state, char ch)
    {
        if (is_word(ch)) {
            build.push_back(ch);
        } else if (!build.empty()) {
            unget_next(ch);
            return {Normal{}, makeAtom<csxp::Keyword>()};
        } else {
            throwError("invalid keyword");
        }

        return {state, {}};
    }

    StateAtom handle(Number&& state, char ch)
    {
        if ('0' <= ch && ch <= '9') {
            // todo: handle octal!
            if (!build.empty()) {
                state.curr_int *= state.curr_base;
            }
            state.curr_int += to_digit(ch);
            build.push_back(ch);
        } else if (ch == '.') {
            return {DecimalNumber{state}, {}};
        } else if (ch == 'r' || ch == 'R') {
            state.curr_base = state.curr_int;
            state.curr_int = 0;
            // todo: verify base 2-32 (check docs to make sure it's 32)
            build.push_back(ch);
            return {IntegerNumber{state}, {}};
        } else if (ch == 'x' || ch == 'X') {
            state.curr_base = 16;
            state.curr_int = 0;
            build.push_back(ch);
            return {IntegerNumber{state}, {}};
        } else if (is_sep(ch)) {
            unget_next(ch);
            return {Normal{}, makeAtom<Num>()};
        } else {
            throwError("invalid number");
        }

        return {state, {}};
    }

    StateAtom handle(DecimalNumber&& state, char ch)
    {
        // todo: add decimal unit tests
        // todo: implement decimals
        throwError("decimals not implemented");
    }

    StateAtom handle(IntegerNumber&& state, char ch)
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
            return {Normal{}, makeAtom<Num>()};
        } else {
            throwError("invalid integer");
        }

        return {state, {}};
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

    // emit a symbol if one has accumulated
    [[nodiscard]] patom sep_emit(char ch)
    {
        // this is called for any separator char
        // to emit any accumulated symbol
        if (!build.empty()) {
            if (auto atom = makeAtom<SymName>(); atom) {
                unget_next(ch);
                return atom;
            }
        }
        return {};
    }

    // emit a symbol if one has accumulated, or ends the frame
    // if not (this can be called twice, to flush
    // then emit some single char token)
    [[nodiscard]] patom emit_end_frame(char ch)
    {
        if (auto atom = sep_emit(ch); atom) {
            return atom;
        } else {
            build.clear();

            // TODO: error on map with leftover keys!

            if (m_stack.size() == 1) {
                auto& frame = m_stack.top();
                auto seq = frame.seq;
                if (m_quoteNext) {
                    seq = quoteAtom(seq);
                    m_quoteNext = false;
                } else if (frame.quoteNext) {
                    throwError("unexpected quote");
                }

                // there should be nothing on the
                // stack, we're returning the accumulated
                // sequence or item
                m_stack.pop();

                return seq;
            } else if (m_stack.size() > 1) {
                // get the top sequence and pop it off
                auto seq = m_stack.top().seq;
                m_stack.pop();

                // prefix quote if needed, and push the
                // accumulated sequence or item to the
                // next stack level
                auto& frame = m_stack.top();
                if (frame.quoteNext) {
                    seq = quoteAtom(seq);
                    frame.quoteNext = false;
                }

                frame.push(m_pos, seq);
            } else {
                throwError("unhandled empty stack on frame end");
            }

            return {};
        }
    }

    // emit a quote if one has accumulated, or emit a
    // token if not (this can be called twice, to flush
    // then emit some single char token)
    [[nodiscard]] patom emit_quote(char ch)
    {
        if (auto atom = sep_emit(ch); atom) {
            return atom;
        } else {
            build.clear();

            if (!m_stack.empty()) {
                m_stack.top().quoteNext = true;
            } else {
                m_quoteNext = true;
            }
            return {};
        }
    }

    patom push_atom(patom atom)
    {
        if (!m_stack.empty()) {
            // quote if needed, and add it to the
            // accumulating sequence or item
            auto& frame = m_stack.top();
            if (frame.quoteNext) {
                atom = quoteAtom(atom);
                frame.quoteNext = false;
            }

            frame.push(m_pos, atom);
            return {};
        } else {
            if (m_quoteNext) {
                atom = quoteAtom(atom);
                m_quoteNext = false;
            }

            return atom;
        }
    }

    std::string_view src;
    Position m_pos;

    std::optional<char> buf;
    std::vector<char> build;

    State state;

    std::stack<ReaderFrame> m_stack;
    bool m_quoteNext = false;
};

patom Handler::findAtom()
{
    patom value;

    while (!src.empty() || buf) {
        char ch = get_next();
        std::tie(state, value) = std::visit(
                [this, ch](auto&& state) -> StateAtom {
                    return handle(std::move(state), ch);
                },
                std::move(state));

        if (value) {
            return value;
        }
    }

    // stuff left to emit?
    if (!build.empty()) {
        return std::visit(
                [this](auto&& state) -> patom {
                    using T = std::decay_t<decltype(state)>;
                    std::string str{build.data(), build.size()};
                    build.clear();

                    if constexpr (std::is_same_v<T, Normal>) {
                        return makeAtom<SymName>(str);
                    } else if constexpr (std::is_same_v<T, Char>) {
                        // char finishes itself or throws. if we
                        // have chars left, something bad has happened
                        assert(false);
                        throwError("character could not be handled");
                    } else if constexpr (std::is_same_v<T, String>) {
                        return makeAtom<Str>(str);
                    } else if constexpr (std::is_same_v<T, Comment>) {
                        // we shouldn't accumulate comments
                        assert(str.empty());
                        return {};
                    } else if constexpr (
                            std::is_same_v<T, Keyword> ||
                            std::is_same_v<T, NamespacedKeyword>) {
                        return makeAtom<csxp::Keyword>(str);
                    } else if constexpr (
                            std::is_same_v<T, Number> ||
                            std::is_same_v<T, DecimalNumber> ||
                            std::is_same_v<T, IntegerNumber>) {
                        return makeAtom<Num>(str);
                    }
                },
                state);
    }

    return {};
}

class ReaderImpl final : public Reader
{
public:
    ReaderImpl(std::string_view str,
            std::string_view name) :
        handler(str, name)
    {
        m_value = handler.findAtom();
    }

    bool next()
    {
        if (first) {
            first = false;
            return static_cast<bool>(m_value);
        }

        m_value = handler.findAtom();
        return static_cast<bool>(m_value);
    }

    patom value()
    {
        return m_value;
    }

private:
    Handler handler;
    bool first = true;
    patom m_value;
};

std::shared_ptr<Reader> createReader(std::string_view str,
        std::string_view name)
{
    return std::make_shared<ReaderImpl>(str, name);
}

} // namespace csxp
