#pragma once

#include <functional>
#include <chrono>
#include <unordered_map>

struct TimerTask {
    using TimerCallback = std::function<void()>;
    TimerTask(size_t i, size_t ms, size_t fr, TimerCallback cb, bool repeat) : interval(i), timeout_ms(ms), timeout_frame(fr), callback(cb), is_repeat(repeat) {}
    size_t interval = 0;
    size_t timeout_ms = 0;
    size_t timeout_frame = 0;
    TimerCallback callback;
    bool is_repeat = false;
    bool is_canceled = false;
};

class WheelTimer {
public:
    struct Wheel {
        using Slot = std::vector<TimerTask*>;
        size_t cur_slot_index = 0;
        std::vector<Slot> slots;
    };

    static const size_t WHEEL_COUNT = 3;

    WheelTimer(size_t interval, size_t max_slots);
    ~WheelTimer();

    void start();
    void update();
    void tick();

    int add_timer(size_t ms, std::function<void()> cb, bool is_repeat = false);
    void del_timer(int timer_id);
    inline size_t ms_since_start() { return ms_since_start_; }

private:
    void move_timers(size_t wheel_index, size_t slot_index);
    void run_timers(size_t wheel_index, size_t slot_index);

    bool calculate_slot_index(size_t frame_count, size_t& wheel_index, size_t& slot_index);

private:
    size_t cur_frame_ = 0;
    // 33ms per tick
    size_t interval_ = 33;
    size_t max_slots_ = 1024;
    int next_timer_id_ = 0;

    Wheel wheels_[WHEEL_COUNT];
    std::unordered_map<int, TimerTask*> id_2_timer_task_;

    size_t left_ms_ = 0;
    size_t ms_since_start_ = 0;
    std::chrono::steady_clock::time_point last_update_clock_;
};

extern WheelTimer G_Timer;