#ifndef CSXP_ATOM_FMT_H
#define CSXP_ATOM_FMT_H

#include "csxp/atom.h"
#include "fmt/format.h"

namespace fmt {

template <>
struct formatter<csxp::variant_type>
{
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const csxp::variant_type& v, FormatContext& ctx)
    {
        if (v.index() == 0) {
            // handle monostate
            return format_to(ctx.out(), "<empty>");
        } else {
            return format_to(ctx.out(), v);
        }
    }
};

template <>
struct formatter<csxp::Callable>
{
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const csxp::Callable& c, FormatContext& ctx)
    {
        // todo: anything else we can say here?
        return format_to(ctx.out(), "<callable>");
    }
};

template <>
struct formatter<csxp::Char>
{
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const csxp::Char& c, FormatContext& ctx)
    {
        // todo: format newline, unicode, octal
        //return format_to(ctx.out(), "\\{}", c.val);
        return format_to(ctx.out(), "<char>");
    }
};

template <>
struct formatter<csxp::Const>
{
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const csxp::Const& c, FormatContext& ctx)
    {
        return format_to(ctx.out(), c.name);
    }
};

template <>
struct formatter<csxp::Keyword>
{
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const csxp::Keyword& k, FormatContext& ctx)
    {
        return format_to(ctx.out(), "{}", k.name);
    }
};

template <>
struct formatter<csxp::List>
{
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const csxp::List& l, FormatContext& ctx)
    {
        format_to(ctx.out(), "(");
        for (std::size_t i = 0; i < l.items.size(); i++) {
            if (i != 0) {
                format_to(ctx.out(), " ");
            }

            format_to(ctx.out(), "{}", *l.items[i]);
        }
        return format_to(ctx.out(), ")");
    }
};

template <>
struct formatter<csxp::Map>
{
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const csxp::Map& m, FormatContext& ctx)
    {
        /*
        format_to(ctx.out(), "(");
        for (std::size_t i = 0; i < l.items.size(); i++) {
            if (i != 0) {
                format_to(ctx.out(), " ");
            }

            format_to(ctx.out(), "{}", *l.items[i]);
        }
        return format_to(ctx.out(), ")");
        */
        // todo: map
        return format_to(ctx.out(), "{???}");
    }
};

template <>
struct formatter<csxp::Num>
{
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const csxp::Num& n, FormatContext& ctx)
    {
        return format_to(ctx.out(), "{}", n.val);
    }
};

template <>
struct formatter<csxp::Seq>
{
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const csxp::Seq& s, FormatContext& ctx)
    {
        if (auto m = dynamic_cast<const csxp::Map*>(&s)) {
            return format_to(ctx.out(), "{}", *m);
        } else if (auto v = dynamic_cast<const csxp::Vec*>(&s)) {
            return format_to(ctx.out(), "{}", *v);
        } else if (auto l = dynamic_cast<const csxp::List*>(&s)) {
            return format_to(ctx.out(), "{}", *l);
        } else {
            // todo: add type to msg
            return format_to(ctx.out(), "<unknown seq>");
        }
    }
};

template <>
struct formatter<csxp::Str>
{
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const csxp::Str& s, FormatContext& ctx)
    {
        // todo: render escaped chars
        return format_to(ctx.out(), "\"{}\"", s.val);
    }
};

template <>
struct formatter<csxp::SymName>
{
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const csxp::SymName& s, FormatContext& ctx)
    {
        return format_to(ctx.out(), "{}", s.name);
    }
};

template <>
struct formatter<csxp::Vec>
{
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const csxp::Vec& v, FormatContext& ctx)
    {
        format_to(ctx.out(), "[");
        for (std::size_t i = 0; i < v.items.size(); i++) {
            if (i != 0) {
                format_to(ctx.out(), " ");
            }

            format_to(ctx.out(), "{}", *v.items[i]);
        }
        return format_to(ctx.out(), "]");
    }
};

template <>
struct formatter<csxp::atom>
{
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const csxp::atom& a, FormatContext& ctx)
    {
        return std::visit(
                [&ctx](auto&& val) -> auto {
                    using T = std::decay_t<decltype(val)>;

                    if constexpr (std::is_same_v<T, std::monostate>) {
                        return format_to(ctx.out(), "<empty>");
                    } else if constexpr (
                            std::is_same_v<T, std::shared_ptr<csxp::Callable>> ||
                            std::is_same_v<T, std::shared_ptr<csxp::Char>> ||
                            std::is_same_v<T, std::shared_ptr<csxp::Const>> ||
                            std::is_same_v<T, std::shared_ptr<csxp::Keyword>> ||
                            std::is_same_v<T, std::shared_ptr<csxp::Num>> ||
                            std::is_same_v<T, std::shared_ptr<csxp::Str>> ||
                            std::is_same_v<T, std::shared_ptr<csxp::SymName>>) {
                        return format_to(ctx.out(), "{}", *val);
                    } else if constexpr (std::is_same_v<T, std::shared_ptr<csxp::Seq>>) {
                        if (auto m = std::dynamic_pointer_cast<csxp::Map>(val)) {
                            return format_to(ctx.out(), "{}", *m);
                        } else if (auto l = std::dynamic_pointer_cast<csxp::List>(val)) {
                            return format_to(ctx.out(), "{}", *l);
                        } else if (auto v = std::dynamic_pointer_cast<csxp::Vec>(val)) {
                            return format_to(ctx.out(), "{}", *v);
                        } else {
                            // todo: add type to msg
                            throw std::runtime_error("unexpected seq type in atom formatter");
                        }
                    } else {
                        return format_to(ctx.out(), "<unknown type>");
                        // todo: add eval map
                        // todo: add type to msg
                        // return format_to(ctx.out(), "huh? {}", typeid(T).name());
                        //throw std::runtime_error("unexpected type in atom formatter");
                    }
                },
                a.p);
    }
};

} // namespace fmt

#endif // CSXP_ATOM_FMT_H
