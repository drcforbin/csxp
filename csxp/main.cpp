#include "csxp/csxp.h"
#include "csxp/atom_fmt.h"
#include "fmt/format.h"
#include "rw/argparse.h"
#include "rw/logging.h"

#include <cstdlib>
#include <fstream>
#include <getopt.h>
#include <streambuf>

#define LOGGER() (rw::logging::get("csxp-main"))

using namespace std::literals;

// todo: remove or something
void dump(csxp::patom a, int indent = 0)
{
    std::string dent(indent, ' ');

    std::visit(
            [ indent, &dent ](auto&& val) -> auto {
                using T = std::decay_t<decltype(val)>;

                if constexpr (std::is_same_v<T, std::monostate>) {
                    // noop
                } else if constexpr (std::is_same_v<T, std::shared_ptr<csxp::Callable>>) {
                    fmt::print("{} callable\n", dent);
                } else if constexpr (std::is_same_v<T, std::shared_ptr<csxp::Char>>) {
                    fmt::print("{} char\n", dent);
                } else if constexpr (std::is_same_v<T, std::shared_ptr<csxp::Const>>) {
                    fmt::print("{} const\n", dent);
                } else if constexpr (std::is_same_v<T, std::shared_ptr<csxp::Keyword>>) {
                    fmt::print("{} keyword\n", dent);
                } else if constexpr (std::is_same_v<T, std::shared_ptr<csxp::Num>>) {
                    fmt::print("{} num {}\n", dent, val->val);
                } else if constexpr (std::is_same_v<T, std::shared_ptr<csxp::Str>>) {
                    fmt::print("{} str\n", dent);
                } else if constexpr (std::is_same_v<T, std::shared_ptr<csxp::SymName>>) {
                    fmt::print("{} sym\n", dent);
                } else if constexpr (std::is_same_v<T, std::shared_ptr<csxp::Seq>>) {
                    if (auto m = std::dynamic_pointer_cast<csxp::Map>(val)) {
                        fmt::print("{} map\n", dent);
                    } else if (auto l = std::dynamic_pointer_cast<csxp::List>(val)) {
                        fmt::print("{} lst\n", dent);
                    } else if (auto v = std::dynamic_pointer_cast<csxp::Vec>(val)) {
                        fmt::print("{} vec\n", dent);
                    } else {
                        // todo: add type to msg
                        throw std::runtime_error("unexpected seq type in atom formatter");
                    }

                    auto it = val->iterator();
                    while (it->next()) {
                        dump(it->value(), indent + 1);
                    }
                } else {
                    // todo: add type to msg
                    // return format_to(ctx.out(), "huh? {}", typeid(T).name());
                    throw std::runtime_error("unexpected type in atom formatter");
                }
            },
            a->p);
}

auto usage = R"(
Usage: csxp [options] [-- args]
  -e, --exec FILE       run specific file
  -h, --help            show help
  -v, --version         show version and exit
)"sv;

static void exit_version()
{
    fmt::print("csxp {}", csxp::version_string());
    std::exit(EXIT_SUCCESS);
}

static std::optional<std::string> read_file(std::string_view path) {
    // ugh. why not take a string_view
    // todo: look at how to best do this...
    std::ifstream in(std::string(path), std::ios::in | std::ios::binary);
    if (in) {
        std::string str;
        in.seekg(0, std::ios::end);
        str.resize(in.tellg());
        in.seekg(0, std::ios::beg);
        in.read(str.data(), str.size());
        in.close();
    }
    return std::nullopt;
}

int main(int argc, char* argv[])
{
    // todo: review these
    // todo: maybe implement global level default
    rw::logging::get("env")->level(rw::logging::log_level::info);
    rw::logging::get("lib/detail/env")->level(rw::logging::log_level::info);

    LOGGER()->debug("csxp {}", csxp::version_string());

    std::string_view exec_path;
    bool show_version = false;
    bool run_tests = false;

    // todo: move NoOpts to argparse ns
    struct NoOpts {};
    auto p = rw::argparse::parser<NoOpts>{};
    p.one_of(false)
        .optional(&exec_path, "exec"sv, "e"sv)
        .optional(&run_tests, "test"sv)
        .optional(&show_version, "version"sv, "v"sv);
    p.usage(usage);
    auto o = p.parse(argc, argv);
    if (!o) {
        return EXIT_FAILURE;
    }

    if (show_version) {
        exit_version();
    }

    if (!exec_path.empty()) {
        LOGGER()->info("exec: {}", exec_path);
        if (auto str = read_file(exec_path)) {
            try {
                if (run_tests) {
                    // set up basic env
                    auto env = csxp::createEnv();
                    csxp::lib::addCore(env.get());
                    csxp::lib::addEnv(env.get());
                    csxp::lib::addMath(env.get());
                    // todo: implement some kinda search path
                    csxp::lib::addLib(env.get(), "../csxp-lib.clj"sv);

                    csxp::patom res = csxp::Nil;
                    for (auto val : csxp::reader(*str, exec_path)) {
                        res = env->eval(val);
                    }
                } else {
                    for (auto val : csxp::reader(*str, exec_path)) {
                        fmt::print("{}\n", *val);
                        dump(val);
                    }
                }
            } catch (const csxp::EnvError& e) {
                LOGGER()->error("env error: {}", e.what());
            } catch (const csxp::ReaderError& e) {
                LOGGER()->error("reader error: {}", e.what());
            } catch (const std::runtime_error& e) {
                LOGGER()->error("unknown error: {}", e.what());
            }
        } else {
            LOGGER()->error("unable to read input file '{}'", exec_path);
        }
    }

    LOGGER()->debug("exiting");
    return EXIT_SUCCESS;
}
