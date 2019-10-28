#include "atom.h"
#include "env.h"
#include "fmt/format.h"
#include "lib/lib.h"
#include "logging.h"
#include "parser.h"
#include "tokenizer.h"

#include <cstdlib>
#include <fstream>
#include <getopt.h>
#include <streambuf>

#define LOGGER() (logging::get("csxp-main"))

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
    // todo:
    // fmt::print("csxp {}\n", version_string());
    fmt::print("csxp {}\n", "1.0.0");
    std::exit(EXIT_SUCCESS);
}

int main(int argc, char* argv[])
{
    constexpr struct option long_options[] =
    {
        {"exec", required_argument, nullptr, 'e'},
        {"help", no_argument, nullptr, 'h'},
        {"version", no_argument, nullptr, 'v'},
        {nullptr, 0, nullptr, 0}
    };

    const char* optargstr = "-e:hv";

    const char* exe_path = nullptr;

    int opt;
    while ((opt = getopt_long(argc, argv, optargstr,
                    long_options, nullptr)) != -1) {
        switch (opt) {
            case 'e':
                LOGGER()->info("exec: {}", optarg);
                exe_path = optarg;
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

    if (exe_path) {
        std::ifstream in(exe_path, std::ios::in | std::ios::binary);
        if (in)
        {
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

            auto tz = createTokenizer(str, exe_path);
            auto p = createParser(tz);

            atom::patom res = atom::Nil;
            while (p->next()) {
                res = env->eval(p->value());
            }
            } catch (const EnvError& e) {
                LOGGER()->error("env error: {}", e.what());
            } catch (const ParserError& e) {
                LOGGER()->error("parser error: {}", e.what());
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
