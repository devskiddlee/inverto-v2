#include "module_includes.h"

class PlantedC4 {
private:
	static ImColor interpolateColor(ImColor col1, ImColor col2, float fraction) {
		return ImColor(
			(col2.Value.x - col1.Value.x) * fraction + col1.Value.x,
			(col2.Value.y - col1.Value.y) * fraction + col1.Value.y,
			(col2.Value.z - col1.Value.z) * fraction + col1.Value.z,
			(col2.Value.w - col1.Value.w) * fraction + col1.Value.w
		);
	}

	static float getBombDamage(const char* map, bool* out_unsure) {
		switch (Hash(map)) {
			case Hash("de_poseidon"):
				return 423.f;
			case Hash("de_anubis"):
				return 450.f;
			case Hash("de_inferno"):
				return 600.f;
			case Hash("de_mirage"):
			case Hash("de_ancient"):
			case Hash("de_stronghold"):
			case Hash("de_overpass"):
			case Hash("de_nuke"):
				return 650.f;
			case Hash("de_dust2"):
				return 700.f;
			default:
				*out_unsure = true;
			case Hash("de_sanctum"):
			case Hash("de_warden"):
			case Hash("de_vertigo"):
			case Hash("de_train"):
				return 500.f;
		}
	}

	static float calculateArmorModifier(float damage, uint32_t armor) {
		if (armor > 0) {
			const float armor_ratio = 0.5f;
			const float armor_bonus = 0.5f;
			float armor_ratio_multiply = damage * armor_ratio;
			float actual = (damage - armor_ratio_multiply) * armor_bonus;

			if (actual > (float)(armor)) {
				actual = (float)(armor) * (1.f / armor_bonus);
				armor_ratio_multiply = damage - actual;
			}

			damage = armor_ratio_multiply;
		}
		return damage;
	}

	static int calculateBombDamage(Vector c4origin, bool* unsure) {
		const int bomb_damage = getBombDamage(G::mapName.c_str(), unsure);
		const int bomb_radius = bomb_damage * 3.5f;
		const double c = bomb_radius / 3.0;

		uint32_t armor = G::memory.Read<uint32_t>(G::localPlayer.address + G::offsets.m_ArmorValue);
		float dist = CalcMagnitude(c4origin, G::localPlayer.origin);
		const float damage = bomb_damage * std::exp(-std::pow(dist, 2) / (2 * std::pow(c, 2)));
		const float damage_armor = calculateArmorModifier(damage, armor);

		return (int)(std::floor(damage_armor));

	}
public:
	static void OnRender(const RenderEvent& event) {
		if (!G::S.c4_esp) return;

		uintptr_t c4_ptr = G::memory.Read<uintptr_t>(
			G::memory.Read<uintptr_t>(G::client + G::offsets.planted_c4)
		);
		if (!c4_ptr) return;
		uintptr_t c4node_ptr = G::memory.Read<uintptr_t>(c4_ptr + G::offsets.gameScene);
		if (!c4node_ptr) return;
		Vector c4origin = G::memory.Read<Vector>(c4node_ptr + G::offsets.m_vecAbsOrigin);
		if (c4origin.length() == 0) return;

		CGlobalVarsBase globalVars = G::memory.Read<CGlobalVarsBase>(G::memory.Read<uintptr_t>(G::client + G::offsets.globalVars));

		float m_flC4Blow = G::memory.Read<float>(c4_ptr + G::offsets.m_flC4Blow);
		float bombTimer = m_flC4Blow - globalVars.m_flCurrentTime;
		float normalizedBombTimer = bombTimer / 40.f;
		bool m_bBombTicking = G::memory.Read<bool>(c4_ptr + G::offsets.m_bBombTicking);
		bool m_bHasExploded = G::memory.Read<bool>(c4_ptr + G::offsets.m_bHasExploded);
		bool m_bC4Activated = G::memory.Read<bool>(c4_ptr + G::offsets.m_bC4Activated);
		bool m_bBombDefused = G::memory.Read<bool>(c4_ptr + G::offsets.m_bBombDefused);

		if ( m_bHasExploded || !m_bC4Activated  || m_bBombDefused  ) return;
		if (!m_bBombTicking || bombTimer <= 0.f || bombTimer > 40.f) return;

		if (G::S.c4_esp_show_duration) {
			bool unsure = false;
			int bombDmg = calculateBombDamage(c4origin, &unsure);
			bool displayDamage = !unsure && G::S.c4_esp_show_damage;

			ImColor color = interpolateColor(ImColor(255, 0, 0), ImColor(0, 255, 0), normalizedBombTimer);
			ImColor survive = (bombDmg < G::localPlayer.health) ? ImColor(0, 255, 0) : ImColor(255, 0, 0);
			if (!displayDamage) survive = color;

			const float width = 300.f;
			const float height = G::windowSize.y - 180.f;
			const float padding = 10.f;
			const float text_size = 30.f;

			std::string bombTimerStr = str(std::round(bombTimer)) + "s";
			if (displayDamage) bombTimerStr += "  -" + str(bombDmg) + " HP";
			float text_width = G::default_font->CalcTextSizeA(text_size, 1000, 1000, bombTimerStr.c_str()).x;

			event.drawList->AddRectFilled(
				{ G::windowSize.x / 2 - width / 2 - padding, height - padding },
				{ G::windowSize.x / 2 - width / 2 + width + padding, height + text_size + 10.f + 2 * padding },
				ImColor(0, 0, 0, 170)
			);

			event.drawList->AddText(
				G::default_font,
				text_size,
				{ G::windowSize.x / 2 - text_width / 2, height },
				survive,
				bombTimerStr.c_str()
			);

			event.drawList->AddRectFilled(
				{ G::windowSize.x / 2 - width / 2, height + text_size + padding },
				{ G::windowSize.x / 2 - width / 2 + normalizedBombTimer * width, height + text_size + 10.f + padding },
				color
			);
		}

		ViewMatrix currentVM = G::memory.Read<ViewMatrix>(G::client + G::offsets.viewmatrix);

		if (G::S.c4_cross) {
			draw3dCross(
				event.drawList, currentVM,
				c4origin,
				10.f,
				G::S.c4_line_width, G::S.c4_color
			);
			return;
		}

		draw3dBoxAroundLine(
			event.drawList, currentVM,
			c4origin.copy() + Vector(0, 0, 5),
			c4origin.copy() - Vector(0, 0, 5),
			1.0, 0.0, 5.f,
			G::S.c4_line_width, G::S.c4_color
		);
	}
};