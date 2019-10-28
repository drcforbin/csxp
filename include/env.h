#ifndef ENV_H
#define ENV_H

#include "atom.h"

#include <functional>
#include <memory>
#include <string_view>

class EnvError : public std::runtime_error
{
public:
    explicit EnvError(const std::string& what_arg) :
        std::runtime_error(what_arg)
    {}

    explicit EnvError(const char* what_arg) :
        std::runtime_error(what_arg)
    {}
};

struct Env
{
    virtual ~Env() = default;

    virtual atom::patom resolve(const atom::SymName* name) = 0;
    virtual atom::patom eval(const atom::patom& val) = 0;
    virtual void destructure(const atom::patom& binding, const atom::patom& val) = 0;
    virtual void pushScope() = 0;
    virtual void popScope() = 0;
    virtual void setInternal(std::string_view name, const atom::patom& val) = 0;
    virtual void setInternal(const atom::SymName* name,
            const atom::patom& val) = 0;
};

class EnvScope
{
public:
    EnvScope(Env* env) : env(env) { env->pushScope(); }
    ~EnvScope() { env->popScope(); }
private:
    Env* env;
};

std::shared_ptr<Env> createEnv();

#endif // ENV_H
