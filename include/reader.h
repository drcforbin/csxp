#ifndef READER_H
#define READER_H

#include "atom.h"
#include "tokenizer.h"

#include <stdexcept>

struct Reader
{
    virtual ~Reader() = default;

    virtual bool next() = 0;
    virtual atom::patom value() = 0;

    // todo: implement const iterator too
    class ReaderIt
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

        explicit ReaderIt(Reader* p) :
            p(p) {}
        atom::patom operator*() const { return curr; }
        const atom::patom* operator->() const { return &curr; }
        bool operator==(const ReaderIt& other) const { return p == other.p; }
        bool operator!=(const ReaderIt& other) const { return !(*this == other); }

        AtomHolder operator++(int)
        {
            AtomHolder ret(curr);
            ++*this;
            return ret;
        }

        ReaderIt& operator++()
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
        Reader* p;
        atom::patom curr;
    };

    ReaderIt begin()
    {
        auto it = ReaderIt(this);
        return ++it;
    }
    ReaderIt end() { return ReaderIt(nullptr); }
};

// note: reader does not take ownership of or copy str or name.
// both must exist for the lifetime of the reader
std::shared_ptr<Reader> createReader(std::string_view str,
        std::string_view name);

// note: reader does not take ownership of or copy str or name.
// both must exist for the lifetime of the reader
// todo: hack
std::shared_ptr<Reader> createReader2(std::string_view str,
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
        p(createReader2(str, name)) {}
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
