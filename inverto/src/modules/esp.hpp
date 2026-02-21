#include "module_includes.h"

struct EspText {
	std::string content;
	ImColor color;
	float fontSize = 0;
	ImVec2 contentSize;

	EspText(const std::string& _content, ImColor _color, float _fontSize, ImFont* font) {
		content = _content;
		color = _color;
		fontSize = _fontSize;
		contentSize = font->CalcTextSizeA(fontSize, _HUGE_ENUF, _HUGE_ENUF, _content.c_str());
	}
};

Vector GetBone(Entity* pawn, int b) {
	uintptr_t sceneNode = G::memory.Read<uintptr_t>(pawn->address + G::offsets.gameScene);
	uintptr_t boneMatrix = G::memory.Read<uintptr_t>(sceneNode + G::offsets.modelState + G::offsets.boneArray);
	return G::memory.Read<Vector>(boneMatrix + (uint64_t)(b) * 32);
}

bool GetBonesScreenPos(Entity* pawn, std::vector<ImVec2>& out, const ViewMatrix& vm) {
	Vector p;
	#define ADD_BONE(b) if (!world_to_screen(GetBone(pawn, b), p, vm)) return false; out[b] = p.toVec2();

	ADD_BONE(Head);
	ADD_BONE(Neck);
	ADD_BONE(UpperChest);
	ADD_BONE(LowerChest);
	ADD_BONE(Stomach);
	ADD_BONE(Pelvis);
	ADD_BONE(LeftShoulder);
	ADD_BONE(LeftElbow);
	ADD_BONE(LeftArm);
	ADD_BONE(RightShoulder);
	ADD_BONE(RightElbow);
	ADD_BONE(RightArm);
	ADD_BONE(LeftThigh);
	ADD_BONE(LeftKnee);
	ADD_BONE(LeftLeg);
	ADD_BONE(RightThigh);
	ADD_BONE(RightKnee);
	ADD_BONE(RightLeg);

	return true;
}

