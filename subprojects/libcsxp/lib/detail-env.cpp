#include "csxp/atom_fmt.h"
#include "csxp/env.h"
#include "csxp/reader.h"
#include "csxp/lib/detail/env.h"
#include "csxp/lib/detail/util.h"
#include "csxp/lib/lib.h"
#include "csxp/logging.h"

#include <fstream>

#define LOGGER() (logging::get("lib/detail/env"))

using namespace std::literals;

namespace csxp::lib::detail::env {

// todo: tests?
patom load_file(Env* env, AtomIterator* args)
{
    // todo: prevent circular loading

    auto name = util::arg_next<Str>(env, args, 0, "core/load-file"sv);
    LOGGER()->debug("loading file \"{}\"", name->val);

    std::ifstream in(name->val, std::ios::in | std::ios::binary);
    if (in) {
        std::string str;
        in.seekg(0, std::ios::end);
        str.resize(in.tellg());
        in.seekg(0, std::ios::beg);
        in.read(str.data(), str.size());
        in.close();

        std::string initialNs = env->currNs();
        for (auto val : reader(str, name->val)) {
            env->eval(val);
        }

        // reset ns, in case it was in the file
        env->currNs(initialNs);
    } else {
        throw lib::LibError(
                fmt::format("unable to open input file '{}'", name->val));
    }

    return Nil;
}

} // namespace csxp::lib::detail::env
