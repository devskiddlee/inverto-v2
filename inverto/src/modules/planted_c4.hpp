#include "module_includes.h"

class PlantedC4 {
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