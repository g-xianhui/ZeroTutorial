#pragma once

#include <functional>
#include <unordered_map>

#include "iskill.h"

class SkillFactory {
private:
    std::unordered_map<int, std::function<ISkill* ()>> creators;

    SkillFactory() {}

public:

    static SkillFactory& instance() {
        static SkillFactory factory;
        return factory;
    }

    template <typename T>
    void reg(int id) {
        creators[id] = []() { return new T(); };
    }

    ISkill* create(int id) {
        auto it = creators.find(id);
        if (it != creators.end()) {
            return it->second();
        }
        return nullptr;
    }
};

#define REGISTER_SKILL(cls, id) \
    namespace { \
        struct AutoRegister_##cls { \
            AutoRegister_##cls() { \
                SkillFactory::instance().reg<cls>(id); \
            } \
        }; \
        static AutoRegister_##cls _autoRegister_##cls; \
    }