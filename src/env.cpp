#include "env.h"
#include "logging.h"
#include "fmt/format.h"

#include <deque>
#include <string_view>
#include <unordered_map>

#include "logging.h"
#define LOGGER() (logging::get("env"))

using namespace std::literals;

// todo: look for places where we should set to atom::Nil

struct EnvImpl : public Env {
public:

    // TODO: namespaces
    // TODO: make internal a slice, to implement dynamic scope

    atom::patom resolve(const atom::SymName* name);
    atom::patom eval(const atom::patom& val);
    void destructure(const atom::patom& binding, const atom::patom& val);
    void pushScope();
    void popScope();
    void setInternal(std::string_view name, const atom::patom& val);
    void setInternal(const atom::SymName* name,
        const atom::patom& val);

private:
    atom::patom evalVec(const std::shared_ptr<atom::Vec>& vec);
    atom::patom evalList(const std::shared_ptr<atom::List>& lst);

    std::unordered_map<std::string, atom::patom> internal = {
        {"true", atom::True},
        {"false", atom::False},
    };

    std::deque<std::unordered_map<std::string, atom::patom>> scope;
};


atom::patom EnvImpl::resolve(const atom::SymName* name) {
    atom::patom res;

    // check scope next
    for (auto it = scope.crbegin(); it != scope.crend(); it++) {
        auto srch = it->find(name->name);
        if (srch != it->cend()) {
            res = srch->second;
            break;
        }
    }

    if (!res) {
        auto srch = internal.find(name->name);
        if (srch != internal.cend()) {
            res = srch->second;
        }
    }

    if (!res) {
        throw EnvError(fmt::format(
            "unable to find symbol {}", name->name));
    }

    return res;
}

/*
atom::patom EnvImpl::evalMap(m hashmap.Map){
    res = atom.Nil
    resMap := hashmap.NewPersistentHashMap()

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

    res = resMap

    return
}
*/

atom::patom EnvImpl::evalVec(const std::shared_ptr<atom::Vec>& vec) {
    auto resVec = std::make_shared<atom::Vec>();

    auto it = vec->iterator();
    while (it->next()) {
        resVec->items.push_back(eval(it->value()));
    }

    return std::make_shared<atom::atom>(resVec);
}

atom::patom EnvImpl::evalList(const std::shared_ptr<atom::List>& lst){
    // evaluate the first argument
    auto it = lst->iterator();

    // empty list, evals to self
    if ( !it->next()) {
        return std::make_shared<atom::atom>(lst);
    }

    auto first = it->value();
    auto res = eval(first);

    if (auto call = std::get_if<std::shared_ptr<atom::Callable>>(&res->p)) {
        return (**call)(static_cast<Env*>(this), it.get());
        /* todo: catch, log?
        if err != nil {
            envLogger.Debugf("no res, err %s", err.Error())
        } else if !res.IsNil() {
            envLogger.Debugf("res %s, no err", res.String())
        } else {
            envLogger.Debug("no res, no err")
        }*/
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
                        if (auto l = std::dynamic_pointer_cast<atom::List>(p)) {
                            return evalList(l);
                        } else if (auto v = std::dynamic_pointer_cast<atom::Vec>(p)) {
                            return evalVec(v);
                        } else {
                            // todo: add type to msg
                            throw EnvError("unexpected seq type in eval");
                        }
                    } else {
                        // todo: add eval map
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
                                                    if (i > bindvec->items.size()-2) {
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
                                                    if (i > bindvec->items.size()-2) {
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
    internal[std::string(name)] = val;
}

void EnvImpl::setInternal(const atom::SymName* name,
    const atom::patom& val)
{
    internal[name->name] = val;
}

std::shared_ptr<Env> createEnv() {
    return std::make_shared<EnvImpl>();
}
