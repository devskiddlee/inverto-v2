/*
	Modular V0.1
	A library for modular handling of events made primarly for cheat clients

	https://github.com/devskiddlee/
*/
#include <imgui/imgui.h>
#include <list>
#include <thread>
#include <chrono>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <cstddef>
#include <memory>
#include <iostream>
#include <functional>
#include <Windows.h>

struct RenderEvent {
	ImDrawList* drawList;
	float last_draw_time;
};

struct TickEvent {
	float delta_time;
};

struct DelayedTask {
	std::chrono::high_resolution_clock::time_point issued;
	float delay = 0.f;
	std::function<void(TickEvent event)> fn;
	const char* id = "";
};

typedef void (*RenderEventHandler)(RenderEvent event);
typedef void (*TickEventHandler)(TickEvent event);

struct KeyEventHandler {
	std::function<void(bool)> callback;
	int VK;
	bool pressed;
};

struct KeyEventHandlerPtr {
	std::function<void(bool)> callback;
	int* VK;
	bool pressed;
};

class Modular {
private:
	inline static std::list<RenderEventHandler> render_event_handlers;
	inline static std::list<TickEventHandler> tick_event_handlers;
	inline static std::list<KeyEventHandler> key_event_handlers;
	inline static std::list<KeyEventHandlerPtr> key_ptr_event_handlers;

	inline static std::list<DelayedTask> delayed_task_requests;
	inline static std::list<DelayedTask> delayed_tasks;

	inline static bool keep_alive_tick_loop;
	inline static std::thread tick_thread;
	inline static float avg_tick_time;

	static void tick_loop() {
		float last_delta_time = 0.f;
		float tick_time = 0.f;
		int ticks = 0;
		while (keep_alive_tick_loop) {
			auto start = std::chrono::high_resolution_clock::now();

			TickEvent event;
			event.delta_time = last_delta_time / 1000;

			for (auto task : delayed_task_requests) {
				delayed_tasks.push_back(task);
			}
			delayed_task_requests.clear();

			for (auto it = delayed_tasks.begin(); it != delayed_tasks.end(); ) {
				DelayedTask task = *it;
				if (std::chrono::duration<float, std::milli>(start - task.issued).count() > task.delay) {
					task.fn(event);
					it = delayed_tasks.erase(it);
				}
				else {
					++it;
				}
			}

			CallTickEvent(event);

			auto end = std::chrono::high_resolution_clock::now();

			last_delta_time = std::chrono::duration<float, std::milli>(end - start).count();

			ticks++;
			tick_time += last_delta_time;
			if (tick_time > 1000) {
				avg_tick_time = tick_time / ticks;
				tick_time = 0;
				ticks = 0;
			}
		}
	}
public:
	static void AddKeyEventHandler(int VK, std::function<void(bool)> callback) {
		KeyEventHandler handler;
		handler.callback = callback;
		handler.VK = VK;
		key_event_handlers.push_back(handler);
	}

	static void AddKeyEventHandler(int* VK, std::function<void(bool)> callback) {
		KeyEventHandlerPtr handler;
		handler.callback = callback;
		handler.VK = VK;
		key_ptr_event_handlers.push_back(handler);
	}

	static float GetAverageTickTime() {
		return Modular::avg_tick_time;
	}

	static void ScheduleDelayedTask(float delay, const char* id, std::function<void(TickEvent event)> fn) {
		DelayedTask task;
		task.issued = std::chrono::high_resolution_clock::now();
		task.delay = delay;
		task.fn = fn;
		task.id = id;
		delayed_task_requests.push_back(task);
	}

	static void AddRenderEventHandler(RenderEventHandler handler) {
		render_event_handlers.push_back(handler);
	}

	static void AddTickEventHandler(TickEventHandler handler) {
		tick_event_handlers.push_back(handler);
	}

	static void CallRenderEvent(RenderEvent event) {
		for (auto handler : render_event_handlers) {
			handler(event);
		}
	};

	static void CallTickEvent(TickEvent event) {
		
		for (auto &handler : key_event_handlers) {
			if (GetAsyncKeyState(handler.VK) && !handler.pressed) {
				handler.pressed = true;
				handler.callback(true);
			}
			if (!GetAsyncKeyState(handler.VK) && handler.pressed) {
				handler.pressed = false;
				handler.callback(false);
			}
		}

		for (auto& handler : key_ptr_event_handlers) {
			if (GetAsyncKeyState(*handler.VK) && !handler.pressed) {
				handler.pressed = true;
				handler.callback(true);
			}
			if (!GetAsyncKeyState(*handler.VK) && handler.pressed) {
				handler.pressed = false;
				handler.callback(false);
			}
		}

		for (auto handler : tick_event_handlers) {
			handler(event);
		}
	};

	static void StartTickLoop() {
		keep_alive_tick_loop = true;
		tick_thread = std::thread(tick_loop);
	};

	static void StopTickLoop() {
		keep_alive_tick_loop = false;
		tick_thread.join();
	}
};