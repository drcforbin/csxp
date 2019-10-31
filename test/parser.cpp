#include "catch.hpp"
#include "parser.h"
#include "tokenizer.h"

#include <string_view>

// todo: map parsing tests

extern const std::string_view arbitrary_clj;
extern const std::string_view arbitrary_long_clj;

using namespace std::literals;

struct MockTokenizer : public Tokenizer
{
    MockTokenizer(std::string_view name, std::vector<Token> tokens)
    {
        for (std::size_t i = 0; i < tokens.size(); i++) {
            auto tk = tokens[i];
            if (tk.pos.name.empty() || tk.pos.line == 0) {
                tk.pos.name = name;
                tk.pos.line = i + 1;
                this->tokens.push_back(tk);
            }
        }
    }

    Position position() const noexcept
    {
        return tokens[idx].pos;
    }

    bool next()
    {
        idx++;
        return static_cast<std::size_t>(idx) < tokens.size();
    }

    Token value() const
    {
        return tokens[idx];
    }

    int idx = -1;
    std::vector<Token> tokens;
};

atom::patom testParseToAtom(std::vector<Token> tokens)
{
    auto name = "test-file"sv;
    auto tz = std::make_shared<MockTokenizer>(name, tokens);

    auto p = createParser(tz);

    // should be able to get first token
    REQUIRE(p->next());

    auto val = p->value();

    // should be done now
    REQUIRE(!p->next());

    return val;
}

std::vector<atom::patom> testParseToAtoms(std::vector<Token> tokens)
{
    auto name = "test-file"sv;
    auto tz = std::make_shared<MockTokenizer>(name, tokens);

    auto p = createParser(tz);

    std::vector<atom::patom> vec;
    while (p->next()) {
        vec.push_back(p->value());
    }

    return vec;
}

TEST_CASE("can parse keyword", "[parser]")
{
    auto val = testParseToAtom({{Tk::keyword, ":abc"}});

    // should just return a single keyword
    auto k = std::get<std::shared_ptr<atom::Keyword>>(val->p);
    REQUIRE(k); // make sure not null or anything
    REQUIRE(k->name == ":abc"sv);
};

TEST_CASE("can parse list", "[parser]")
{
    auto val = testParseToAtom({{Tk::lparen, "("},
            {Tk::keyword, ":abc"},
            {Tk::rparen, ")"}});

    // should be a list containing the keyword abc

    auto s = std::get<std::shared_ptr<atom::Seq>>(val->p);
    REQUIRE(s);

    auto lst = std::dynamic_pointer_cast<atom::List>(s);
    REQUIRE(lst);
    REQUIRE(lst->items.size() == 1);

    auto k = std::get<std::shared_ptr<atom::Keyword>>(lst->items[0]->p);
    REQUIRE(k);
    REQUIRE(k->name == ":abc"sv);
};

TEST_CASE("can parse quote", "[parser]"){
        SECTION("single quoted sym"){
                auto val = testParseToAtom({{Tk::quote, "'"},
                        {Tk::sym, "L"}});

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
    auto val = testParseToAtom({{Tk::lbracket, "["},
            {Tk::quote, "'"},
            {Tk::sym, "L"},
            {Tk::quote, "'"},
            {Tk::sym, "L"},
            {Tk::rbracket, "]"}});

    // a quoted thing should be a vec containing two lists,
    // each with a quote symbol and the thing that's quoted, in this
    // case, the symbol L

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
    REQUIRE(sym->name == "L"sv);
}
}
;

TEST_CASE("can parse chars", "[parser]")
{
    // see "can tokenize chars" tokenizer test

    auto vec = testParseToAtoms({
            {Tk::chr, "a"},
            {Tk::chr, "b"},
            {Tk::chr, "2"},
            {Tk::chr, "3"},
            {Tk::chr, "\n"},
            {Tk::chr, " "},
            {Tk::chr, "\t"},
            {Tk::chr, "\f"},
            {Tk::chr, "\b"},
            {Tk::chr, "\r"},
            {Tk::chr, "u03A9"},
            {Tk::chr, "o777"},
    });

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
};

TEST_CASE("can parse nums", "[tokenizer]")
{
    // see "can tokenize nums" tokenize test

    // todo: floating points?
    // todo: 32/24/16/8 bit?

    auto vec = testParseToAtoms({{Tk::num, "10"},
            {Tk::num, "-1"},
            {Tk::num, "34226"},
            {Tk::num, "-30920"},
            {Tk::num, "0XFFFF"},
            {Tk::num, "0xFFFF"},
            {Tk::num, "0xF0"},
            {Tk::num, "0xffff"},
            {Tk::num, "0xf0"},
            {Tk::num, "0x00"},
            {Tk::num, "0x0"},
            {Tk::num, "0"}});

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
