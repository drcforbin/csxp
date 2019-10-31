#include "catch.hpp"
#include "reader.h"

#include <string_view>

// todo: map parsing tests

extern const std::string_view arbitrary_clj;
extern const std::string_view arbitrary_long_clj;

using namespace std::literals;

atom::patom testReadToAtom(std::string_view str)
{
    // todo: switch to 'reader'?

    auto r = createReader(str, "test-file"sv);

    // should be able to get first atom
    REQUIRE(r->next());

    auto val = r->value();

    // should be done now
    REQUIRE(!r->next());

    return val;
}

std::vector<atom::patom> testReadToAtoms(std::string_view str)
{
    std::vector<atom::patom> vec;
    for (auto val : reader(str, "test-file"sv)) {
        vec.push_back(val);
    }

    return vec;
}

TEST_CASE("can read keyword", "[reader]")
{
    // todo: test namespaced keyword

    auto str = ":keyword"sv;
    auto val = testReadToAtom(str);

    // should just return a single keyword
    auto k = std::get<std::shared_ptr<atom::Keyword>>(val->p);
    REQUIRE(k); // make sure not null or anything
    REQUIRE(k->name == ":keyword"sv);

    /*
    todo: test
        none,
        lbracket,
        rbracket,
        lcurly,
        rcurly,
        sym,
        str,
        chr,
        nil,
        quote
	*/
}

TEST_CASE("can read list", "[reader]")
{
    auto str = "(:abc)"sv;
    auto val = testReadToAtom(str);

    // should be a list containing the keyword abc

    auto s = std::get<std::shared_ptr<atom::Seq>>(val->p);
    REQUIRE(s);

    auto lst = std::dynamic_pointer_cast<atom::List>(s);
    REQUIRE(lst);
    REQUIRE(lst->items.size() == 1);

    auto k = std::get<std::shared_ptr<atom::Keyword>>(lst->items[0]->p);
    REQUIRE(k);
    REQUIRE(k->name == ":abc"sv);
}

TEST_CASE("can read vec", "[reader]")
{
    auto str = "[:abc]"sv;
    auto val = testReadToAtom(str);

    // should be a vec containing the keyword abc

    auto s = std::get<std::shared_ptr<atom::Seq>>(val->p);
    REQUIRE(s);

    auto vec = std::dynamic_pointer_cast<atom::Vec>(s);
    REQUIRE(vec);
    REQUIRE(vec->items.size() == 1);

    auto k = std::get<std::shared_ptr<atom::Keyword>>(vec->items[0]->p);
    REQUIRE(k);
    REQUIRE(k->name == ":abc"sv);
}

TEST_CASE("can read quote", "[reader]"){
        SECTION("single quoted sym"){
                auto str = "'L"sv;
auto val = testReadToAtom(str);

// a quoted thing should be a list containing the
// quote symbol and the thing that's quoted, in this
// case, the symbol L

auto s = std::get<std::shared_ptr<atom::Seq>>(val->p);
REQUIRE(s);

auto lst = std::dynamic_pointer_cast<atom::List>(s);
REQUIRE(lst);
REQUIRE(lst->items.size() == 2);

auto sym = std::get<std::shared_ptr<atom::SymName>>(lst->items[0]->p);
REQUIRE(sym);
REQUIRE(sym->name == "quote"sv);

sym = std::get<std::shared_ptr<atom::SymName>>(lst->items[1]->p);
REQUIRE(sym);
REQUIRE(sym->name == "L"sv);
}

SECTION("vec with quoted syms")
{
    auto str = "['L 'M]"sv;
    auto val = testReadToAtom(str);

    // a quoted thing should be a vec containing two lists,
    // each with a quote symbol and the thing that's quoted, in this
    // case, the symbols L and M

    // check outer vec

    auto s = std::get<std::shared_ptr<atom::Seq>>(val->p);
    REQUIRE(s);

    auto vec = std::dynamic_pointer_cast<atom::Vec>(s);
    REQUIRE(vec);
    REQUIRE(vec->items.size() == 2);

    // check first list

    auto q = std::get<std::shared_ptr<atom::Seq>>(vec->items[0]->p);
    REQUIRE(q);

    auto qlst = std::dynamic_pointer_cast<atom::List>(q);
    REQUIRE(qlst);
    REQUIRE(qlst->items.size() == 2);

    auto sym = std::get<std::shared_ptr<atom::SymName>>(qlst->items[0]->p);
    REQUIRE(sym);
    REQUIRE(sym->name == "quote"sv);

    sym = std::get<std::shared_ptr<atom::SymName>>(qlst->items[1]->p);
    REQUIRE(sym);
    REQUIRE(sym->name == "L"sv);

    // check second list

    q = std::get<std::shared_ptr<atom::Seq>>(vec->items[1]->p);
    REQUIRE(q);

    qlst = std::dynamic_pointer_cast<atom::List>(q);
    REQUIRE(qlst);
    REQUIRE(qlst->items.size() == 2);

    sym = std::get<std::shared_ptr<atom::SymName>>(qlst->items[0]->p);
    REQUIRE(sym);
    REQUIRE(sym->name == "quote"sv);

    sym = std::get<std::shared_ptr<atom::SymName>>(qlst->items[1]->p);
    REQUIRE(sym);
    REQUIRE(sym->name == "M"sv);
}
}
;

