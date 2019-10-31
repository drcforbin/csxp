#include "catch.hpp"
#include "tokenizer.h"

#include <string_view>

// todo: add iterator-based tests

extern const std::string_view arbitrary_clj;
extern const std::string_view arbitrary_long_clj;

using namespace std::literals;

TEST_CASE("can tokenize keyword", "[tokenizer]")
{
    // todo: test namespaced keyword

    auto str = ":keyword"sv;
    auto tz = createTokenizer(str, "internal-test");

    // should be able to get first token
    REQUIRE(tz->next());

    // should be a kw
    auto tok = tz->value();
    REQUIRE(tok.t == Tk::keyword);

    // should be done now
    REQUIRE(!tz->next());
    // end val should be eof
    tok = tz->value();
    REQUIRE(tok.t == Tk::eof);

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

TEST_CASE("can tokenize list", "[tokenizer]")
{
    auto str = "(:keyword)"sv;
    auto tz = createTokenizer(str, "internal-test");

    // should be able to get first token
    REQUIRE(tz->next());

    // should be a lparen
    auto tok = tz->value();
    REQUIRE(tok.t == Tk::lparen);
    REQUIRE(tok.s == "(");

    // should be a kw
    REQUIRE(tz->next());
    tok = tz->value();
    REQUIRE(tok.t == Tk::keyword);
    REQUIRE(tok.s == ":keyword");

    // should be a rparen
    REQUIRE(tz->next());
    tok = tz->value();
    REQUIRE(tok.t == Tk::rparen);
    REQUIRE(tok.s == ")");

    // should be done now
    REQUIRE(!tz->next());
    // end val should be eof
    tok = tz->value();
    REQUIRE(tok.t == Tk::eof);
}

TEST_CASE("can tokenize vec", "[tokenizer]")
{
    auto str = "[:keyword]"sv;
    auto tz = createTokenizer(str, "internal-test");

    // should be able to get first token
    REQUIRE(tz->next());

    // should be a lbracket
    auto tok = tz->value();
    REQUIRE(tok.t == Tk::lbracket);
    REQUIRE(tok.s == "[");

    // should be a kw
    REQUIRE(tz->next());
    tok = tz->value();
    REQUIRE(tok.t == Tk::keyword);
    REQUIRE(tok.s == ":keyword");

    // should be a rbracket
    REQUIRE(tz->next());
    tok = tz->value();
    REQUIRE(tok.t == Tk::rbracket);
    REQUIRE(tok.s == "]");

    // should be done now
    REQUIRE(!tz->next());
    // end val should be eof
    tok = tz->value();
    REQUIRE(tok.t == Tk::eof);
}

TEST_CASE("can tokenize quote and num", "[tokenizer]")
{
    auto str = "'1"sv;
    auto tz = createTokenizer(str, "internal-test");

    // should be able to get first token
    REQUIRE(tz->next());

    // should be a quote
    auto tok = tz->value();
    REQUIRE(tok.t == Tk::quote);
    REQUIRE(tok.s == "'");

    // should be a num
    REQUIRE(tz->next());
    tok = tz->value();
    REQUIRE(tok.t == Tk::num);
    REQUIRE(tok.s == "1");

    // should be done now
    REQUIRE(!tz->next());
    // end val should be eof
    tok = tz->value();
    REQUIRE(tok.t == Tk::eof);
}

TEST_CASE("can tokenize chars", "[tokenizer]")
{
    // see "can parse chars" parser test

    auto str = R"-(
            \a \b \2 \3
            \newline \space \tab \formfeed \backspace \return
            \u03A9 \o777
            )-"sv;

    auto tz = createTokenizer(str, "internal-test");

    REQUIRE(tz->next());
    auto tok = tz->value();
    REQUIRE(tok.t == Tk::chr);
    REQUIRE(tok.s == "a");

    REQUIRE(tz->next());
    tok = tz->value();
    REQUIRE(tok.t == Tk::chr);
    REQUIRE(tok.s == "b");

    REQUIRE(tz->next());
    tok = tz->value();
    REQUIRE(tok.t == Tk::chr);
    REQUIRE(tok.s == "2");

    REQUIRE(tz->next());
    tok = tz->value();
    REQUIRE(tok.t == Tk::chr);
    REQUIRE(tok.s == "3");

    REQUIRE(tz->next());
    tok = tz->value();
    REQUIRE(tok.t == Tk::chr);
    REQUIRE(tok.s == "\n");

    REQUIRE(tz->next());
    tok = tz->value();
    REQUIRE(tok.t == Tk::chr);
    REQUIRE(tok.s == " ");

    REQUIRE(tz->next());
    tok = tz->value();
    REQUIRE(tok.t == Tk::chr);
    REQUIRE(tok.s == "\t");

    REQUIRE(tz->next());
    tok = tz->value();
    REQUIRE(tok.t == Tk::chr);
    REQUIRE(tok.s == "\f");

    REQUIRE(tz->next());
    tok = tz->value();
    REQUIRE(tok.t == Tk::chr);
    REQUIRE(tok.s == "\b");

    REQUIRE(tz->next());
    tok = tz->value();
    REQUIRE(tok.t == Tk::chr);
    REQUIRE(tok.s == "\r");

    REQUIRE(tz->next());
    tok = tz->value();
    REQUIRE(tok.t == Tk::chr);
    REQUIRE(tok.s == "u03A9");

    REQUIRE(tz->next());
    tok = tz->value();
    REQUIRE(tok.t == Tk::chr);
    REQUIRE(tok.s == "o777");

    REQUIRE(!tz->next());
}

TEST_CASE("can tokenize nums", "[tokenizer]")
{
    // see "can parse nums" parser test

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

    auto tz = createTokenizer(str, "internal-test");

    REQUIRE(tz->next());
    auto tok = tz->value();
    REQUIRE(tok.t == Tk::num);
    REQUIRE(tok.s == "10");

    REQUIRE(tz->next());
    tok = tz->value();
    REQUIRE(tok.t == Tk::num);
    REQUIRE(tok.s == "-1");

    REQUIRE(tz->next());
    tok = tz->value();
    REQUIRE(tok.t == Tk::num);
    REQUIRE(tok.s == "34226");

    REQUIRE(tz->next());
    tok = tz->value();
    REQUIRE(tok.t == Tk::num);
    REQUIRE(tok.s == "-30920");

    REQUIRE(tz->next());
    tok = tz->value();
    REQUIRE(tok.t == Tk::num);
    REQUIRE(tok.s == "0XFFFF");

    REQUIRE(tz->next());
    tok = tz->value();
    REQUIRE(tok.t == Tk::num);
    REQUIRE(tok.s == "0xFFFF");

    REQUIRE(tz->next());
    tok = tz->value();
    REQUIRE(tok.t == Tk::num);
    REQUIRE(tok.s == "0xF0");

    REQUIRE(tz->next());
    tok = tz->value();
    REQUIRE(tok.t == Tk::num);
    REQUIRE(tok.s == "0xffff");

    REQUIRE(tz->next());
    tok = tz->value();
    REQUIRE(tok.t == Tk::num);
    REQUIRE(tok.s == "0xf0");

    REQUIRE(tz->next());
    tok = tz->value();
    REQUIRE(tok.t == Tk::num);
    REQUIRE(tok.s == "0x00");

    REQUIRE(tz->next());
    tok = tz->value();
    REQUIRE(tok.t == Tk::num);
    REQUIRE(tok.s == "0x0");

    REQUIRE(tz->next());
    tok = tz->value();
    REQUIRE(tok.t == Tk::num);
    REQUIRE(tok.s == "0");

    REQUIRE(!tz->next());
}
