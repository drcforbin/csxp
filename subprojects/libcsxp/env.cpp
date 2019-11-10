#include "csxp/env.h"
#include "csxp/atom_fmt.h"
#include "csxp/logging.h"
#include "fmt/format.h"

#include <deque>
#include <stack>
#include <string_view>
// todo: compare map vs unordered_map perf
#include <unordered_map>

#define LOGGER() (logging::get("env"))

using namespace std::literals;

namespace csxp {

struct EnvImpl : public Env
{
public:
    EnvImpl();

    // TODO: namespaces need some love. what namespace are we in,
    // when a func blows up? see also nswhere.

    patom resolve(const SymName* name);
    patom lexResolve(const SymName* name);
    patom eval(const patom& val);
    void pushFrame(const Scope& scope);
    void popFrame();
    void destructure(const patom& binding, const patom& val);
    void pushScope();
    void popScope();
    void setInternal(std::string_view name, const patom& val);
    void setInternal(const SymName* name,
            const patom& val);
    void registerModule(std::string_view nsname,
            void (*initfunc)(Env* env, std::string_view nsname));
    std::shared_ptr<Module> createModule(std::string_view nsname);
    bool hasModule(std::string_view nsname);
    const std::string& currNs() const { return where; };
    void currNs(std::string_view nsname) { where = nsname; }
    void aliasNs(std::string_view nsname, std::string_view nstarget);

private:
    patom evalMap(const std::shared_ptr<Map>& map);
    patom evalVec(const std::shared_ptr<Vec>& vec);
    patom evalList(const std::shared_ptr<List>& lst);

    std::unordered_map<std::string,
            std::unordered_map<std::string, patom>>
            namespaces;
    std::unordered_map<std::string,
            std::unordered_map<std::string, std::string>>
            aliases;
    std::unordered_map<std::string, patom> internal = {
            {"true", True},
            {"false", False},
    };

