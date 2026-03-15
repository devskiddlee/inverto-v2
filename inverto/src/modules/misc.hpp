#include "module_includes.h"

std::map<std::string, int> shoot_delay{
	{"weapon_ssg08", 1300},
	{"weapon_usp_silencer", 350},
	{"weapon_ak47", 250},
	{"weapon_m4a1", 250},
	{"weapon_m4a1_silencer", 200},
	{"weapon_knife", 0},
	{"weapon_glock", 350},
	{"weapon_elite", 250},
	{"weapon_p250", 350},
	{"weapon_revolver", 100},
	{"weapon_deagle", 900},
	{"weapon_mp7", 200},
	{"weapon_mp9", 200},
	{"weapon_mac10", 200},
	{"weapon_famas", 200},
	{"weapon_galilar", 200},
	{"weapon_awp", 1750},
	{"weapon_scar20", 250},
	{"weapon_g3sg1", 250}
};

void checkWeapon() {
	uintptr_t clippingWeapon = G::memory.Read<uintptr_t>(G::localPlayer.address + G::offsets.clippingWeapon);
	short viewModelIndex = G::memory.Read<short>(clippingWeapon + G::offsets.AttributeManager + G::offsets.item + G::offsets.ItemDefinitionIndex);
	std::string name = getWeaponFromId(viewModelIndex);
	std::transform(name.begin(), name.end(), name.begin(), ::tolower);
	G::weaponName = name;
}

bool free_to_shoot = true;
void shoot_tick(float dt) {
	if (G::shoot && free_to_shoot) {
		free_to_shoot = false;
		Modular::ScheduleDelayedTask(G::S.triggerbotDelay - dt * 1000, "schedule_shoot_fn", [](const TickEvent& event) {
			int revolver = 0;
			if (G::weaponName == "weapon_revolver")
			{
				revolver = 200;
			}

			if (G::S.onlyShootWhenStill && G::localPlayer.absVelocity.length() > 75.f) {
				G::shoot = false;
				free_to_shoot = true;
				return;
			}

			mouse_event(0x0002, 0, 0, 0, GetMessageExtraInfo());

			Modular::ScheduleDelayedTask(10.f + rand() % 11 + revolver, "release_shoot_btn", [](const TickEvent& event) {

				mouse_event(0x0004, 0, 0, 0, GetMessageExtraInfo());

				float weaponShootDelay = 0.f;
				if (!shoot_delay.contains(G::weaponName))
				{
					weaponShootDelay = G::S.default_shoot_delay;
				}
				else {
					weaponShootDelay = (float)shoot_delay[G::weaponName];
				}

				Modular::ScheduleDelayedTask(weaponShootDelay, "free_shoot_fn", [](const TickEvent& event) {
					G::shoot = false;
					free_to_shoot = true;
					});
				});
		});
	}
}

void checkFov()
{
	uintptr_t cameraServices = G::memory.Read<uintptr_t>(G::localPlayer.address + G::offsets.camService);
	unsigned int tempFov = G::memory.Read<unsigned int>(cameraServices + G::offsets.fov);
	if (tempFov == 0) tempFov = 90;
	G::fov = (float)tempFov;
}

const unsigned int INAIR = 65664;
const unsigned int STANDING = 65665;

bool free_to_bhop = true;
void bhop_tick(float dt) {
	if (G::S.bhop && GetAsyncKeyState(G::S.BHOP_KEY) < 0 && free_to_bhop) {
		uintptr_t playerPawnAddress = G::localPlayer.address;
		unsigned int fFlag = G::memory.Read<unsigned int>(playerPawnAddress + G::offsets.jumpFlag);

		if (fFlag == STANDING)
		{
			free_to_bhop = false;

			long to_scroll = 120 + rand() % 51;
			mouse_event(0x0800, 0, 0, -to_scroll, 0);
			mouse_event(0x0800, 0, 0, to_scroll, 0);

			Modular::ScheduleDelayedTask(100, "free_bhop", [](const TickEvent& event) {
				free_to_bhop = true;
			});
		}
	}
}

