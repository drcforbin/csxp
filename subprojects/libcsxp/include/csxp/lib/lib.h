#ifndef CSXP_LIB_LIB_H
#define CSXP_LIB_LIB_H

#include <stdexcept>
#include <string>

namespace csxp {
struct Env;

// todo: yeah, namespace name sucks
namespace lib {

class LibError : public std::runtime_error
{
public:
    explicit LibError(const std::string& what_arg) :
        std::runtime_error(what_arg)
    {}

    explicit LibError(const char* what_arg) :
        std::runtime_error(what_arg)
    {}
};

void addCore(Env* env);
void addEnv(Env* env);
void addMath(Env* env);
void addLib(Env* env, std::string_view name);

} // namespace lib
} // namespace csxp

#endif // CSXP_LIB_LIB_H
