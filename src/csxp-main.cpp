#include "atom-fmt.h"
#include "atom.h"
#include "env.h"
#include "fmt/format.h"
#include "lib/lib.h"
#include "logging.h"
#include "reader.h"

#include <cstdlib>
#include <fstream>
#include <getopt.h>
#include <streambuf>

#define LOGGER() (logging::get("csxp-main"))

void dump(atom::patom a, int indent = 0)
{
    // todo: better way?
    std::string dent;
    for (int i = indent; i > 0; i--) {
        dent += " ";
    }

    std::visit(
            [ indent, &dent ](auto&& val) -> auto {
                using T = std::decay_t<decltype(val)>;

                if constexpr (std::is_same_v<T, std::monostate>) {
                    // noop
                } else if constexpr (std::is_same_v<T, std::shared_ptr<atom::Callable>>) {
                    fmt::print("{} callable\n", dent);
                } else if constexpr (std::is_same_v<T, std::shared_ptr<atom::Char>>) {
                    fmt::print("{} char\n", dent);
                } else if constexpr (std::is_same_v<T, std::shared_ptr<atom::Const>>) {
                    fmt::print("{} const\n", dent);
                } else if constexpr (std::is_same_v<T, std::shared_ptr<atom::Keyword>>) {
                    fmt::print("{} keyword\n", dent);
                } else if constexpr (std::is_same_v<T, std::shared_ptr<atom::Num>>) {
                    fmt::print("{} num {}\n", dent, val->val);
                } else if constexpr (std::is_same_v<T, std::shared_ptr<atom::Str>>) {
                    fmt::print("{} str\n", dent);
                } else if constexpr (std::is_same_v<T, std::shared_ptr<atom::SymName>>) {
                    fmt::print("{} sym\n", dent);
                } else if constexpr (std::is_same_v<T, std::shared_ptr<atom::Seq>>) {
                    if (auto m = std::dynamic_pointer_cast<atom::Map>(val)) {
                        fmt::print("{} map\n", dent);
                    } else if (auto l = std::dynamic_pointer_cast<atom::List>(val)) {
                        fmt::print("{} lst\n", dent);
                    } else if (auto v = std::dynamic_pointer_cast<atom::Vec>(val)) {
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

static void exit_help(int code)
{
    fmt::print((code == EXIT_SUCCESS) ? stdout : stderr,
            "Usage: csxp [options] [-- args]\n"
            "  -e, --exec FILE       run specific file\n"
            "  -h, --help            show help\n"
            "  -v, --version         show version and exit\n");
    std::exit(code);
}

static void exit_version()
{
    // todo: versioning
    // fmt::print("csxp {}\n", version_string());
    fmt::print("csxp {}\n", "1.0.0");
    std::exit(EXIT_SUCCESS);
}

// todo: remove dummy lib
void register_rwte(Env* env);

int main(int argc, char* argv[])
{
    constexpr struct option long_options[] =
            {
                    {"exec", required_argument, nullptr, 'e'},
                    {"test", required_argument, nullptr, 't'},
                    {"help", no_argument, nullptr, 'h'},
                    {"version", no_argument, nullptr, 'v'},
                    {nullptr, 0, nullptr, 0}};

    const char* optargstr = "-e:t:hv";

    const char* test_path = nullptr;
    const char* exe_path = nullptr;

    int opt;
    while ((opt = getopt_long(argc, argv, optargstr,
                    long_options, nullptr)) != -1) {
        switch (opt) {
            case 'e':
                LOGGER()->info("exec: {}", optarg);
                exe_path = optarg;
                break;
            case 't':
                LOGGER()->info("test: {}", optarg);
                test_path = optarg;
                break;
            case 'h':
                exit_help(EXIT_SUCCESS);
                break;
            case 'v':
                exit_version();
                break;
            case 1:
                fmt::print(stderr, "{}: invalid arg -- '{}'\n",
                        argv[0], argv[optind - 1]);
                [[fallthrough]];
            default:
                exit_help(EXIT_FAILURE);
        }
    }

    if (optind < argc) {
        LOGGER()->info("non-option args:");
        for (; optind < argc; optind++) {
            LOGGER()->info("{}", argv[optind]);

            // capture args with we had -e
            if (exe_path) {
                // todo: options.cmd.push_back(argv[optind]);
            }
        }
    }

    if (exe_path && test_path) {
        fmt::print(stderr, "{}: cannot specify exec and test\n", argv[0]);
    }

    if (exe_path) {
        std::ifstream in(exe_path, std::ios::in | std::ios::binary);
        if (in) {
            std::string str;
            in.seekg(0, std::ios::end);
            str.resize(in.tellg());
            in.seekg(0, std::ios::beg);
            in.read(str.data(), str.size());
            in.close();

            try {
                // set up basic env
                auto env = createEnv();
                lib::addCore(env.get());
                lib::addMath(env.get());

                register_rwte(env.get());

                atom::patom res = atom::Nil;
                for (auto val : reader(str, exe_path)) {
                    res = env->eval(val);
                }
            } catch (const EnvError& e) {
                LOGGER()->error("env error: {}", e.what());
            } catch (const ReaderError& e) {
                LOGGER()->error("reader error: {}", e.what());
            } catch (const std::runtime_error& e) {
                LOGGER()->error("unknown error: {}", e.what());
            }
        } else {
            LOGGER()->error("unable to open input file '{}'", exe_path);
        }
    }

    if (test_path) {
        std::ifstream in(test_path, std::ios::in | std::ios::binary);
        if (in) {
            std::string str;
            in.seekg(0, std::ios::end);
            str.resize(in.tellg());
            in.seekg(0, std::ios::beg);
            in.read(str.data(), str.size());
            in.close();

            try {
                // set up basic env
                //auto env = createEnv();
                //lib::addCore(env.get());
                //lib::addMath(env.get());

                for (auto val : reader(str, exe_path)) {
                    fmt::print("{}\n", *val);
                    dump(val);
                }
            } catch (const EnvError& e) {
                LOGGER()->error("env error: {}", e.what());
            } catch (const ReaderError& e) {
                LOGGER()->error("reader error: {}", e.what());
            } catch (const std::runtime_error& e) {
                LOGGER()->error("unknown error: {}", e.what());
            }
        } else {
            LOGGER()->error("unable to open input file '{}'", exe_path);
        }
    }

    LOGGER()->debug("exiting");
    return EXIT_SUCCESS;
}
