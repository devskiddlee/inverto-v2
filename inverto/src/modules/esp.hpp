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

Vector GetBone(const Entity* pawn, int b) {
	uintptr_t sceneNode = G::memory.Read<uintptr_t>(pawn->address + G::offsets.gameScene);
	uintptr_t boneMatrix = G::memory.Read<uintptr_t>(sceneNode + G::offsets.modelState + G::offsets.boneArray);
	return G::memory.Read<Vector>(boneMatrix + (uint64_t)(b) * 32);
}

bool GetBonesScreenPosEx(std::array<ImVec2, 33>& out, const ViewMatrix& vm) {
	Vector p;
	#define ADD_BONE(b) if (!world_to_screen(G::bonesSample[b], p, vm)) return false; out[b] = p.toVec2();

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

bool GetBonesScreenPos(Entity* pawn, std::array<ImVec2, 33>& out, const ViewMatrix& vm) {
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

bool GetBones(Entity* pawn, std::vector<Vector>& out) {
	Vector p;
	#define ADD_BONE(b) out[b] = GetBone(pawn, b);

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

bool AddPolygon(Entity* pawn, const ViewMatrix& vm, int b1, int b2, const Vector cuboid_size, std::vector<std::vector<ImVec2>>* polygons, bool sort_ccw) {
	Vector start = GetBone(pawn, b1);
	Vector end   = GetBone(pawn, b2);

	std::vector<ImVec2> sP;
	std::vector<Vector> temp_sP;
	for (Vector pt : GetCuboidCorners(start, end, cuboid_size.x * G::S.chams_size, cuboid_size.y * G::S.chams_size)) {
		Vector p;
		if (!world_to_screen(pt, p, vm))
			return false;
		temp_sP.push_back(p);
	}

	std::vector<Vector> convex = GetConvexHull(temp_sP);

	for (auto& pt : convex)
		sP.push_back(pt.toVec2());

	std::vector<ImVec2> out = sort_ccw ? sort_points_ccw(sP) : sP;
	out.push_back(out.front());
	polygons->push_back(out);

	return true;
}

bool AddPolygonEx(const ViewMatrix& vm, int b1, int b2, const Vector cuboid_size, std::vector<std::vector<ImVec2>>* polygons, bool sort_ccw) {
	Vector start = G::bonesSample[b1];
	Vector end = G::bonesSample[b2];

	std::vector<ImVec2> sP;
	std::vector<Vector> temp_sP;
	for (Vector pt : GetCuboidCorners(start, end, cuboid_size.x * G::S.chams_size, cuboid_size.y * G::S.chams_size)) {
		Vector p;
		if (!world_to_screen(pt, p, vm))
			return false;
		temp_sP.push_back(p);
	}

	std::vector<Vector> convex = GetConvexHull(temp_sP);

	for (auto& pt : convex)
		sP.push_back(pt.toVec2());

	std::vector<ImVec2> out = sort_ccw ? sort_points_ccw(sP) : sP;
	out.push_back(out.front());
	polygons->push_back(out);

	return true;
}

ID3D11BlendState* g_NoBlendState = nullptr;
ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;

void CreateNoBlendState(ID3D11Device* device, ID3D11DeviceContext* ctx) {
	D3D11_BLEND_DESC desc = {};
	desc.RenderTarget[0].BlendEnable = FALSE;
	desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	device->CreateBlendState(&desc, &g_NoBlendState);
	g_pd3dDeviceContext = ctx;
}

void SetNoBlendCallback(const ImDrawList* parent_list, const ImDrawCmd* cmd) {
	ID3D11DeviceContext* ctx = (ID3D11DeviceContext*)cmd->UserCallbackData;

	float blendFactor[4] = { 0, 0, 0, 0 };
	ctx->OMSetBlendState(g_NoBlendState, blendFactor, 0xffffffff);
}

void RestoreBlendCallback(const ImDrawList* parent_list, const ImDrawCmd* cmd) {
	ID3D11DeviceContext* ctx = (ID3D11DeviceContext*)cmd->UserCallbackData;

	ctx->OMSetBlendState(nullptr, nullptr, 0xffffffff);
}

void esp_logic(Entity& entity, ImDrawList* drawList, int index, int max) {
	ViewMatrix currentVM = G::memory.Read<ViewMatrix>(G::client + G::offsets.viewmatrix);

	float hS = 10000 / CalcMagnitude(G::localPlayer.origin, entity.origin) * GetFovScaleFactor(90.f);

	ImColor imcolor = G::S.normalColor;

	if (entity.compare(G::render_entities[0]) && ((entity.angleDiff < G::S.maxAngleDiffAimbot * GetFovScaleFactor(90.f)) || G::S.disableAngleDiff)) {
		imcolor = G::S.closestEnemy;
	}

	if (entity.visible) {
		imcolor = ImColor(255, 255, 255);
	}

	Vector head_screen_pos = Vector(-10000, -10000, 0);
	world_to_screen(entity.head, head_screen_pos, currentVM);
	drawList->AddCircle(ImVec2(head_screen_pos.x, head_screen_pos.y), hS / 2, imcolor, 0, 2.f);

	ImColor color = G::S.normalColor;
	if (entity.compare(G::render_entities[0]))
		color = G::S.closestEnemy;

	Vector vec = entity.headScreenPos.copy().operator-(Vector(hS / 2, hS / 2, 0));
	Vector feet = entity.originScreenPos;

	std::array<ImVec2, 33> bones;
	bool bones_valid = GetBonesScreenPos(&entity, bones, currentVM);

	if (G::S.bone_esp && bones_valid) {
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

	if (G::S.boxEsp) {
		double angle = entity.angleEye.y / 180 * M_PI;

		double c = cos(angle);
		double s = sin(angle);

		float size = 15.f;
		draw3dBoxAroundLine(drawList, currentVM, entity.origin, entity.head.copy() + Vector(0, 0, 10), c, s, size, G::S.boxEspWidth, G::S.boxColor);
	}

	if (G::S.chams) {
		std::vector<std::vector<ImVec2>> polygons;
		polygons.reserve(33);

		#define ADD_LIMB(b1, b2, size) if (!AddPolygon(&entity, currentVM, b1, b2, size, &polygons, G::S.chams_filled)) goto skip_chams;

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

		if (G::S.chams_filled) {
			ID3D11DeviceContext* ctx = g_pd3dDeviceContext;

			drawList->AddCallback(SetNoBlendCallback, ctx);

			for (auto& polygon : polygons) {
				drawList->AddConcavePolyFilled(polygon.data(), (int)polygon.size(), G::S.chamsColor);
			}

			drawList->AddCallback(RestoreBlendCallback, ctx);
			drawList->AddCallback(ImDrawCallback_ResetRenderState, nullptr);

			goto skip_chams;
		}

		for (auto& polygon : remove_intersections(polygons)) {
			drawList->AddPolyline(polygon.data(), (int)polygon.size(), G::S.chamsColor, ImDrawFlags_RoundCornersAll, G::S.chamsWidth);
		}
	}
	skip_chams:

	if ((G::S.show_only_nearest_info && entity.compare(G::render_entities[0]) && ((entity.angleDiff < G::S.maxAngleDiffAimbot * GetFovScaleFactor(90.f)) || G::S.disableAngleDiff))
		|| !G::S.show_only_nearest_info) {
		float head_dependency = hS / 20;
		if (G::S.absolute_text_size)
			head_dependency = 1.f;

		Vector pos;
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

					int m_iClip1 = G::memory.Read<int>(clippingWeapon + G::offsets.m_iClip1);
					int m_pReserveAmmo = G::memory.Read<int>(clippingWeapon + G::offsets.m_pReserveAmmo);
					espText.emplace_back(
						str(m_iClip1) + "/" + str(m_pReserveAmmo),
						G::S.weaponTextAmmoColor,
						15.f * head_dependency,
						G::default_font
					);

					bool m_bInReload = G::memory.Read<bool>(clippingWeapon + G::offsets.m_bInReload);
					if (m_bInReload)
						espText.emplace_back(
							"RELOADING",
							G::S.weaponTextReloadingColor,
							15.f * head_dependency,
							G::default_font
						);

					bool m_bPawnHasDefuser = G::memory.Read<bool>(entity.controller + G::offsets.m_bPawnHasDefuser);
					if (m_bPawnHasDefuser) {
						espText.emplace_back(
							"DEFUSE-KIT",
							G::S.weaponTextDefuseKitColor,
							15.f * head_dependency,
							G::default_font
						);
					}

					uintptr_t c4_entity = G::memory.Read<uintptr_t>(
						G::memory.Read<uintptr_t>(G::client + G::offsets.dwWeaponC4)
					);

					int c4_owner = G::memory.Read<int>(
						c4_entity + G::offsets.m_hOwnerEntity
					);

					if (c4_owner == entity.handle) {
						espText.emplace_back(
							"BOMB",
							G::S.weaponTextBombCarrierColor,
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
	static void OnRender(const RenderEvent& event) {
		ImDrawList* drawList = event.drawList;
		std::vector<Entity> entities(G::render_entities);

		if (!G::S.disableAngleDiff)
			drawList->AddCircle(
				G::windowCenter.toVec2(),
				G::S.maxAngleDiffAimbot * GetFovScaleFactor(90.f),
				ImColor(255, 255, 255, 200),
				0,
				1.5f
			);
			
		
		std::sort(entities.begin(), entities.end(), [](const Entity& a, const Entity& b) -> bool {
			return a.magnitude > b.magnitude;
		});

		ViewMatrix currentVM = G::memory.Read<ViewMatrix>(G::client + G::offsets.viewmatrix);

		int ix = 0;
		for (Entity& entity : entities) {
			if (G::S.espOnlyWhenVisible && !entity.visible)
				continue;

			if (G::S.directionTracer) {
				ImColor color = G::S.directionCrosshair;
				if (entity.compare(G::render_entities[0]))
					color = G::S.closestEnemy;
				if (entity.compare(G::render_entities[0]) && (GetAsyncKeyState(G::S.AIMBOT_KEY) < 0))
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

			if (G::S.esp)
				esp_logic(entity, drawList, ix, (int)entities.size());
			ix++;
		}
	}
};