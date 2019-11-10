#ifndef CSXP_READER_H
#define CSXP_READER_H

#include "csxp/atom.h"

#include <stdexcept>

namespace csxp {

struct Position
{
    int line = 0;
    std::string name;
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
    virtual patom value() = 0;
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

    reader_iterator(std::string_view str, std::string_view name) :
        p(createReader(str, name)) {}
    reader_iterator() {}

    patom operator*() const { return curr; }
    const patom* operator->() const { return &curr; }
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
    patom curr;
};

// note: reader does not take ownership of or copy str or name.
// both must exist for the lifetime of the reader
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

} // namespace csxp

#endif // CSXP_READER_H
