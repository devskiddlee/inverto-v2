#include "module_includes.h"

void esp_logic(Entity& entity, ImDrawList* drawList, int index, int max) {
	ViewMatrix currentVM = G::memory.Read<ViewMatrix>(G::client + G::offsets.viewmatrix);

	float hSTemp = 10000 / CalcMagnitude(G::localPlayer.origin, entity.origin) * (90.f / G::fov);

	ImColor imcolor = G::S.normalColor;

	if (entity.compare(G::render_nearest_player) && ((entity.angleDiff < G::S.maxAngleDiffAimbot * (90.f / G::fov)) || G::S.disableAngleDiff)) {
		imcolor = G::S.closestEnemy;
	}

	if (entity.visible) {
		imcolor = ImColor(255, 255, 255);
	}

	Vector head_screen_pos = Vector(-10000, -10000, 0);
	world_to_screen(entity.head, head_screen_pos, currentVM);
	drawList->AddCircle(ImVec2(head_screen_pos.x, head_screen_pos.y), hSTemp / 2, imcolor, 0, 2.f);

	if (G::S.directionTracer) {
		ImColor color = G::S.directionCrosshair;
		if (entity.compare(G::render_nearest_player))
			color = G::S.closestEnemy;
		if (entity.compare(G::render_nearest_player) && (GetAsyncKeyState(G::S.AIMBOT_KEY) < 0))
			color = G::S.aimLockedEnemy;

		Vector vec = entity.headScreenPos;

		double sin = vec.y - G::windowCenter.y;
		double cos = vec.x - G::windowCenter.x;
		double dist = sqrt(cos * cos + sin * sin);
		sin = sin / dist;
		cos = cos / dist;

		float angle = (float)atan2(sin, cos);

		float sx = cos * 10.f;
		float sy = sin * 10.f;

		float ex = cos * (10.f + G::S.directionTracerMaxLength);
		float ey = sin * (10.f + G::S.directionTracerMaxLength);

		if (dist > 10.f + G::S.directionTracerMaxLength && IsPixelInsideScreen(vec))
		{
			drawList->AddLine(
				{ G::windowCenter.x + sx, G::windowCenter.y + sy },
				{ G::windowCenter.x + ex, G::windowCenter.y + ey },
				color, 1.5f
			);
		}
		else if (dist > 10.f && dist < 10.f + G::S.directionTracerMaxLength && IsPixelInsideScreen(vec)) {
			drawList->AddLine(
				{ G::windowCenter.x + sx, G::windowCenter.y + sy },
				entity.headScreenPos.toVec2(),
				color, 1.5f
			);
		}
	}

	float hS = 10000 / CalcMagnitude(G::localPlayer.origin, entity.origin) * (90.f / G::fov);

	ImColor color = G::S.normalColor;
	if (entity.compare(G::render_nearest_player))
		color = G::S.closestEnemy;

	Vector vec = entity.headScreenPos.copy().operator-(Vector(hS / 2, hS / 2, 0));
	Vector feet = entity.originScreenPos;

	if (G::S.bone_esp) {
		for (auto& bone_connection : G::bone_connections) {
			Vector start = get_object_at_index<Vector>(entity.bone_screen_pos, bone_connection.first);
			if (!IsPixelInsideScreen(start))
				continue;
			
			for (auto& connection : bone_connection.second) {
				Vector end = get_object_at_index<Vector>(entity.bone_screen_pos, connection);

				if (!IsPixelInsideScreen(end))
					continue;

				drawList->AddLine(start.toVec2(), end.toVec2(), ImColor(100 + (155 * (index + 1) / max), 0, 100 + (155 * (index + 1) / max), 255), G::S.width);
			}
		}
	}

	if (G::S.boxEsp) {
		double angle = entity.angleEye.y / 180 * M_PI;

		double c = cos(angle);
		double s = sin(angle);

		float size = 15.f;
		draw3dBoxAroundLine(drawList, currentVM, entity.origin, entity.head.copy() + Vector(0, 0, 10), c, s, size);
	}

	if (G::S.chams) {
		std::vector<std::vector<ImVec2>> polygons;

		for (auto& bone_connection : G::bone_connections) {
			Vector start = get_object_at_index<Vector>(entity.bone_pos, bone_connection.first);

			for (auto& connection : bone_connection.second) {
				Vector end = get_object_at_index<Vector>(entity.bone_pos, connection);

				Vector cuboid_size(7.5f, 7.5f, 0);

				if (bone_connection.first == 3 || connection == 3)
					cuboid_size = Vector(10, 17.5f, 0);

				std::vector<ImVec2> sP;
				std::vector<Vector> temp_sP;
				bool valid = true;
				for (Vector pt : GetCuboidCorners(start, end, cuboid_size.x, cuboid_size.y)) {
					Vector p(-10000, -10000, 0);
					world_to_screen(pt, p, currentVM);
					temp_sP.push_back(p);
					if (!IsPixelInsideScreen(p))
						valid = false;
				}

				if (!valid)
					continue;

				std::vector<Vector> convex = GetConvexHull(temp_sP);

				for (auto pt : convex)
					sP.push_back(pt.toVec2());

				std::vector<ImVec2> out = sort_points_ccw(sP);
				out.push_back(out.front());
				polygons.push_back(out);
			}
		}

		for (auto& polygon : remove_intersections(polygons)) {
			drawList->AddPolyline(&polygon[0], (int)polygon.size(), G::S.chamsColor, ImDrawFlags_RoundCornersAll, G::S.chamsWidth);
		}
	}

	if ((G::S.show_only_nearest_info && entity.compare(G::render_nearest_player) && ((entity.angleDiff < G::S.maxAngleDiffAimbot * (90.f / G::fov)) || G::S.disableAngleDiff))
		|| !G::S.show_only_nearest_info) {
		float head_dependency = hS / 20;
		if (G::S.absolute_text_size)
			head_dependency = 1.f;
		float fontSize = 15.f * head_dependency;
		if (G::S.name) {
			Vector pos = Vector(-10000, -10000, 0);
			world_to_screen(entity.head.copy() + Vector(0, 0, 20), pos, currentVM);
			ImVec2 text_size = G::default_font->CalcTextSizeA(fontSize, 1000, 1000, entity.name.c_str());

			ImVec2 healthTextPos;

			std::ostringstream ss;
			ss << " ";
			ss << entity.health;
			ss << "/100";
			std::string health_str = ss.str();

			if (G::S.healthText) {
				ImVec2 health_text_size = G::default_font->CalcTextSizeA(fontSize, 1000, 1000, health_str.c_str());
				healthTextPos = pos.copy().operator-(Vector((text_size.x + health_text_size.x) / 2, max(text_size.y, health_text_size.y) / 2, 0)).operator+(Vector(text_size.x, 0, 0)).toVec2();

				text_size = ImVec2(text_size.x + health_text_size.x, max(text_size.y, health_text_size.y));
			}

			pos = pos.copy().operator-(Vector(text_size.x / 2, text_size.y / 2, 0));

			Vector text_pos = pos.copy().operator+(Vector(text_size.x, text_size.y, 0));
			drawList->AddRectFilled(pos.copy().operator-(Vector(G::S.text_padding, G::S.text_padding, 0)).toVec2(), text_pos.operator+(Vector(G::S.text_padding, G::S.text_padding, 0)).toVec2(), ImColor(0.f, 0.f, 0.f, 0.5f), G::S.text_padding);
			drawList->AddText(G::default_font, fontSize, pos.toVec2(), G::S.playerText, entity.name.c_str(), 0, 0.0f, 0);

			if (G::S.healthText) {
				drawList->AddText(G::default_font, fontSize, healthTextPos, G::S.health, health_str.c_str(), 0, 0.0f, 0);
			}
		}
		if (G::S.healthBox) {
			drawList->AddRectFilled(ImVec2({ (vec.x - 0.2f * hS) - 10.f , vec.y }), ImVec2({ vec.x - 10.f, vec.y + (feet.y - vec.y) / 100 * entity.health }), G::S.health);
		}
	}
}

