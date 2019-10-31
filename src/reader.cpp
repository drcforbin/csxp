#include "atom-fmt.h"
#include "parser.h"
#include "reader.h"
#include "tokenizer.h"

#include <charconv>

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

[[noreturn]] void throwError(Position pos, const char* msg)
{
    throw std::runtime_error(fmt::format("({}:{}) reader error: {}",
            pos.line, pos.name, msg));
}

[[noreturn]] void throwError(Position pos, const std::string& msg)
{
    throw std::runtime_error(fmt::format("({}:{}) reader error: {}",
            pos.line, pos.name, msg));
}

struct ReaderFrame
{
    ReaderFrame(atom::patom seq) :
        seq(seq)
    {
    }

    void push(const Position& pos, atom::patom val)
    {
        std::visit(
                [&pos, &val](auto&& s) {
                    using T = std::decay_t<decltype(s)>;
                    if constexpr (std::is_same_v<T, std::shared_ptr<atom::Seq>>) {
                        if (auto m = std::dynamic_pointer_cast<atom::Map>(s)) {
                            // todo: map
                            // we want to capture a key, then value,
                            // then push, repeat. some error handling needed too
                            // m->items.push_back(val);
                        } else if (auto l = std::dynamic_pointer_cast<atom::List>(s)) {
                            l->items.push_back(val);
                        } else if (auto v = std::dynamic_pointer_cast<atom::Vec>(s)) {
                            v->items.push_back(val);
                        } else {
                            throwError(pos, "push to unexpected seq type");
                        }
                    } else {
                        // seq is expected frame to contain seq, but
                        // we'll ignore if if not (may want to log or throw
                        // at runtime)
                    }
                },
                seq->p);
    }

    atom::patom seq;
    bool quoteNext = false;
};

static atom::patom quoteAtom(atom::patom val)
{
    // todo: can partially centralize quote, like Nil, at least here
    return atom::List::make_atom({atom::SymName::make_atom("quote"sv),
            val});
}

struct Handler2
{
    Handler2(std::string_view src, std::string_view name) :
        src(src),
        m_pos{0, name}
    {
    }

    atom::patom findAtom();

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

    using StateAtom = std::pair<State, atom::patom>;

    [[noreturn]] void throwError(const char* msg)
    {
        ::throwError(m_pos, msg);
    }

    [[noreturn]] void throwError(const std::string& msg)
    {
        ::throwError(m_pos, msg);
    }

    StateAtom handle(Normal&& state, char ch)
    {
        atom::patom res;

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
                    res = emit(Tk::sym);
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
                return {state, emit(Tk::sym)};

            // brackets, curly brackets, and parens are all tokens,
            // as well as separators. emit any symbol we might have,
            // and emit the token next round
            case '[':
                m_stack.emplace_back(atom::Vec::make_atom());
                break;
            case ']':
                res = sep_emit(ch, Tk::rbracket);
                break;

            case '{':
                m_stack.emplace_back(atom::Map::make_atom());
                break;
            case '}':
                res = sep_emit(ch, Tk::rcurly);
                break;

            case '(':
                m_stack.emplace_back(atom::List::make_atom());
                break;
            case ')':
                res = sep_emit(ch, Tk::rparen);
                break;

            // quote is a token and a symbol.
            case '\'':
                res = sep_emit(ch, Tk::quote);
                break;

            // backtick is a separator, so we may need
            // to emit a symbol before emitting the
            // symbol for the backtick itself. the rest
            // of the chars here just emit their symbol.
            case '`':
                if (res = sep_emit(ch); res) {
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
                return {state, emit(Tk::sym)};

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
                return {Normal{}, emit(Tk::chr)};
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
                return {Normal{}, emit(Tk::chr)};
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
                return {Normal{}, emit(Tk::chr)};
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
            return {Normal{}, emit(Tk::chr)};
        } catch (std::out_of_range&) {
            throwError("unsupported character");
        }

        // really, should never get here
        assert(false);
        return {state, {}};
    }

