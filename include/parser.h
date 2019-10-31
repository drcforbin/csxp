#ifndef PARSER_H
#define PARSER_H

#include "atom.h"
#include "tokenizer.h"

#include <stdexcept>
#include <string>

class ParserError : public std::runtime_error
{
public:
    explicit ParserError(Position pos, const std::string& what_arg) :
        std::runtime_error(what_arg),
        m_pos(pos)
    {}

    explicit ParserError(Position pos, const char* what_arg) :
        std::runtime_error(what_arg),
        m_pos(pos)
    {}

    Position position() const noexcept { return m_pos; }

private:
    Position m_pos;
};

struct Parser
{
    virtual ~Parser() = default;

    virtual bool next() = 0;
    virtual atom::patom value() = 0;

    // todo: implement const iterator too
    class ParserIt
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

        explicit ParserIt(Parser* p) :
            p(p) {}
        atom::patom operator*() const { return curr; }
        const atom::patom* operator->() const { return &curr; }
        bool operator==(const ParserIt& other) const { return p == other.p; }
        bool operator!=(const ParserIt& other) const { return !(*this == other); }

        AtomHolder operator++(int)
        {
            AtomHolder ret(curr);
            ++*this;
            return ret;
        }

        ParserIt& operator++()
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
        Parser* p;
        atom::patom curr;
    };

    ParserIt begin()
    {
        auto it = ParserIt(this);
        return ++it;
    }
    ParserIt end() { return ParserIt(nullptr); }
};

// note: parser does not take ownership of or copy str or name.
// both must exist for the lifetime of the reader
std::shared_ptr<Parser> createParser(std::shared_ptr<Tokenizer> tz);

#endif // PARSER_H
