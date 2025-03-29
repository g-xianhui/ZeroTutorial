#pragma once

class CombatComponent;

class ISkill {
public:
    virtual ~ISkill() = default;

    virtual void destroy() {}
    virtual void execute() = 0;

    void set_owner(CombatComponent* owner) { _owner = owner; }
    CombatComponent* get_owner() { return _owner; }

    inline void set_active(bool active) { _is_active = active; }
    inline bool is_active() { return _is_active; }

private:
    bool _is_active = true;
    CombatComponent* _owner = nullptr;
};