bool AddPolygon(Entity* pawn, const ViewMatrix& vm, int b1, int b2, const Vector cuboid_size, std::vector<std::vector<ImVec2>>* polygons) {
	Vector start = GetBone(pawn, b1);
	Vector end   = GetBone(pawn, b2);

	std::vector<ImVec2> sP;
	std::vector<Vector> temp_sP;
	for (Vector pt : GetCuboidCorners(start, end, cuboid_size.x, cuboid_size.y)) {
		Vector p;
		if (!world_to_screen(pt, p, vm))
			return false;
		temp_sP.push_back(p);
	}

	std::vector<Vector> convex = GetConvexHull(temp_sP);

	for (auto& pt : convex)
		sP.push_back(pt.toVec2());

	std::vector<ImVec2> out = sort_points_ccw(sP);
	out.push_back(out.front());
	polygons->push_back(out);

	return true;
}

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
		std::vector<ImVec2> bones(32);
		if (!GetBonesScreenPos(&entity, bones, currentVM)) goto skip_bone_esp;

		#define DRAW_BONE_CONNECTION(b1, b2) drawList->AddLine(bones[b1], bones[b2], G::S.boneColor, G::S.width);

		// Torso
		DRAW_BONE_CONNECTION(Head, Neck);
		DRAW_BONE_CONNECTION(Neck, UpperChest);
		DRAW_BONE_CONNECTION(UpperChest, LowerChest);
		DRAW_BONE_CONNECTION(LowerChest, Stomach);
		DRAW_BONE_CONNECTION(Stomach, Pelvis);

		// Left Arm
		DRAW_BONE_CONNECTION(UpperChest, LeftShoulder);
		DRAW_BONE_CONNECTION(LeftShoulder, LeftElbow);
		DRAW_BONE_CONNECTION(LeftElbow, LeftArm);

		// Right Arm
		DRAW_BONE_CONNECTION(UpperChest, RightShoulder);
		DRAW_BONE_CONNECTION(RightShoulder, RightElbow);
		DRAW_BONE_CONNECTION(RightElbow, RightArm);

		// Left Leg
		DRAW_BONE_CONNECTION(Pelvis, LeftThigh);
		DRAW_BONE_CONNECTION(LeftThigh, LeftKnee);
		DRAW_BONE_CONNECTION(LeftKnee, LeftLeg);

		// Right Leg
		DRAW_BONE_CONNECTION(Pelvis, RightThigh);
		DRAW_BONE_CONNECTION(RightThigh, RightKnee);
		DRAW_BONE_CONNECTION(RightKnee, RightLeg);
	}
	skip_bone_esp:

	if (G::S.boxEsp) {
		double angle = entity.angleEye.y / 180 * M_PI;

		double c = cos(angle);
		double s = sin(angle);

		float size = 15.f;
		draw3dBoxAroundLine(drawList, currentVM, entity.origin, entity.head.copy() + Vector(0, 0, 10), c, s, size, G::S.boxEspWidth, G::S.boxColor);
	}

	if (G::S.chams) {
		std::vector<std::vector<ImVec2>> polygons;

		#define ADD_LIMB(b1, b2, size) if (!AddPolygon(&entity, currentVM, b1, b2, size, &polygons)) goto skip_chams;

		// Torso
		ADD_LIMB(Head, Neck, Vector(7.5f, 7.5f, 0));
		ADD_LIMB(Neck, UpperChest, Vector(7.5f, 7.5f, 0));
		ADD_LIMB(UpperChest, LowerChest, Vector(10, 17.5f, 0));
		ADD_LIMB(LowerChest, Stomach, Vector(10, 17.5f, 0));
		ADD_LIMB(Stomach, Pelvis, Vector(7.5f, 7.5f, 0));

		// Left Arm
		ADD_LIMB(UpperChest, LeftShoulder, Vector(7.5f, 7.5f, 0));
		ADD_LIMB(LeftShoulder, LeftElbow, Vector(7.5f, 7.5f, 0));
		ADD_LIMB(LeftElbow, LeftArm, Vector(7.5f, 7.5f, 0));

		// Right Arm
		ADD_LIMB(UpperChest, RightShoulder, Vector(7.5f, 7.5f, 0));
		ADD_LIMB(RightShoulder, RightElbow, Vector(7.5f, 7.5f, 0));
		ADD_LIMB(RightElbow, RightArm, Vector(7.5f, 7.5f, 0));

		// Left Leg
		ADD_LIMB(Pelvis, LeftThigh, Vector(7.5f, 7.5f, 0));
		ADD_LIMB(LeftThigh, LeftKnee, Vector(7.5f, 7.5f, 0));
		ADD_LIMB(LeftKnee, LeftLeg, Vector(7.5f, 7.5f, 0));

		// Right Leg
		ADD_LIMB(Pelvis, RightThigh, Vector(7.5f, 7.5f, 0));
		ADD_LIMB(RightThigh, RightKnee, Vector(7.5f, 7.5f, 0));
		ADD_LIMB(RightKnee, RightLeg, Vector(7.5f, 7.5f, 0));

		for (auto& polygon : remove_intersections(polygons)) {
			drawList->AddPolyline(polygon.data(), (int)polygon.size(), G::S.chamsColor, ImDrawFlags_RoundCornersAll, G::S.chamsWidth);
		}
	}
	skip_chams:

	if ((G::S.show_only_nearest_info && entity.compare(G::render_nearest_player) && ((entity.angleDiff < G::S.maxAngleDiffAimbot * (90.f / G::fov)) || G::S.disableAngleDiff))
		|| !G::S.show_only_nearest_info) {
		float head_dependency = hS / 20;
		if (G::S.absolute_text_size)
			head_dependency = 1.f;

		Vector pos = Vector(-10000, -10000, 0);
		if (world_to_screen(entity.head.copy() + Vector(0, 0, 20), pos, currentVM)) {
			const float padding = 2.f;
			const float spacing = 10.f;
			std::vector<EspText> espText;

			float spacing_sized = spacing * head_dependency;
			float padding_sized = padding * head_dependency;

			// TOP PLAYER TEXT

			// Player Name
			if (G::S.name)
				espText.emplace_back(
					entity.name,
					G::S.playerText,
					15.f * head_dependency,
					G::default_font
				);

			// Player Health
			if (G::S.healthText)
				espText.emplace_back(
					str(entity.health),
					G::S.health,
					15.f * head_dependency,
					G::default_font
				);

			// Player Armor
			if (G::S.armorText) {
				uint32_t armor = G::memory.Read<uint32_t>(entity.address + G::offsets.m_ArmorValue);

				espText.emplace_back(
					str(armor),
					G::S.armorTextColor,
					15.f * head_dependency,
					G::default_font
				);
			}

			ImVec2 size;
			for (auto& eT : espText) {
				size.x += eT.contentSize.x + spacing_sized;
				size.y = max(eT.contentSize.y, size.y);
			}
			size.x -= spacing_sized; // Remove last spacing


			ImVec2 curCursor(
				pos.x - size.x / 2.f,
				pos.y - size.y / 2.f
			);

			drawList->AddRectFilled(
				{ curCursor.x - padding_sized * 2.f	,			curCursor.y - padding_sized				},
				{ curCursor.x + size.x + padding_sized * 2.f,	curCursor.y + size.y + padding_sized	},
				ImColor(0.f, 0.f, 0.f, 0.5f)
			);

			for (auto& eT : espText) {
				drawList->AddText(
					G::default_font,
					eT.fontSize,
					{ curCursor.x, curCursor.y + size.y - eT.contentSize.y },
					eT.color,
					eT.content.c_str()
				);
				curCursor.x += eT.contentSize.x + spacing_sized;
			}

			// BOTTOM PLAYER TEXT

			if (world_to_screen(entity.origin.copy() - Vector(0, 0, 20), pos, currentVM)) {
				espText.clear();

				// Equipped Weapon
				if (G::S.weaponText) {
					uintptr_t clippingWeapon = G::memory.Read<uintptr_t>(entity.address + G::offsets.clippingWeapon);
					short viewModelIndex = G::memory.Read<short>(clippingWeapon + G::offsets.AttributeManager + G::offsets.item + G::offsets.ItemDefinitionIndex);

					espText.emplace_back(
						getReadableNameFromID(viewModelIndex),
						G::S.weaponTextColor,
						15.f * head_dependency,
						G::default_font
					);

					uintptr_t c4_entity = G::memory.Read<uintptr_t>(
						G::memory.Read<uintptr_t>(G::client + G::offsets.dwWeaponC4)
					);

					int c4_owner = G::memory.Read<int>(
						c4_entity + G::offsets.m_hOwnerEntity
					);

					if (c4_owner == entity.handle) {
						espText.emplace_back(
							"BOMB",
							ImColor(180, 0, 0),
							15.f * head_dependency,
							G::default_font
						);
					}
				}

				size = { 0, 0 };
				for (auto& eT : espText) {
					size.x += eT.contentSize.x + spacing_sized;
					size.y = max(eT.contentSize.y, size.y);
				}
				size.x -= spacing_sized; // Remove last spacing


				ImVec2 curCursor(
					pos.x - size.x / 2.f,
					pos.y - size.y / 2.f
				);

				drawList->AddRectFilled(
					{ curCursor.x - padding_sized * 2.f	,			curCursor.y - padding_sized },
					{ curCursor.x + size.x + padding_sized * 2.f,	curCursor.y + size.y + padding_sized },
					ImColor(0.f, 0.f, 0.f, 0.5f)
				);

				for (auto& eT : espText) {
					drawList->AddText(
						G::default_font,
						eT.fontSize,
						{ curCursor.x, curCursor.y + size.y - eT.contentSize.y },
						eT.color,
						eT.content.c_str()
					);
					curCursor.x += eT.contentSize.x + spacing_sized;
				}
			}
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