TEST_CASE("can read chars", "[reader]")
{
    auto str = R"-(
            \a \b \2 \3
            \newline \space \tab \formfeed \backspace \return
            \u03A9 \o777
            )-"sv;

    auto vec = testReadToAtoms(str);

    REQUIRE(vec.size() == 12);

    auto chr = std::get<std::shared_ptr<atom::Char>>(vec[0]->p);
    REQUIRE(chr->val == 'a');

    chr = std::get<std::shared_ptr<atom::Char>>(vec[1]->p);
    REQUIRE(chr->val == 'b');

    chr = std::get<std::shared_ptr<atom::Char>>(vec[2]->p);
    REQUIRE(chr->val == '2');

    chr = std::get<std::shared_ptr<atom::Char>>(vec[3]->p);
    REQUIRE(chr->val == '3');

    chr = std::get<std::shared_ptr<atom::Char>>(vec[4]->p);
    REQUIRE(chr->val == '\n');

    chr = std::get<std::shared_ptr<atom::Char>>(vec[5]->p);
    REQUIRE(chr->val == ' ');

    chr = std::get<std::shared_ptr<atom::Char>>(vec[6]->p);
    REQUIRE(chr->val == '\t');

    chr = std::get<std::shared_ptr<atom::Char>>(vec[7]->p);
    REQUIRE(chr->val == '\f');

    chr = std::get<std::shared_ptr<atom::Char>>(vec[8]->p);
    REQUIRE(chr->val == '\b');

    chr = std::get<std::shared_ptr<atom::Char>>(vec[9]->p);
    REQUIRE(chr->val == '\r');

    chr = std::get<std::shared_ptr<atom::Char>>(vec[10]->p);
    REQUIRE(chr->val == 0x03A9);

    chr = std::get<std::shared_ptr<atom::Char>>(vec[11]->p);
    REQUIRE(chr->val == 0777);
}

TEST_CASE("can read nums", "[reader]")
{
    // todo: floating points
    // including 1.23 12e3 1.2e3 1.2e-3
    // todo: test arbitrary NrXXX where (<= 2 N 36) and X is in [0-9,A-Z]
    // including binary 2r0110
    // todo: consider how to handle 32/24/16/8 bit

    auto str = R"-(
            10 -1 34226 -30920
            0XFFFF 0xFFFF 0xF0 0xffff 0xf0
            0x00 0x0 0
            )-"sv;

    auto vec = testReadToAtoms(str);

    REQUIRE(vec.size() == 12);

    auto num = std::get<std::shared_ptr<atom::Num>>(vec[0]->p);
    REQUIRE(num->val == 10);

    num = std::get<std::shared_ptr<atom::Num>>(vec[1]->p);
    REQUIRE(num->val == -1);

    num = std::get<std::shared_ptr<atom::Num>>(vec[2]->p);
    REQUIRE(num->val == 34226);

    num = std::get<std::shared_ptr<atom::Num>>(vec[3]->p);
    REQUIRE(num->val == -30920);

    num = std::get<std::shared_ptr<atom::Num>>(vec[4]->p);
    REQUIRE(num->val == 0xFFFF);

    num = std::get<std::shared_ptr<atom::Num>>(vec[5]->p);
    REQUIRE(num->val == 0xFFFF);

    num = std::get<std::shared_ptr<atom::Num>>(vec[6]->p);
    REQUIRE(num->val == 0xF0);

    num = std::get<std::shared_ptr<atom::Num>>(vec[7]->p);
    REQUIRE(num->val == 0xFFFF);

    num = std::get<std::shared_ptr<atom::Num>>(vec[8]->p);
    REQUIRE(num->val == 0xF0);

    num = std::get<std::shared_ptr<atom::Num>>(vec[9]->p);
    REQUIRE(num->val == 0);

    num = std::get<std::shared_ptr<atom::Num>>(vec[10]->p);
    REQUIRE(num->val == 0);

    num = std::get<std::shared_ptr<atom::Num>>(vec[11]->p);
    REQUIRE(num->val == 0);
}
