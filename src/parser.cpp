#include "parser.h"

#include <charconv>
#include <typeinfo> // todo: remove
#include <vector>

// todo: remove
#include "logging.h"
#define LOGGER() (logging::get("parser"))

using namespace std::literals;

// special value indicating that we have not
// yet found a token
const auto noToken = atom::Const::make_atom("notoken"sv);

// TODO: macros
// TODO: maps

/*
// helper type for visitor
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;
*/

struct ParseFrame
{
    ParseFrame(atom::patom seq) :
        seq(seq)
    {
    }

    void push(const Position& pos, atom::patom val)
    {
        // todo: need to check whether seq is valid?
        std::visit(
                // todo: this not needed
                [this, &pos, &val](auto&& s) {
                    using T = std::decay_t<decltype(s)>;
                    if constexpr (std::is_same_v<T, std::shared_ptr<atom::Seq>>) {
                        if (auto l = std::dynamic_pointer_cast<atom::List>(s)) {
                            l->items.push_back(val);
                        } else if (auto v = std::dynamic_pointer_cast<atom::Vec>(s)) {
                            v->items.push_back(val);
                        } else {
                            throw ParserError(pos,
                                    "push to unexpected seq type");
                        }
                    } else {
                        // seq is expected frame to contain seq, but
                        // we'll ignore if if not (may want to log or throw
                        // at runtime)
                        //
                        // todo: decide on data structure
                        // may want to capture a key, then value, when val
                        // is the data structure thing
                    }
                },
                seq->p);
    }

    atom::patom seq;
    bool quoteNext = false;
};

// TODO: rename quoteVal
static atom::patom quoteAtom(atom::patom val)
{
    // todo: can centralize quote, like Nil
    return atom::List::make_atom({atom::SymName::make_atom("quote"sv),
            val});
}

class ParserImpl : public Parser
{
public:
    ParserImpl(std::shared_ptr<Tokenizer> tz) :
        m_tz(tz)
    {}

    bool next()
    {
        while (m_tz->next()) {
            handleToken(m_tz->value());

            // TODO: stop on EOF?
            if (m_value != noToken) {
                return true;
            }
        }

        return false;
    }

    atom::patom value()
    {
        // todo: this side effect isn't cool.
        auto val = m_value;
        m_value = noToken;
        return val;
    }

private:
    ParseFrame* topFrame()
    {
        if (!m_stack.empty()) {
            return &m_stack.back();
        }

        return nullptr;
    }

    void pushAtom(atom::patom atom)
    {
        auto frame = topFrame();
        if (frame) {
            // wrap in quote if needed
            if (frame->quoteNext) {
                atom = quoteAtom(atom);
                frame->quoteNext = false;
            }

            frame->push(m_tz->position(), atom);
        } else {
            if (m_quoteNext) {
                atom = quoteAtom(atom);
                m_quoteNext = false;
            }

            m_value = atom;
        }
    }

    void endFrame()
    {
        // TODO: error on map with leftover keys!

        switch (m_stack.size()) {
            case 0:
                throw ParserError(m_tz->position(),
                        "unhandled empty stack on RParen");
            case 1: {
                auto frame = topFrame();
                auto seq = frame->seq;
                if (m_quoteNext) {
                    seq = quoteAtom(seq);
                    m_quoteNext = false;
                } else if (frame->quoteNext) {
                    // TODO: error
                }
                m_value = seq;

                // clear stack
                m_stack.clear();
                break;
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

                frame->push(m_tz->position(), seq);
            }
        }
    }

    void handleToken(Token t)
    {
        switch (t.t) {
            case Tk::lparen:
                m_stack.emplace_back(atom::List::make_atom());
                break;
            case Tk::lbracket:
                m_stack.emplace_back(atom::Vec::make_atom());
                break;
            case Tk::lcurly:
                // todo: this is a hack...some kind of data structure?
                m_stack.emplace_back(atom::Vec::make_atom());
                break;
            case Tk::rparen:
                [[fallthrough]];
            case Tk::rbracket:
                [[fallthrough]];
            case Tk::rcurly:
                endFrame();
                break;
            case Tk::sym:
                pushAtom(atom::SymName::make_atom(t.s));
                break;
            case Tk::str:
                pushAtom(atom::Str::make_atom(t.s));
                break;
            case Tk::keyword:
                pushAtom(atom::Keyword::make_atom(t.s));
                break;
            case Tk::chr:
                // we know it's a slash + whatever text, min 2 chars
                if (t.s.size() == 2) {
                    pushAtom(atom::Char::make_atom(t.s[1]));
                } else if (t.s.size() > 2) {
                    if (t.s == "\\newline"sv) {
                        pushAtom(atom::Char::make_atom('\n'));
                    } else if (t.s == "\\space"sv) {
                        pushAtom(atom::Char::make_atom(' '));
                    } else if (t.s == "\\tab"sv) {
                        pushAtom(atom::Char::make_atom('\t'));
                    } else if (t.s == "\\formfeed"sv) {
                        pushAtom(atom::Char::make_atom('\f'));
                    } else if (t.s == "\\backspace"sv) {
                        pushAtom(atom::Char::make_atom('\b'));
                    } else if (t.s == "\\return"sv) {
                        pushAtom(atom::Char::make_atom('\r'));
                    } else {
                        // todo: in c++20, starts_with
                        auto prefix = t.s.substr(0, 2);
                        int base = 0;
                        if (prefix == "\\u"sv) {
                            base = 16;
                        } else if (prefix == "\\o"sv) {
                            base = 8;
                        } else {
                            // todo: throw unexpected
                        }

                        int i = 0;
                        auto [p, _] = std::from_chars(
                                t.s.data() + 2, t.s.data() + t.s.size(), i, base);
                        if (p != t.s.data()) {
                            pushAtom(atom::Char::make_atom(static_cast<char32_t>(i)));
                        } else {
                            // todo: throw invalid
                        }
                    }
                } else {
                    // todo: throw unexpected
                }
                break;
            case Tk::nil:
                pushAtom(atom::Nil);
                break;
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
                    pushAtom(atom::Num::make_atom(i));
                } else {
                    throw ParserError(m_tz->position(),
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
                throw ParserError(m_tz->position(),
                        fmt::format("unexpected token in parse {}", t.t));
        }
    }

    std::shared_ptr<Tokenizer> m_tz;
    std::vector<ParseFrame> m_stack;
    bool m_quoteNext = false;

    atom::patom m_value = noToken;
};

std::shared_ptr<Parser> createParser(std::shared_ptr<Tokenizer> tz)
{
    return std::make_shared<ParserImpl>(tz);
}