float time_in_air = 0.f;
void jumpShot_tick(float dt) {
	unsigned int fFlag = G::memory.Read<unsigned int>(G::localPlayer.address + G::offsets.jumpFlag);

	if (fFlag == INAIR)
		time_in_air += dt * 1000;
	else
		time_in_air = 0.f;

	if (time_in_air > 100.f && GetAsyncKeyState(G::S.JUMPSHOT_HOTKEY) < 0 && G::S.jumpShotHack && G::weaponName == "weapon_ssg08")
	{
		Vector velocity = G::localPlayer.absVelocity;

		if (velocity.z < G::jumpShotThreshold && velocity.z > -G::jumpShotThreshold)
			G::shoot = true;
	}
}

float get_flashbang_alpha()
{
	return G::memory.Read<float>(G::localPlayer.address + G::offsets.m_flFlashOverlayAlpha);
}

bool menu_call_cooldown = false;
class Misc {
public:
	static void OnTick(const TickEvent& event) {
		checkWeapon();
		checkFov();
		shoot_tick(event.delta_time);
		bhop_tick(event.delta_time);
		jumpShot_tick(event.delta_time);

		CGlobalVarsBase globalVars = G::memory.Read<CGlobalVarsBase>(G::memory.Read<uintptr_t>(G::client + G::offsets.globalVars));
		std::ostringstream ss;
		for (int i = 0; i < 255; i++) {
			char buffer = { 0 };

			G::memory.ReadRaw(globalVars.m_uCurrentMapName + i, &buffer, sizeof(buffer));

			if (!buffer)
				break;

			ss << buffer;
		}
		std::string mapName = ss.str();
		if (mapName.length() < 9)
			G::mapName = "";
		else
			G::mapName = mapName.substr(5, mapName.length() - 9);
	}

	static void OnRender(const RenderEvent& event) {
		if (G::S.color_overlay) {
			if (G::S.color_overlay_background)
				event.drawList->AddRectFilled(
					{ 0, 0 },
					G::windowSize.toVec2(),
					G::S.color_overlay_color
				);
			else
				ImGui::GetForegroundDrawList()->AddRectFilled(
					{ 0, 0 },
					G::windowSize.toVec2(),
					G::S.color_overlay_color
				);
		}

		if (G::S.anti_flashbang) {
			ImVec4 flashbang_color_vec = G::S.anti_flashbang_color;
			flashbang_color_vec.w = get_flashbang_alpha();
			ImColor flashbang_color = flashbang_color_vec;
			event.drawList->AddRectFilled(ImVec2(0, 0), G::windowSize.toVec2(), flashbang_color);
			event.drawList->AddCircle(G::windowCenter.toVec2(), 6.f, ImColor(1.f, 1.f, 1.f, flashbang_color_vec.w), 0, 2.f);

			if (flashbang_color_vec.w > 0.5f && G::S.anti_flashbang_world_render)
				RenderWorldInRadius(event.drawList, G::S.anti_flashbang_world_render_radius);
		}

		if (G::S.showVelocity) {
			Vector velocity = G::localPlayer.absVelocity;
			velocity.z = 0.f;
			std::string velocity_str = str((int)velocity.length()) + " u/s";
			float text_width = G::default_font->CalcTextSizeA(30.f, 1000, 1000, velocity_str.c_str()).x;
			event.drawList->AddText(G::default_font, 30.f, { G::windowSize.x / 2 - text_width / 2, 700 }, ImColor(255, 255, 0), velocity_str.c_str());
		
			event.drawList->AddCircle(G::windowCenter.toVec2(), velocity.length(), ImColor(255, 255, 0));
		}

		if (!G::S.hide_watermark) {
			const char* watermark = "inverto v3.0";
			ImVec2 size = G::default_font->CalcTextSizeA(15.f, FLT_MAX, FLT_MAX, watermark);
			event.drawList->AddText(
				G::default_font, 15.f,
				{
					G::windowSize.x - size.x - 11.f,
					G::windowSize.y - size.y - 5.f
				},
				ImColor(0, 0, 0),
				watermark
			);
			event.drawList->AddText(
				G::default_font, 15.f,
				{
					G::windowSize.x - size.x - 9.f,
					G::windowSize.y - size.y - 5.f
				},
				ImColor(0, 0, 0),
				watermark
			);

			event.drawList->AddText(
				G::default_font, 15.f,
				{
					G::windowSize.x - size.x - 10.f,
					G::windowSize.y - size.y - 5.f
				},
				ImColor(242, 84, 73),
				watermark
			);
		}
	}
};