    StateAtom handle(String&& state, char ch)
    {
        if (ch == '"') {
            // end of string
            return {Normal{}, emit(Tk::str)};
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
            return {Normal{}, emit(Tk::keyword)};
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
            return {Normal{}, emit(Tk::keyword)};
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
            return {Normal{}, emit(Tk::num)};
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
            return {Normal{}, emit(Tk::num)};
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

    // emit a single token
    [[nodiscard]] atom::patom emit(Tk t)
    {
        Token tk{t, std::string(build.data(), build.size())};
        build.clear();

        // cheating?
        if (tk.s == "nil"sv) {
            tk.t = Tk::nil;
        }

        return handleToken(tk);
    }

    // emit a symbol if one has accumulated
    [[nodiscard]] atom::patom sep_emit(char ch)
    {
        // this is called for any separator char
        // to emit any accumulated symbol
        if (!build.empty()) {
            if (auto atom = emit(Tk::sym); atom) {
                unget_next(ch);
                return atom;
            }
        }
        return {};
    }

    // emit a symbol if one has accumulated, or emit a
    // token if not (this can be called twice, to flush
    // then emit some single char token)
    [[nodiscard]] atom::patom sep_emit(char ch, Tk t)
    {
        if (auto atom = sep_emit(ch); atom) {
            return atom;
        } else {
            build.push_back(ch);
            return emit(t);
        }
    }

    ReaderFrame* topFrame()
    {
        if (!m_stack.empty()) {
            return &m_stack.back();
        }

        return nullptr;
    }

    atom::patom pushAtom(atom::patom atom)
    {
        auto frame = topFrame();
        if (frame) {
            // wrap in quote if needed
            if (frame->quoteNext) {
                atom = quoteAtom(atom);
                frame->quoteNext = false;
            }

            frame->push(m_pos, atom);
            return {};
        } else {
            if (m_quoteNext) {
                atom = quoteAtom(atom);
                m_quoteNext = false;
            }

            return atom;
        }
    }

    atom::patom endFrame()
    {
        // TODO: error on map with leftover keys!

        switch (m_stack.size()) {
            case 0:
                throwError("unhandled empty stack on frame end");
            case 1: {
                auto frame = topFrame();
                auto seq = frame->seq;
                if (m_quoteNext) {
                    seq = quoteAtom(seq);
                    m_quoteNext = false;
                } else if (frame->quoteNext) {
                    throwError("unexpected quote");
                }

                // clear stack
                m_stack.clear();
                return seq;
            }
            default: // > 1
            {
                auto frame = topFrame();
                auto seq = frame->seq;

                // pop it
                m_stack.pop_back();

                frame = topFrame();
                // prefix quote if needed
                if (frame->quoteNext) {
                    seq = quoteAtom(seq);
                    frame->quoteNext = false;
                }

                frame->push(m_pos, seq);
            }
        }

        return {};
    }

    atom::patom handleToken(Token t)
    {
        switch (t.t) {
            case Tk::rparen:
                [[fallthrough]];
            case Tk::rbracket:
                [[fallthrough]];
            case Tk::rcurly:
                return endFrame();
            case Tk::sym:
                return pushAtom(atom::SymName::make_atom(t.s));
            case Tk::str:
                return pushAtom(atom::Str::make_atom(t.s));
            case Tk::keyword:
                return pushAtom(atom::Keyword::make_atom(t.s));
            case Tk::chr:
                // we know it's a slash + whatever text, min 2 chars
                if (t.s.size() == 1) {
                    return pushAtom(atom::Char::make_atom(t.s.front()));
                } else if (t.s.size() == 4 && t.s.front() == 'o') {
                    int base = 8;

                    int i = 0;
                    auto [p, _] = std::from_chars(
                            t.s.data() + 1, t.s.data() + t.s.size(), i, base);
                    if (p != t.s.data()) {
                        return pushAtom(atom::Char::make_atom(static_cast<char32_t>(i)));
                    } else {
                        throwError("unable to parse octal char");
                    }
                } else if (t.s.size() == 5 && t.s.front() == 'u') {
                    int base = 16;

                    int i = 0;
                    auto [p, _] = std::from_chars(
                            t.s.data() + 1, t.s.data() + t.s.size(), i, base);
                    if (p != t.s.data()) {
                        return pushAtom(atom::Char::make_atom(static_cast<char32_t>(i)));
                    } else {
                        throwError("unable to parse unicode char");
                    }
                } else {
                    throwError("unexpected character size");
                }
                break;
            case Tk::nil:
                return pushAtom(atom::Nil);
            case Tk::num: {
                // note: only handling base 10 and 16
                int base = 10;
                std::size_t off = 0;
                if (t.s.size() > 2) {
                    // todo: in c++20, starts_with
                    auto prefix = t.s.substr(0, 2);
                    if (prefix == "0x"sv || prefix == "0X"sv) {
                        base = 16;
                        off = 2;
                    }
                }
                int i = 0;
                auto [p, _] = std::from_chars(
                        t.s.data() + off, t.s.data() + t.s.size(), i, base);
                if (p != t.s.data()) {
                    return pushAtom(atom::Num::make_atom(i));
                } else {
                    throwError(
                            fmt::format(
                                    "invalid numeric format for '{}'", t.s));
                }
                break;
            }
            case Tk::quote: {
                auto frame = topFrame();
                if (frame) {
                    frame->quoteNext = true;
                } else {
                    m_quoteNext = true;
                }
                break;
            }
            default:
                throwError(fmt::format("unexpected token in read {}", t.t));
        }

        return {};
    }

    std::string_view src;
    Position m_pos;

    std::optional<char> buf;
    std::vector<char> build;

    State state;

    std::vector<ReaderFrame> m_stack;
    bool m_quoteNext = false;
};

atom::patom Handler2::findAtom()
{
    atom::patom value;

    while (!src.empty() || buf) {
        char ch = get_next();
        std::tie(state, value) = std::visit(
                [this, ch](auto&& state) -> StateAtom {
                    return handle(std::move(state), ch);
                },
                state);

        if (value) {
            return value;
        }
    }

    // stuff left to emit?
    if (!build.empty()) {
        return std::visit(
                [this](auto&& state) -> atom::patom {
                    using T = std::decay_t<decltype(state)>;
                    std::string str{build.data(), build.size()};
                    build.clear();

                    if constexpr (std::is_same_v<T, Normal>) {
                        // cheating?
                        if (str == "nil"sv) {
                            return handleToken({Tk::nil, str});
                        } else {
                            return handleToken({Tk::sym, str});
                        }
                    } else if constexpr (std::is_same_v<T, Char>) {
                        // char finishes itself or throws. if we
                        // have chars left, something bad has happened
                        assert(false);
                        throwError("unsupported character");
                    } else if constexpr (std::is_same_v<T, String>) {
                        return handleToken({Tk::str, str});
                    } else if constexpr (std::is_same_v<T, Comment>) {
                        // we shouldn't accumulate comments
                        assert(str.empty());
                        return handleToken({Tk::none});
                    } else if constexpr (
                            std::is_same_v<T, Keyword> ||
                            std::is_same_v<T, NamespacedKeyword>) {
                        return handleToken({Tk::keyword, str});
                    } else if constexpr (
                            std::is_same_v<T, Number> ||
                            std::is_same_v<T, DecimalNumber> ||
                            std::is_same_v<T, IntegerNumber>) {
                        return handleToken({Tk::num, str});
                    }
                },
                state);
    }

    return {};
}

class ReaderImpl : public Reader
{
public:
    ReaderImpl(std::string_view str,
            std::string_view name) :
        m_p(createParser(createTokenizer(str, "internal-test")))
    {}

    bool next()
    {
        return m_p->next();
    }

    atom::patom value()
    {
        return m_p->value();
    }

private:
    std::shared_ptr<Parser> m_p;
};

class ReaderImpl2 : public Reader
{
public:
    ReaderImpl2(std::string_view str,
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

    atom::patom value()
    {
        return m_value;
    }

private:
    Handler2 handler;
    bool first = true;
    atom::patom m_value;
};

std::shared_ptr<Reader> createReader(std::string_view str,
        std::string_view name)
{
    return std::make_shared<ReaderImpl>(str, name);
}

// todo: hack
std::shared_ptr<Reader> createReader2(std::string_view str,
        std::string_view name)
{
    return std::make_shared<ReaderImpl2>(str, name);
}
