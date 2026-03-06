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

#define TIME_POINT std::chrono::high_resolution_clock::time_point
#define TIME_PERIOD std::chrono::high_resolution_clock::duration
#define NOW std::chrono::high_resolution_clock::now()
#define TIME_SINCE(p) std::chrono::duration<float, std::milli>(NOW - p).count()
#define SLEEP(t) std::this_thread::sleep_for(std::chrono::duration<float, std::milli>(t))

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
	std::function<void(const TickEvent& event)> fn;
	const char* id = "";
};

typedef void (*RenderEventHandler)(const RenderEvent& event);
typedef void (*TickEventHandler)(const TickEvent& event);

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

struct TimeReport {
	float lastTickTimes[8];
	float avgTime;
	size_t currentTickTimeIndex;
};

class Modular {
private:
	inline static std::vector<RenderEventHandler> render_event_handlers;
	inline static std::vector<TickEventHandler> tick_event_handlers;
	inline static std::vector<KeyEventHandler> key_event_handlers;
	inline static std::vector<KeyEventHandlerPtr> key_ptr_event_handlers;

	inline static std::vector<DelayedTask> delayed_tasks;

	inline static bool keep_alive_tick_loop;
	inline static std::thread tick_thread;
	inline static float avg_tick_time;

	inline static bool debug_tick_speed = false;
	inline static std::vector<TimeReport> tickTimeReports;
	inline static bool debug_render_speed = false;
	inline static std::vector<TimeReport> renderTimeReports;

	inline static TIME_POINT nextTickTime = NOW;
	inline static int tickCap = -1;

	static void tick_loop() {
		TIME_POINT lastTickTP = NOW;
		#define LAST_TICK_TIME_SIZE 64
		float lastTickTimes[LAST_TICK_TIME_SIZE]{ 0 };
		size_t currentTickTimeIndex = 0;

		while (keep_alive_tick_loop) {
			float lastTickTime = TIME_SINCE(lastTickTP);
			lastTickTP = NOW;

			if (lastTickTime > 0.f) {
				lastTickTimes[currentTickTimeIndex++] = lastTickTime;
				if (currentTickTimeIndex == LAST_TICK_TIME_SIZE) {
					float all = 0.f;
					for (size_t i = 0; i < LAST_TICK_TIME_SIZE; i++) {
						all += lastTickTimes[i];
					}
					avg_tick_time = all / LAST_TICK_TIME_SIZE;
					currentTickTimeIndex = 0;
				}
			}

			TickEvent event;
			event.delta_time = lastTickTime / 1000.f;

			for (auto it = delayed_tasks.begin(); it != delayed_tasks.end(); ) {
				if (TIME_SINCE(it->issued) > it->delay) {
					it->fn(event);
					it = delayed_tasks.erase(it);
				}
				else {
					++it;
				}
			}

			CallTickEvent(event);

			if (tickCap > 0) {
				nextTickTime += std::chrono::milliseconds((int)(1000.f / tickCap));
				std::this_thread::sleep_until(nextTickTime);
			}
		}
	}
public:
	static void EnableTickSpeedDebugging() {
		debug_tick_speed = true;
	}

	static void EnableRenderSpeedDebugging() {
		debug_render_speed = true;
	}

	static void SetTickCap(int cap) {
		nextTickTime = NOW;
		tickCap = cap;
	}

	static void GetTickTimeReports(TimeReport** out_rep, size_t* out_size) {
		if (!debug_tick_speed) return;
		*out_rep = tickTimeReports.data();
		*out_size = tickTimeReports.size();
	}

	static void GetRenderTimeReports(TimeReport** out_rep, size_t* out_size) {
		if (!debug_render_speed) return;
		*out_rep = renderTimeReports.data();
		*out_size = renderTimeReports.size();
	}

	static void AddKeyEventHandler(int VK, std::function<void(bool)> callback) {
		key_event_handlers.emplace_back(callback, VK, false);
	}

	static void AddKeyEventHandler(int* VK, std::function<void(bool)> callback) {
		key_ptr_event_handlers.emplace_back(callback, VK, false);
	}

	static float GetAverageTickTime() {
		return Modular::avg_tick_time;
	}

	static void ScheduleDelayedTask(float delay, const char* id, std::function<void(const TickEvent& event)> fn) {
		delayed_tasks.emplace_back(NOW, delay, fn, id);
	}

	static void AddRenderEventHandler(const RenderEventHandler& handler) {
		render_event_handlers.push_back(handler);
	}

	static void AddTickEventHandler(const TickEventHandler& handler) {
		tick_event_handlers.push_back(handler);
	}

	static void CallRenderEvent(const RenderEvent& event) {
		size_t id = 0;
		for (auto& handler : render_event_handlers) {
			if (debug_render_speed) {
				TimeReport* rep = &renderTimeReports.at(id);

				TIME_POINT start = NOW;
				handler(event);
				rep->lastTickTimes[rep->currentTickTimeIndex++] = TIME_SINCE(start);
				if (rep->currentTickTimeIndex == 8) {
					float all = 0.f;
					for (size_t i = 0; i < 8; i++) {
						all += rep->lastTickTimes[i];
					}
					rep->avgTime = all / 8;
					rep->currentTickTimeIndex = 0;
				}

				id++;
				continue;
			}

			handler(event);
		}
	};

	static void CallTickEvent(const TickEvent& event) {
		for (auto& handler : key_event_handlers) {
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

		size_t id = 0;
		for (auto& handler : tick_event_handlers) {
			if (debug_tick_speed) {
				TimeReport* rep = &tickTimeReports.at(id);

				TIME_POINT start = NOW;
				handler(event);
				rep->lastTickTimes[rep->currentTickTimeIndex++] = TIME_SINCE(start);
				if (rep->currentTickTimeIndex == 8) {
					float all = 0.f;
					for (size_t i = 0; i < 8; i++) {
						all += rep->lastTickTimes[i];
					}
					rep->avgTime = all / 8;
					rep->currentTickTimeIndex = 0;
				}
				
				id++;
				continue;
			}
			
			handler(event);
		}
	};

	static void StartTickLoop() {
		if (debug_tick_speed)
			tickTimeReports.resize(tick_event_handlers.size());
		if (debug_render_speed)
			renderTimeReports.resize(render_event_handlers.size());

		keep_alive_tick_loop = true;
		tick_thread = std::thread(tick_loop);
	};

	static void StopTickLoop() {
		keep_alive_tick_loop = false;
		tick_thread.join();
	}
};