    std::stack<std::deque<Scope>> stack;
    std::string where;
};

EnvImpl::EnvImpl()
{
    // stack needs an initial frame
    stack.emplace();
}

patom EnvImpl::resolve(const SymName* name)
{
    const auto& sname = name->name;

    // sure would be nice to be able to look up string_views
    // in a string keyed unordered_map
    std::string nspart;
    std::string namepart;

    auto slash = sname.find('/');
    if (slash != std::string::npos) {
        nspart = sname.substr(0, slash);

        const auto& requires = aliases[where];
        if (auto it = requires.find(nspart); it != requires.end()) {
            nspart = it->second;
        }

        namepart = sname.substr(slash + 1);
    } else {
        namepart = sname;
    }

    patom res;
    if (!nspart.empty()) {
        if (auto it = namespaces.find(nspart); it != namespaces.end()) {
            if (auto srch = it->second.find(namepart); srch != it->second.cend()) {
                res = srch->second;
            }
        } else {
            throw EnvError(fmt::format(
                    "unable to find namespace {} for {}", nspart, sname));
        }
    } else {
        // check scope next
        auto& frame = stack.top();
        for (auto it = frame.crbegin(); it != frame.crend(); it++) {
            auto srch = it->find(namepart);
            if (srch != it->cend()) {
                res = srch->second;
                break;
            }
        }

        // check curr ns if we have one
        if (!where.empty()) {
            if (auto it = namespaces.find(where); it != namespaces.end()) {
                if (auto srch = it->second.find(namepart); srch != it->second.cend()) {
                    res = srch->second;
                }
            }
        }
        // otherwise, check global ns
        if (!res) {
            if (auto srch = internal.find(namepart); srch != internal.cend()) {
                res = srch->second;
            }
        }
    }

    if (!res) {
        throw EnvError(fmt::format(
                "unable to find symbol {}", sname));
    }

    return res;
}

patom EnvImpl::lexResolve(const SymName* name)
{
    const auto& sname = name->name;

    // namespaces mean not lexical scope
    auto slash = sname.find('/');
    if (slash == std::string::npos) {
        // check scopes
        auto& frame = stack.top();
        for (auto it = frame.crbegin(); it != frame.crend(); it++) {
            auto srch = it->find(sname);
            if (srch != it->cend()) {
                return srch->second;
            }
        }
    }

    return {};
}

patom EnvImpl::evalMap(const std::shared_ptr<Map>& map)
{
    auto resMap = std::make_shared<Map>();

    /* todo: map
    var pairAtom atom.Atom
    it := m.Iterator()
    for it.Next() {
        // assumes map is well formed, where each value is a vec pair

        if pairAtom, err = env.Eval(it.Value()); err != nil {
            return
        }

        pair := pairAtom.(*atom.Vec)
        resMap = resMap.Assoc(pair.Items[0], pair.Items[1])
    }
    */

    auto it = map->iterator();
    while (it->next()) {
        // todo: map
        // resVec->items.push_back(eval(it->value()));
    }

    return std::make_shared<atom>(resMap);
}

patom EnvImpl::evalVec(const std::shared_ptr<Vec>& vec)
{
    auto resVec = std::make_shared<Vec>();

    auto it = vec->iterator();
    while (it->next()) {
        resVec->items.push_back(eval(it->value()));
    }

    return std::make_shared<atom>(resVec);
}

patom EnvImpl::evalList(const std::shared_ptr<List>& lst)
{
    // evaluate the first argument
    auto it = lst->iterator();

    // empty list, evals to self
    if (!it->next()) {
        return std::make_shared<atom>(lst);
    }

    auto first = it->value();
    auto res = eval(first);

    if (auto call = get_if<Callable>(res)) {
        return (*call)(static_cast<Env*>(this), it.get());
    }

    throw EnvError("unable to cast first item of list to Callable");
}

patom EnvImpl::eval(const patom& val)
{
    if (is_nil(val)) {
        return Nil;
    }

    return std::visit(
            [this, &val](auto&& p) -> patom {
                using T = std::decay_t<decltype(p)>;
                if constexpr (
                        std::is_same_v<T, std::shared_ptr<std::monostate>> ||
                        std::is_same_v<T, std::shared_ptr<Callable>> ||
                        std::is_same_v<T, std::shared_ptr<Char>> ||
                        std::is_same_v<T, std::shared_ptr<Const>> ||
                        std::is_same_v<T, std::shared_ptr<Keyword>> ||
                        std::is_same_v<T, std::shared_ptr<Num>> ||
                        std::is_same_v<T, std::shared_ptr<Str>>) {
                    return val;
                } else if constexpr (std::is_same_v<T, std::shared_ptr<SymName>>) {
                    return resolve(p.get());
                } else if constexpr (std::is_same_v<T, std::shared_ptr<Seq>>) {
                    if (auto m = std::dynamic_pointer_cast<Map>(p)) {
                        return evalMap(m);
                    } else if (auto l = std::dynamic_pointer_cast<List>(p)) {
                        return evalList(l);
                    } else if (auto v = std::dynamic_pointer_cast<Vec>(p)) {
                        return evalVec(v);
                    } else {
                        // todo: add type to msg
                        throw EnvError("unexpected seq type in eval");
                    }
                } else {
                    // todo: add type to msg
                    throw EnvError("unexpected type in eval");
                }
            },
            val->p);
}

void EnvImpl::pushFrame(const Scope& scope)
{
    stack.push({scope});
}

void EnvImpl::popFrame()
{
    stack.pop();
}

void EnvImpl::destructure(const patom& binding, const patom& val)
{
    // todo: check for nil binding?

    // todo: refactor
    std::visit(
            [this, &val](auto&& binding) {
                using T = std::decay_t<decltype(binding)>;
                if constexpr (std::is_same_v<T, std::shared_ptr<SymName>>) {
                    auto& frame = stack.top();
                    if (!frame.empty()) {
                        frame.back()[binding->name] = val;
                    } else {
                        throw EnvError("attempted to set scope when scope stack was empty");
                    }
                } else if constexpr (std::is_same_v<T, std::shared_ptr<Seq>>) {
                    if (auto bindvec = std::dynamic_pointer_cast<Vec>(binding)) {
                        if (auto seq = get_if<Seq>(val)) {
                            auto valit = seq->iterator();

                            for (std::size_t i = 0; i < bindvec->items.size(); i++) {
                                auto b = bindvec->items[i];

                                bool handled = std::visit(
                                        [this, &bindvec, &val, &valit, &i, &b](auto&& v) -> bool {
                                            using T = std::decay_t<decltype(v)>;
                                            if constexpr (std::is_same_v<T, std::shared_ptr<SymName>>) {
                                                if (v->name == "&"sv) {
                                                    if (i > bindvec->items.size() - 2) {
                                                        throw EnvError("destructuring & requires a symbol");
                                                    }

                                                    // get / consume the symbol
                                                    i++;
                                                    b = bindvec->items[i];

                                                    if (std::holds_alternative<std::shared_ptr<SymName>>(b->p)) {
                                                        auto vec = std::make_shared<Vec>();
                                                        while (valit->next()) {
                                                            vec->items.push_back(valit->value());
                                                        }

                                                        destructure(b, std::make_shared<atom>(vec));
                                                        return true;
                                                    }
                                                }
                                            } else if constexpr (std::is_same_v<T, std::shared_ptr<Keyword>>) {
                                                if (v->name == ":as"sv) {
                                                    if (i > bindvec->items.size() - 2) {
                                                        throw EnvError("destructuring :as requires a symbol");
                                                    }

                                                    // get / consume the symbol
                                                    i++;
                                                    b = bindvec->items[i];

                                                    // bind symbol to incoming value
                                                    destructure(b, val);
                                                    return true;
                                                }
                                            }
                                            return false;
                                        },
                                        b->p);

                                if (!handled) {
                                    patom valval;
                                    if (valit->next()) {
                                        valval = valit->value();
                                    } else {
                                        valval = Nil;
                                    }

                                    destructure(b, valval);
                                }
                            }
                        } else {
                            throw EnvError("destructuring vector, unable to iterate over arg");
                        }
                    } else {
                        throw EnvError("unexpected seq type for destructure binding");
                    }
                } else {
                    // for map:
                    // TODO: destructure map
                    // TODO: :keys, :strs and :syms

                    throw EnvError("unexpected type for destructure binding");
                }
            },
            binding->p);
}

void EnvImpl::pushScope()
{
    stack.top().emplace_back();
}

void EnvImpl::popScope()
{
    stack.top().pop_back();
}

void EnvImpl::setInternal(std::string_view name, const patom& val)
{
    // sure would be nice to be able to look up string_views
    // in a string keyed unordered_map
    std::string nspart;
    std::string namepart;

    auto slash = name.find('/');
    if (slash != std::string::npos) {
        nspart = name.substr(0, slash);
        namepart = name.substr(slash + 1);
    } else {
        nspart = where;
        namepart = name;
    }

    if (!nspart.empty()) {
        namespaces[nspart][namepart] = val;
    } else {
        internal[namepart] = val;
    }
}

void EnvImpl::setInternal(const SymName* name,
        const patom& val)
{
    setInternal(name->name, val);
}

void EnvImpl::registerModule(std::string_view nsname,
        void (*initfunc)(Env* env, std::string_view nsname))
{
    // todo: lazy initialization upon require
    initfunc(this, nsname);
}

struct ModuleImpl : public Module
{
    ModuleImpl(Env* env, std::string_view nsname) :
        env(env), nsname(nsname) {}

    void addMembers(const std::vector<ModuleMember>& members)
    {
        for (auto& member : members) {
            auto name = fmt::format("{}/{}", nsname, member.name);
            env->setInternal(name, member.member);
        }
    }

    Env* env;
    std::string nsname;
};

std::shared_ptr<Module> EnvImpl::createModule(std::string_view nsname)
{
    namespaces[std::string(nsname)] = {};
    return std::make_shared<ModuleImpl>(static_cast<Env*>(this), nsname);
}

bool EnvImpl::hasModule(std::string_view nsname)
{
    // ugh.
    return namespaces.find(std::string(nsname)) != namespaces.end();
}

// todo: swap args, like "alias X as x"
void EnvImpl::aliasNs(std::string_view nsname, std::string_view nstarget)
{
    // ugh. allocating here sucks.
    aliases[where][std::string(nsname)] = nstarget;
}

std::shared_ptr<Env> createEnv()
{
    return std::make_shared<EnvImpl>();
}

} // namespace csxp
