#include "doctest.h"
#include "csxp/reader.h"

// todo: remove
#include "csxp/atom_fmt.h"
#include "csxp/logging.h"

#include <string_view>

// todo: map parsing tests

extern const std::string_view arbitrary_clj;
extern const std::string_view arbitrary_long_clj;

using namespace std::literals;

csxp::patom testReadToAtom(std::string_view str)
{
    // todo: switch to 'reader'?

    auto r = csxp::createReader(str, "test-file"sv);

    // should be able to get first atom
    REQUIRE(r->next());

    auto val = r->value();

    // should be done now
    REQUIRE(!r->next());

    return val;
}

std::vector<csxp::patom> testReadToAtoms(std::string_view str)
{
    std::vector<csxp::patom> vec;
    for (auto val : csxp::reader(str, "test-file"sv)) {
        vec.push_back(val);
    }

    return vec;
}

TEST_SUITE_BEGIN("reader");

TEST_CASE("can read keyword")
{
    // todo: test namespaced keyword

    auto str = ":keyword"sv;
    auto val = testReadToAtom(str);

    // should just return a single keyword
    auto k = csxp::get<csxp::Keyword>(val);
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

TEST_CASE("can read list")
{
    auto str = "(:abc)"sv;
    auto val = testReadToAtom(str);

    // should be a list containing the keyword abc

    auto s = csxp::get<csxp::Seq>(val);
    REQUIRE(s);

    auto lst = std::dynamic_pointer_cast<csxp::List>(s);
    REQUIRE(lst);
    REQUIRE(lst->items.size() == 1);

    auto k = csxp::get<csxp::Keyword>(lst->items[0]);
    REQUIRE(k);
    REQUIRE(k->name == ":abc"sv);
}

TEST_CASE("can read vec")
{
    auto str = "[:abc]"sv;
    auto val = testReadToAtom(str);

    // should be a vec containing the keyword abc

    auto s = csxp::get<csxp::Seq>(val);
    REQUIRE(s);

    auto vec = std::dynamic_pointer_cast<csxp::Vec>(s);
    REQUIRE(vec);
    REQUIRE(vec->items.size() == 1);

    auto k = csxp::get<csxp::Keyword>(vec->items[0]);
    REQUIRE(k);
    REQUIRE(k->name == ":abc"sv);
}

TEST_CASE("can read quote")
{
    SUBCASE("single quoted sym")
    {
        auto str = "'L"sv;
        auto val = testReadToAtom(str);

        // a quoted thing should be a list containing the
        // quote symbol and the thing that's quoted, in this
        // case, the symbol L

        auto s = csxp::get<csxp::Seq>(val);
        REQUIRE(s);

        auto lst = std::dynamic_pointer_cast<csxp::List>(s);
        REQUIRE(lst);
        REQUIRE(lst->items.size() == 2);

        auto sym = csxp::get<csxp::SymName>(lst->items[0]);
        REQUIRE(sym);
        REQUIRE(sym->name == "quote"sv);

        sym = csxp::get<csxp::SymName>(lst->items[1]);
        REQUIRE(sym);
        REQUIRE(sym->name == "L"sv);
    }

    SUBCASE("vec with quoted syms")
    {
        auto str = "['L 'M]"sv;
        auto val = testReadToAtom(str);

        // a quoted thing should be a vec containing two lists,
        // each with a quote symbol and the thing that's quoted, in this
        // case, the symbols L and M

        // check outer vec

        auto s = csxp::get<csxp::Seq>(val);
        REQUIRE(s);

        auto vec = std::dynamic_pointer_cast<csxp::Vec>(s);
        REQUIRE(vec);
        REQUIRE(vec->items.size() == 2);

        // check first list

        auto q = csxp::get<csxp::Seq>(vec->items[0]);
        REQUIRE(q);

        auto qlst = std::dynamic_pointer_cast<csxp::List>(q);
        REQUIRE(qlst);
        REQUIRE(qlst->items.size() == 2);

        auto sym = csxp::get<csxp::SymName>(qlst->items[0]);
        REQUIRE(sym);
        REQUIRE(sym->name == "quote"sv);

        sym = csxp::get<csxp::SymName>(qlst->items[1]);
        REQUIRE(sym);
        REQUIRE(sym->name == "L"sv);

        // check second list

        q = csxp::get<csxp::Seq>(vec->items[1]);
        REQUIRE(q);

        qlst = std::dynamic_pointer_cast<csxp::List>(q);
        REQUIRE(qlst);
        REQUIRE(qlst->items.size() == 2);

        sym = csxp::get<csxp::SymName>(qlst->items[0]);
        REQUIRE(sym);
        REQUIRE(sym->name == "quote"sv);

        sym = csxp::get<csxp::SymName>(qlst->items[1]);
        REQUIRE(sym);
        REQUIRE(sym->name == "M"sv);
    }

    SUBCASE("quoted seq with quoted seq")
    {
        auto str = "'('(1 2) 3 4)"sv;
        auto val = testReadToAtom(str);

        // check outer seq (s0). should be quote and inner seq

        auto s0 = csxp::get<csxp::Seq>(val);
        auto it0 = s0->iterator();

        REQUIRE(it0->next());
        auto sym = csxp::get<csxp::SymName>(it0->value());
        REQUIRE(sym->name == "quote"sv);

        REQUIRE(it0->next());
        auto s1 = csxp::get<csxp::Seq>(it0->value());
        auto it1 = s1->iterator();

        REQUIRE(!it0->next());

        // check inner seq (s1). should be a seq containing
        // an inner seq (s2), followed by 3 and 4

        REQUIRE(it1->next());
        auto s2 = csxp::get<csxp::Seq>(it1->value());
        auto it2 = s2->iterator();

        REQUIRE(it1->next());
        auto num = csxp::get<csxp::Num>(it1->value());
        REQUIRE(num->val == 3);

        REQUIRE(it1->next());
        num = csxp::get<csxp::Num>(it1->value());
        REQUIRE(num->val == 4);

        REQUIRE(!it1->next());

        // s2 should contain quote and an inner seq (s3)

        REQUIRE(it2->next());
        sym = csxp::get<csxp::SymName>(it2->value());
        REQUIRE(sym->name == "quote"sv);

        REQUIRE(it2->next());
        auto s3 = csxp::get<csxp::Seq>(it2->value());
        auto it3 = s3->iterator();

        REQUIRE(!it2->next());

        // s3 should contain 1 and 2

        REQUIRE(it3->next());
        num = csxp::get<csxp::Num>(it3->value());
        REQUIRE(num->val == 1);

        REQUIRE(it3->next());
        num = csxp::get<csxp::Num>(it3->value());
        REQUIRE(num->val == 2);

        REQUIRE(!it3->next());
    }
}

TEST_CASE("can read chars")
{
    auto str = R"-(
            \a \b \2 \3
            \newline \space \tab \formfeed \backspace \return
            \u03A9 \o777
            )-"sv;

    auto vec = testReadToAtoms(str);

    REQUIRE(vec.size() == 12);

    auto chr = csxp::get<csxp::Char>(vec[0]);
    REQUIRE(chr->val == 'a');

    chr = csxp::get<csxp::Char>(vec[1]);
    REQUIRE(chr->val == 'b');

    chr = csxp::get<csxp::Char>(vec[2]);
    REQUIRE(chr->val == '2');

    chr = csxp::get<csxp::Char>(vec[3]);
    REQUIRE(chr->val == '3');

    chr = csxp::get<csxp::Char>(vec[4]);
    REQUIRE(chr->val == '\n');

    chr = csxp::get<csxp::Char>(vec[5]);
    REQUIRE(chr->val == ' ');

    chr = csxp::get<csxp::Char>(vec[6]);
    REQUIRE(chr->val == '\t');

    chr = csxp::get<csxp::Char>(vec[7]);
    REQUIRE(chr->val == '\f');

    chr = csxp::get<csxp::Char>(vec[8]);
    REQUIRE(chr->val == '\b');

    chr = csxp::get<csxp::Char>(vec[9]);
    REQUIRE(chr->val == '\r');

    chr = csxp::get<csxp::Char>(vec[10]);
    REQUIRE(chr->val == 0x03A9);

    chr = csxp::get<csxp::Char>(vec[11]);
    REQUIRE(chr->val == 0777);
}

