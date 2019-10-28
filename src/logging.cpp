#include "fmt/time.h"
#include "logging.h"

#include <ctime>
#include <string_view>
#include <unordered_map>

using namespace std::literals;

template <>
struct fmt::formatter<std::shared_ptr<logging::Logger>>
{
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const std::shared_ptr<logging::Logger>& logger, FormatContext& ctx)
    {
        if (logger) {
            return format_to(ctx.out(), "logger '{}'", logger->name());
        } else {
            return format_to(ctx.out(), "null logger");
        }
    }
};

static std::unordered_map<std::string, std::shared_ptr<logging::Logger>> g_loggers;

constexpr std::array level_names{
        "TRACE"sv,
        "DEBUG"sv,
        " INFO"sv,
        " WARN"sv,
        "ERROR"sv,
        "FATAL"sv,
        "OTHER"sv
};

std::shared_ptr<logging::Logger> logging::get(std::string_view name)
{
    // todo: allocating a string here feels like a hack
    std::string sname{name};
    auto found = g_loggers.find(sname);
    if (found != g_loggers.end())
        return found->second;
    else {
        auto logger = std::make_shared<logging::Logger>(sname);
        g_loggers[sname] = logger;
        return logger;
    }
}

std::shared_ptr<logging::Logger> logging::dbg()
{
    return logging::get("dbg");
}

void logging::details::log_message(const logging::details::Message& msg)
{
    auto ts = std::chrono::system_clock::to_time_t(msg.ts);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            msg.ts.time_since_epoch())
                      .count();

    std::tm ltm = {0};
    localtime_r(&ts, &ltm);

    int level = msg.level;
    if (level < logging::trace || logging::off < level)
        level = logging::off; // OTHER
    auto levelstr = level_names[level];

    fmt::print("{:%Y-%m-%dT%H:%M:%S}.{:03d} {} {} - {}\n",
            ltm, ms % 1000, levelstr, msg.logname, msg.msg);
}