class ESP {
public:
	static void OnRender(RenderEvent event) {
		ImDrawList* drawList = event.drawList;
		std::list<Entity> entities(G::render_entities);

		if (!G::S.disableAngleDiff)
			drawList->AddCircle(G::windowCenter.toVec2(), G::S.maxAngleDiffAimbot * (90.f / G::fov), ImColor(255, 255, 255, 200), 0, 1.5f);

		if (!G::S.esp)
			return;

		std::list<Entity> esp_entities_sorted_distance;

		for (int i = 0; i < 100; i++) {
			float closest_dist = 0.f;
			Entity temp_nearest_player;
			for (Entity& player : entities) {
				if (closest_dist < player.magnitude || closest_dist == 0.f) {
					closest_dist = player.magnitude;
					temp_nearest_player = player;
				}
			}
			if (closest_dist == 0.f)
				break;
			esp_entities_sorted_distance.push_back(temp_nearest_player);
			entities.remove(temp_nearest_player);
		}

		ViewMatrix currentVM = G::memory.Read<ViewMatrix>(G::client + G::offsets.viewmatrix);

		int ix = 0;
		for (Entity& player : esp_entities_sorted_distance) {
			if (G::S.espOnlyWhenVisible && !player.visible)
				continue;

			esp_logic(player, drawList, ix, (int)esp_entities_sorted_distance.size());
			ix++;
		}
	}
};