TEST_CASE("can read nums")
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

    auto num = csxp::get<csxp::Num>(vec[0]);
    REQUIRE(num->val == 10);

    num = csxp::get<csxp::Num>(vec[1]);
    REQUIRE(num->val == -1);

    num = csxp::get<csxp::Num>(vec[2]);
    REQUIRE(num->val == 34226);

    num = csxp::get<csxp::Num>(vec[3]);
    REQUIRE(num->val == -30920);

    num = csxp::get<csxp::Num>(vec[4]);
    REQUIRE(num->val == 0xFFFF);

    num = csxp::get<csxp::Num>(vec[5]);
    REQUIRE(num->val == 0xFFFF);

    num = csxp::get<csxp::Num>(vec[6]);
    REQUIRE(num->val == 0xF0);

    num = csxp::get<csxp::Num>(vec[7]);
    REQUIRE(num->val == 0xFFFF);

    num = csxp::get<csxp::Num>(vec[8]);
    REQUIRE(num->val == 0xF0);

    num = csxp::get<csxp::Num>(vec[9]);
    REQUIRE(num->val == 0);

    num = csxp::get<csxp::Num>(vec[10]);
    REQUIRE(num->val == 0);

    num = csxp::get<csxp::Num>(vec[11]);
    REQUIRE(num->val == 0);
}

/*
// todo: other reader macros (see reader code)
TEST_CASE("skip next form reader macro")
{
    // todo: make sure symbols can include #

    // the array should not include any of the commented forms
    auto str = "[:a #_\\m #_ \\h #_:b #_ :n 2 #_(f :b) #_ (g 7) #_1 #_ 3 :z]"sv;
    auto val = testReadToAtom(str);

    logging::dbg()->info("got {}", *val);

    auto vec = csxp::get<csxp::Vec>(val);
    REQUIRE(vec->items.size() == 3);

    auto kw = csxp::get<csxp::Keyword>(vec->items[0]);
    REQUIRE(kw->name == "a"sv);

    auto num = csxp::get<csxp::Num>(vec->items[1]);
    REQUIRE(num->val == 0);

    kw = csxp::get<csxp::Keyword>(vec->items[2]);
    REQUIRE(kw->name == "z"sv);
}
*/

TEST_SUITE_END();
