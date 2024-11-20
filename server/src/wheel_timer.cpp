#include "wheel_timer.h"

#include <algorithm>
#include <cassert>

WheelTimer::WheelTimer(size_t interval, size_t max_slots) : interval_(interval), max_slots_(max_slots) {
	for (size_t i = 0; i < WHEEL_COUNT; i++) {
		wheels_[i].slots.resize(max_slots);
	}
}

WheelTimer::~WheelTimer()
{
    for (size_t i = 0; i < WHEEL_COUNT; i++) {
        for (Wheel::Slot& slot : wheels_[i].slots) {
            for (TimerTask* task : slot) {
                delete task;
            }
        }
    }
}

void WheelTimer::start()
{
	last_update_clock_ = std::chrono::steady_clock::now();
}

void WheelTimer::update() {
	std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_update_clock_);
	left_ms_ += duration.count();
    ms_since_start_ += duration.count();

	while (left_ms_ >= interval_) {
		left_ms_ -= interval_;
		cur_frame_ += 1;
		tick();
	}
	last_update_clock_ = std::chrono::steady_clock::now();
}

// 每过一个interval_时间调用一次，起始第0个slot不会被执行，但由于add_timer确保了至少延后一帧，所以在第一次tick之前添加的timer起码都在第1个slot中，不会被漏掉。
void WheelTimer::tick() {
	for (size_t i = 0; i < WHEEL_COUNT; i++) {
		Wheel& wheel = wheels_[i];
		wheel.cur_slot_index = (wheel.cur_slot_index + 1) % max_slots_;

		if (i > 0) {
			move_timers(i, wheel.cur_slot_index);
		}
		else {
			run_timers(i, wheel.cur_slot_index);
		}

		if (wheel.cur_slot_index != 0)
			break;
	}
}

int WheelTimer::add_timer(size_t ms, std::function<void()> cb, bool is_repeat)
{
	size_t wheel_index = 0;
	size_t slot_index = 0;
	// 至少延后一帧
	size_t delay_frame_count = std::max(ms / interval_, (size_t)1);
	if (calculate_slot_index(delay_frame_count, wheel_index, slot_index)) {
		TimerTask* task = new TimerTask{ ms, cur_frame_ * interval_ + ms, cur_frame_ + delay_frame_count, cb, is_repeat };
		wheels_[wheel_index].slots[slot_index].push_back(task);

		int timer_id = next_timer_id_++;
		auto result = id_2_timer_task_.insert(std::make_pair(timer_id, task));
		assert(result.second);
		return timer_id;
	}
	else {
		return -1;
	}
}

void WheelTimer::del_timer(int timer_id)
{
	auto iter = id_2_timer_task_.find(timer_id);
	if (iter != id_2_timer_task_.end()) {
		TimerTask* task = iter->second;
		task->is_canceled = true;
		id_2_timer_task_.erase(iter);
	}
}

void WheelTimer::move_timers(size_t wheel_index, size_t slot_index)
{
	Wheel& wheel = wheels_[wheel_index];
	Wheel::Slot& slot = wheel.slots[slot_index];

	size_t new_wheel_index = 0;
	size_t new_slot_index = 0;
	for (TimerTask* task : slot) {
		size_t delay_frame_count = task->timeout_frame - cur_frame_;
		if (calculate_slot_index(delay_frame_count, new_wheel_index, new_slot_index)) {
			wheels_[new_wheel_index].slots[new_slot_index].push_back(task);
		}
		else {
			assert(0);
		}
	}
	slot.clear();
}

void WheelTimer::run_timers(size_t wheel_index, size_t slot_index)
{
	Wheel& wheel = wheels_[wheel_index];
	Wheel::Slot& slot = wheel.slots[slot_index];
	std::sort(slot.begin(), slot.end(), [](TimerTask* a, TimerTask* b) {
		return a->timeout_ms < b->timeout_ms;
		});

	size_t new_wheel_index = 0;
	size_t new_slot_index = 0;
	for (TimerTask* task : slot) {
		if (task->is_canceled) {
			delete task;
			continue;
		}

		task->callback();
		if (task->is_repeat) {
			size_t delay_frame_count = std::max(task->interval / interval_, (size_t)1);
			if (calculate_slot_index(delay_frame_count, new_wheel_index, new_slot_index)) {
				task->timeout_frame = cur_frame_ + delay_frame_count;
				task->timeout_ms = cur_frame_ * interval_ + task->interval;
				wheels_[new_wheel_index].slots[new_slot_index].push_back(task);
			}
			else {
				assert(0);
			}
		}
		else {
			delete task;
		}
	}
	slot.clear();
}

bool WheelTimer::calculate_slot_index(size_t frame_count, size_t& wheel_index, size_t& slot_index)
{
	size_t slot_count = frame_count;
	for (size_t i = 0; i < WHEEL_COUNT; i++) {
		if (slot_count >= max_slots_ && i < WHEEL_COUNT - 1) {
			slot_count = slot_count / max_slots_;
		}
		else {
			wheel_index = i;
			break;
		}
	}

	if (slot_count >= max_slots_)
		return false;

	slot_index = (wheels_[wheel_index].cur_slot_index + slot_count) % max_slots_;
	return true;
}
