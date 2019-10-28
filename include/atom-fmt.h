#ifndef ATOM_FMT_H
#define ATOM_FMT_H

#include "fmt/format.h"

namespace fmt {

template <>
struct formatter<atom::atom::variant_type>
{
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const atom::atom::variant_type& v, FormatContext& ctx)
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
struct formatter<atom::Callable>
{
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const atom::Callable& c, FormatContext& ctx)
    {
        // todo: anything else we can say here?
        return format_to(ctx.out(), "<callable>");
    }
};

template <>
struct formatter<atom::Char>
{
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const atom::Char& c, FormatContext& ctx)
    {
        // todo: format newline, unicode, octal
        return format_to(ctx.out(), "\\{}", c.val);
    }
};

template <>
struct formatter<atom::Const>
{
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const atom::Const& c, FormatContext& ctx)
    {
        return format_to(ctx.out(), c.name);
    }
};

template <>
struct formatter<atom::Keyword>
{
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const atom::Keyword& k, FormatContext& ctx)
    {
        return format_to(ctx.out(), "{}", k.name);
    }
};

template <>
struct formatter<atom::List>
{
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const atom::List& l, FormatContext& ctx)
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
struct formatter<atom::Num>
{
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const atom::Num& n, FormatContext& ctx)
    {
        return format_to(ctx.out(), "{}", n.val);
    }
};

template <>
struct formatter<atom::Seq>
{
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const atom::Seq& s, FormatContext& ctx)
    {
        if (auto v = dynamic_cast<atom::Vec*>(&s)) {
            return format_to(ctx.out(), "{}", *v);
        } else if (auto l = dynamic_cast<atom::List*>(&s)) {
            return format_to(ctx.out(), "{}", *l);
        } else {
            // todo: add type to msg
            return format_to(ctx.out(), "<unknown seq>");
        }
    }
};

template <>
struct formatter<atom::Str>
{
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const atom::Str& s, FormatContext& ctx)
    {
        return format_to(ctx.out(), "{}", s.val);
    }
};

template <>
struct formatter<atom::SymName>
{
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const atom::SymName& s, FormatContext& ctx)
    {
        return format_to(ctx.out(), "{}", s.name);
    }
};

template <>
struct formatter<atom::Vec>
{
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const atom::Vec& v, FormatContext& ctx)
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
struct formatter<atom::atom>
{
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const atom::atom& a, FormatContext& ctx)
    {
        return std::visit(
                [&ctx](auto&& val) -> auto {
                    using T = std::decay_t<decltype(val)>;

                    if constexpr (std::is_same_v<T, std::monostate>) {
                        return format_to(ctx.out(), "<empty>");
                    } else if constexpr (
                            std::is_same_v<T, std::shared_ptr<atom::Callable>> ||
                            std::is_same_v<T, std::shared_ptr<atom::Char>> ||
                            std::is_same_v<T, std::shared_ptr<atom::Const>> ||
                            std::is_same_v<T, std::shared_ptr<atom::Keyword>> ||
                            std::is_same_v<T, std::shared_ptr<atom::Num>> ||
                            std::is_same_v<T, std::shared_ptr<atom::Str>> ||
                            std::is_same_v<T, std::shared_ptr<atom::SymName>>) {
                        return format_to(ctx.out(), "{}", *val);
                    } else if constexpr (std::is_same_v<T, std::shared_ptr<atom::Seq>>) {
                        if (auto l = std::dynamic_pointer_cast<atom::List>(val)) {
                            return format_to(ctx.out(), "{}", *l);
                        } else if (auto v = std::dynamic_pointer_cast<atom::Vec>(val)) {
                            return format_to(ctx.out(), "{}", *v);
                        } else {
                            // todo: add type to msg
                            throw std::runtime_error("unexpected seq type in atom formatter");
                        }
                    } else {
                        // todo: add eval map
                        // todo: add type to msg
                        // return format_to(ctx.out(), "huh? {}", typeid(T).name());
                        throw std::runtime_error("unexpected type in atom formatter");
                    }
                },
            a.p);
    }
};

} // namespace fmt

#endif // ATOM_FMT_H
