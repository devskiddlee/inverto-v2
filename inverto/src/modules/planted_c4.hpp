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
public:
	static void OnRender(RenderEvent event) {
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

		// not an actual bomb
		if (bombTimer <= 0.f || bombTimer > 40.f) return;
		float normalizedBombTimer = bombTimer / 40.f;

		ImColor color = interpolateColor(ImColor(255, 0, 0), ImColor(0, 255, 0), normalizedBombTimer);

		const float width = 300.f;
		const float height = G::windowSize.y - 180.f;
		const float padding = 10.f;
		const float text_size = 30.f;

		std::string bombTimerStr = str(std::round(bombTimer)) + "s";
		float text_width = G::default_font->CalcTextSizeA(text_size, 1000, 1000, bombTimerStr.c_str()).x;

		event.drawList->AddRectFilled(
			{ G::windowSize.x / 2 - width / 2 - padding, height - padding },
			{ G::windowSize.x / 2 - width / 2 + width + padding, height + text_size + 10.f + 2 * padding },
			ImColor(0, 0, 0, 170)
		);

		event.drawList->AddText(G::default_font, text_size, { G::windowSize.x / 2 - text_width / 2, height }, color, bombTimerStr.c_str());

		event.drawList->AddRectFilled(
			{ G::windowSize.x / 2 - width / 2, height + text_size + padding },
			{ G::windowSize.x / 2 - width / 2 + normalizedBombTimer * width, height + text_size + 10.f + padding },
			color
		);

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