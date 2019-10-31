#include "env.h"
#include "fmt/format.h"
#include "logging.h"

#include <deque>
#include <string_view>
#include <unordered_map>
#define LOGGER() (logging::get("env"))

using namespace std::literals;

// todo: look for places where we should set to atom::Nil

struct EnvImpl : public Env
{
public:
    // TODO: namespaces need some love. what namespace are we in,
    // when a func blows up? see also nswhere.
    // TODO: make internal a slice, to implement dynamic scope

    atom::patom resolve(const atom::SymName* name);
    atom::patom eval(const atom::patom& val);
    void destructure(const atom::patom& binding, const atom::patom& val);
    void pushScope();
    void popScope();
    void setInternal(std::string_view name, const atom::patom& val);
    void setInternal(const atom::SymName* name,
            const atom::patom& val);
    void registerModule(std::string_view nsname,
            void (*initfunc)(Env* env, std::string_view nsname));
    std::shared_ptr<Module> createModule(std::string_view nsname);
    bool hasModule(std::string_view nsname);
    void aliasNs(std::string_view nswhere, std::string_view nsname,
            std::string_view nstarget);

private:
    atom::patom evalMap(const std::shared_ptr<atom::Map>& map);
    atom::patom evalVec(const std::shared_ptr<atom::Vec>& vec);
    atom::patom evalList(const std::shared_ptr<atom::List>& lst);

    std::unordered_map<std::string,
            std::unordered_map<std::string, atom::patom>>
            namespaces;
    std::unordered_map<std::string,
            std::unordered_map<std::string, std::string>>
            aliases;
    std::unordered_map<std::string, atom::patom> internal = {
            {"true", atom::True},
            {"false", atom::False},
    };

    std::deque<std::unordered_map<std::string, atom::patom>> scope;
};

