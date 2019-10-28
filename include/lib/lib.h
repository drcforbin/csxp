#ifndef LIB_LIB_H
#define LIB_LIB_H

#include "detail-op.h"
#include "detail-core.h"
#include "detail-fn.h"

class Env;

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

// todo: shared_ptr?
void addCore(Env* env);
void addMath(Env* env);

} // namespace lib

#endif // LIB_LIB_H

