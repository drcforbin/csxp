#ifndef CSXP_ENV_H
#define CSXP_ENV_H

#include "csxp/atom.h"

#include <functional>
#include <memory>
#include <string_view>

namespace csxp {

// todo: add the rest of this to the env::namespace

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

struct ModuleMember
{
    std::string_view name;
    patom member;
};

struct Module
{
    virtual ~Module() = default;

    virtual void addMembers(const std::vector<ModuleMember>& members) = 0;
};

using Scope = std::unordered_map<std::string, patom>;

struct Env
{
    virtual ~Env() = default;

    virtual patom resolve(const SymName* name) = 0;
    virtual patom lexResolve(const SymName* name) = 0;
    virtual patom eval(const patom& val) = 0;
    virtual void pushFrame(const Scope& scope) = 0;
    virtual void popFrame() = 0;
    virtual void destructure(const patom& binding, const patom& val) = 0;
    virtual void pushScope() = 0;
    virtual void popScope() = 0;
    virtual void setInternal(std::string_view name, const patom& val) = 0;
    virtual void setInternal(const SymName* name,
            const patom& val) = 0;
    virtual void registerModule(std::string_view nsname,
            void (*initfunc)(Env* env, std::string_view nsname)) = 0;
    virtual std::shared_ptr<Module> createModule(std::string_view nsname) = 0;
    virtual bool hasModule(std::string_view nsname) = 0;
    virtual const std::string& currNs() const = 0;
    virtual void currNs(std::string_view nsname) = 0;
    virtual void aliasNs(std::string_view nsname, std::string_view nstarget) = 0;
};

// todo: drop Env prefix, have returned by pushScope
class EnvScope
{
public:
    EnvScope(Env* env) :
        env(env) { env->pushScope(); }
    ~EnvScope() { env->popScope(); }

private:
    Env* env;
};

// todo: drop Env prefix, have returned by pushFrame
class EnvFrame
{
public:
    EnvFrame(Env* env, const Scope& scope) :
        env(env) { env->pushFrame(scope); }
    ~EnvFrame() { env->popFrame(); }

private:
    Env* env;
};

std::shared_ptr<Env> createEnv();

} // namespace csxp

#endif // CSXP_ENV_H