atom::patom EnvImpl::resolve(const atom::SymName* name)
{
    const auto& sname = name->name;

    // sure would be nice to be able to look up string_views
    // in a string keyed unordered_map
    std::string nspart;
    std::string namepart;

    auto slash = sname.find('/');
    if (slash != std::string::npos) {
        nspart = sname.substr(0, slash);

        // todo: implement nswhere!
        const auto& requires = aliases[""];
        if (auto it = requires.find(nspart); it != requires.end()) {
            nspart = it->second;
        }

        namepart = sname.substr(slash + 1);
    } else {
        namepart = sname;
    }

    atom::patom res;
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
        for (auto it = scope.crbegin(); it != scope.crend(); it++) {
            auto srch = it->find(namepart);
            if (srch != it->cend()) {
                res = srch->second;
                break;
            }
        }

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

atom::patom EnvImpl::evalMap(const std::shared_ptr<atom::Map>& map)
{
    auto resMap = std::make_shared<atom::Map>();

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

    return std::make_shared<atom::atom>(resMap);
}

atom::patom EnvImpl::evalVec(const std::shared_ptr<atom::Vec>& vec)
{
    auto resVec = std::make_shared<atom::Vec>();

    auto it = vec->iterator();
    while (it->next()) {
        resVec->items.push_back(eval(it->value()));
    }

    return std::make_shared<atom::atom>(resVec);
}

atom::patom EnvImpl::evalList(const std::shared_ptr<atom::List>& lst)
{
    // evaluate the first argument
    auto it = lst->iterator();

    // empty list, evals to self
    if (!it->next()) {
        return std::make_shared<atom::atom>(lst);
    }

    auto first = it->value();
    auto res = eval(first);

    if (auto call = std::get_if<std::shared_ptr<atom::Callable>>(&res->p)) {
        return (**call)(static_cast<Env*>(this), it.get());
    }

    throw EnvError("unable to cast first item of list to Callable");
}

atom::patom EnvImpl::eval(const atom::patom& val)
{
    if (is_nil(val)) {
        return atom::Nil;
    }

    return std::visit(
            [this, &val](auto&& p) -> atom::patom {
                using T = std::decay_t<decltype(p)>;
                if constexpr (
                        std::is_same_v<T, std::shared_ptr<std::monostate>> ||
                        std::is_same_v<T, std::shared_ptr<atom::Callable>> ||
                        std::is_same_v<T, std::shared_ptr<atom::Char>> ||
                        std::is_same_v<T, std::shared_ptr<atom::Const>> ||
                        std::is_same_v<T, std::shared_ptr<atom::Keyword>> ||
                        std::is_same_v<T, std::shared_ptr<atom::Num>> ||
                        std::is_same_v<T, std::shared_ptr<atom::Str>>) {
                    return val;
                } else if constexpr (std::is_same_v<T, std::shared_ptr<atom::SymName>>) {
                    return resolve(p.get());
                } else if constexpr (std::is_same_v<T, std::shared_ptr<atom::Seq>>) {
                    if (auto m = std::dynamic_pointer_cast<atom::Map>(p)) {
                        return evalMap(m);
                    } else if (auto l = std::dynamic_pointer_cast<atom::List>(p)) {
                        return evalList(l);
                    } else if (auto v = std::dynamic_pointer_cast<atom::Vec>(p)) {
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

void EnvImpl::destructure(const atom::patom& binding, const atom::patom& val)
{
    // todo: check for nil binding?

    // todo: refactor
    std::visit(
            [this, &val](auto&& binding) {
                using T = std::decay_t<decltype(binding)>;
                if constexpr (std::is_same_v<T, std::shared_ptr<atom::SymName>>) {
                    if (!scope.empty()) {
                        scope.back()[binding->name] = val;
                    } else {
                        throw EnvError("attempted to set scope when scope stack was empty");
                    }
                } else if constexpr (std::is_same_v<T, std::shared_ptr<atom::Seq>>) {
                    if (auto bindvec = std::dynamic_pointer_cast<atom::Vec>(binding)) {
                        if (auto seq = std::get_if<std::shared_ptr<atom::Seq>>(&val->p)) {
                            auto valit = (*seq)->iterator();

                            for (std::size_t i = 0; i < bindvec->items.size(); i++) {
                                auto b = bindvec->items[i];

                                bool handled = std::visit(
                                        [this, &bindvec, &val, &valit, &i, &b](auto&& v) -> bool {
                                            using T = std::decay_t<decltype(v)>;
                                            if constexpr (std::is_same_v<T, std::shared_ptr<atom::SymName>>) {
                                                if (v->name == "&"sv) {
                                                    if (i > bindvec->items.size() - 2) {
                                                        throw EnvError("destructuring & requires a symbol");
                                                    }

                                                    // get / consume the symbol
                                                    i++;
                                                    b = bindvec->items[i];

                                                    if (std::holds_alternative<std::shared_ptr<atom::SymName>>(b->p)) {
                                                        auto vec = std::make_shared<atom::Vec>();
                                                        while (valit->next()) {
                                                            vec->items.push_back(valit->value());
                                                        }

                                                        destructure(b, std::make_shared<atom::atom>(vec));
                                                        return true;
                                                    }
                                                }
                                            } else if constexpr (std::is_same_v<T, std::shared_ptr<atom::Keyword>>) {
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
                                    atom::patom valval;
                                    if (valit->next()) {
                                        valval = valit->value();
                                    } else {
                                        valval = atom::Nil;
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
    scope.push_back({});
}

void EnvImpl::popScope()
{
    scope.pop_back();
}

void EnvImpl::setInternal(std::string_view name, const atom::patom& val)
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
        namepart = name;
    }

    if (!nspart.empty()) {
        namespaces[nspart][namepart] = val;
    } else {
        internal[namepart] = val;
    }
}

void EnvImpl::setInternal(const atom::SymName* name,
        const atom::patom& val)
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

void EnvImpl::aliasNs(std::string_view nswhere, std::string_view nsname,
        std::string_view nstarget)
{
    LOGGER()->info("aliasNs: {} -> {}", nsname, nstarget);
    // ugh. allocating here sucks.
    aliases[std::string(nswhere)][std::string(nsname)] = nstarget;
}

std::shared_ptr<Env> createEnv()
{
    return std::make_shared<EnvImpl>();
}
