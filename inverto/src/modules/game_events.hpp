#include "module_includes.h"

class GameEvents {
private:
	inline static int kills = 0;
	inline static float kill_animation = 0.f;

	static void OnPlayerKill() {
		kill_animation = G::S.kill_animation_duration;
	}
public:
	static void OnTick(TickEvent event) {
		uintptr_t ats = G::memory.Read<uintptr_t>(G::localPlayerController + G::offsets.m_pActionTrackingServices);
		CSMatchStats stats = G::memory.Read<CSMatchStats>(ats + G::offsets.m_matchStats);
		int kill_count = stats.kills;

		if (
			kill_count != kills
		) {
			if (kill_count > kills)
				OnPlayerKill();
			kills = kill_count;
		}
	}

	static void OnRender(RenderEvent event) {
		float t = kill_animation / G::S.kill_animation_duration;

		if (t == 0.f || !G::S.kill_animation)
			return;

		// formula: -4(x-0.5)^{2}+1 [https://www.desmos.com/calculator/rh96esvzs8]
		float y = -4 * pow(t - 0.5f, 2) + 1;

		ImColor color = G::S.kill_animation_color;

		for (int i = 0; i < G::S.kill_animation_size * y; i++) {
			event.drawList->AddRect(
				{ (float)i, (float)i },
				{ G::windowSize.x - i, G::windowSize.y - i },
				ImColor(color.Value.x, color.Value.y, color.Value.z, (G::S.kill_animation_size - i) / (float)G::S.kill_animation_size * y)
			);
		}

		kill_animation = max(0.f, kill_animation - event.last_draw_time);
	}
};