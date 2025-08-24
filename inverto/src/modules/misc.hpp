#include "module_includes.h"

std::map<std::string, short> weaponNames{
	{"WEAPON_DEAGLE", 1},
	{"WEAPON_ELITE", 2},
	{"WEAPON_FIVESEVEN", 3},
	{"WEAPON_GLOCK", 4},
	{"WEAPON_AK47", 7},
	{"WEAPON_AUG", 8},
	{"WEAPON_AWP", 9},
	{"WEAPON_FAMAS", 10},
	{"WEAPON_G3SG1", 11},
	{"WEAPON_GALILAR", 13},
	{"WEAPON_M249", 14},
	{"WEAPON_M4A1", 16},
	{"WEAPON_MAC10", 17},
	{"WEAPON_P90", 19},
	{"WEAPON_ZONE_REPULSOR", 20},
	{"WEAPON_MP5SD", 23},
	{"WEAPON_UMP45", 24},
	{"WEAPON_XM1014", 25},
	{"WEAPON_BIZON", 26},
	{"WEAPON_MAG7", 27},
	{"WEAPON_NEGEV", 28},
	{"WEAPON_SAWEDOFF", 29},
	{"WEAPON_TEC9", 30},
	{"WEAPON_TASER", 31},
	{"WEAPON_HKP2000", 32},
	{"WEAPON_MP7", 33},
	{"WEAPON_MP9", 34},
	{"WEAPON_NOVA", 35},
	{"WEAPON_P250", 36},
	{"WEAPON_SHIELD", 37},
	{"WEAPON_SCAR20", 38},
	{"WEAPON_SG556", 39},
	{"WEAPON_SSG08", 40},
	{"WEAPON_KNIFEGG", 41},
	{"WEAPON_KNIFE", 42},
	{"WEAPON_FLASHBANG", 43},
	{"WEAPON_HEGRENADE", 44},
	{"WEAPON_SMOKEGRENADE", 45},
	{"WEAPON_MOLOTOV", 46},
	{"WEAPON_DECOY", 47},
	{"WEAPON_INCGRENADE", 48},
	{"WEAPON_C4", 49},
	{"WEAPON_HEALTHSHOT", 50},
	{"WEAPON_KNIFE_T", 59},
	{"WEAPON_M4A1_SILENCER", 60},
	{"WEAPON_USP_SILENCER", 61},
	{"WEAPON_CZ75A", 63},
	{"WEAPON_REVOLVER", 64},
	{"WEAPON_TAGRENADE", 68},
	{"WEAPON_FISTS", 69},
	{"WEAPON_BREACHCHARGE", 70},
	{"WEAPON_TABLET", 72},
	{"WEAPON_MELEE", 74},
	{"WEAPON_AXE", 75},
	{"WEAPON_HAMMER", 76},
	{"WEAPON_SPANNER", 78},
	{"WEAPON_KNIFE_GHOST", 80},
	{"WEAPON_FIREBOMB", 81},
	{"WEAPON_DIVERSION", 82},
	{"WEAPON_FRAG_GRENADE", 83},
	{"WEAPON_SNOWBALL", 84},
	{"WEAPON_BUMPMINE", 85},
	{"WEAPON_KNIFE_BAYONET", 500},
	{"WEAPON_KNIFE_CSS", 503},
	{"WEAPON_KNIFE_FLIP", 505},
	{"WEAPON_KNIFE_GUT", 506},
	{"WEAPON_KNIFE_KARAMBIT", 507},
	{"WEAPON_KNIFE_M9_BAYONET", 508},
	{"WEAPON_KNIFE_TACTICAL", 509},
	{"WEAPON_KNIFE_FALCHION", 512},
	{"WEAPON_KNIFE_SURVIVAL_BOWIE", 514},
	{"WEAPON_KNIFE_BUTTERFLY", 515},
	{"WEAPON_KNIFE_PUSH", 516},
	{"WEAPON_KNIFE_CORD", 517},
	{"WEAPON_KNIFE_CANIS", 518},
	{"WEAPON_KNIFE_URSUS", 519},
	{"WEAPON_KNIFE_GYPSY_JACKKNIFE", 520},
	{"WEAPON_KNIFE_OUTDOOR", 521},
	{"WEAPON_KNIFE_STILETTO", 522},
	{"WEAPON_KNIFE_WIDOWMAKER", 523},
	{"WEAPON_KNIFE_SKELETON", 525},
	{"WEAPON_KNIFE_KUKRI", 526}
};

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

std::string getWeaponFromId(short weapon_id) {
	for (auto entry : weaponNames) {
		if (entry.second == weapon_id) {
			return entry.first;
		}
	}
	return "";
}

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
		Modular::ScheduleDelayedTask(50.f - dt * 1000, "schedule_shoot_fn", [](TickEvent event) {
			int revolver = 0;
			if (G::weaponName == "weapon_revolver")
			{
				revolver = 200;
			}

			mouse_event(0x0002, 0, 0, 0, GetMessageExtraInfo());

			Modular::ScheduleDelayedTask(10.f + rand() % 11 + revolver, "release_shoot_btn", [](TickEvent event) {

				mouse_event(0x0004, 0, 0, 0, GetMessageExtraInfo());

				float weaponShootDelay = 0.f;
				if (!shoot_delay.contains(G::weaponName))
				{
					weaponShootDelay = G::S.default_shoot_delay;
				}
				else {
					weaponShootDelay = (float)shoot_delay[G::weaponName];
				}

				Modular::ScheduleDelayedTask(weaponShootDelay, "free_shoot_fn", [](TickEvent event) {
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

			Modular::ScheduleDelayedTask(100, "free_bhop", [](TickEvent event) {
				free_to_bhop = true;
			});
		}
	}
}

float time_in_air = 0.f;
void jumpShot_tick(float dt)
{
	uintptr_t playerPawnAddress = G::localPlayer.address;

	unsigned int fFlag = G::memory.Read<unsigned int>(playerPawnAddress + G::offsets.jumpFlag);

	if (fFlag == INAIR)
		time_in_air += dt * 1000;
	else
		time_in_air = 0.f;

	if (time_in_air > 100.f && GetAsyncKeyState(G::S.JUMPSHOT_HOTKEY) < 0 && G::S.jumpShotHack && G::weaponName == "weapon_ssg08")
	{
		Vector velocity = G::localPlayer.absVeloctiy;

		if (velocity.z < G::S.jumpShotThreshold && velocity.z > -G::S.jumpShotThreshold)
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
	static void OnTick(TickEvent event) {
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
		G::mapName = ss.str();
	}

	static void OnRender(RenderEvent event) {
		if (G::S.anti_flashbang) {
			ImVec4 flashbang_color_vec = G::S.anti_flashbang_color;
			flashbang_color_vec.w = get_flashbang_alpha();
			ImColor flashbang_color = flashbang_color_vec;
			event.drawList->AddRectFilled(ImVec2(0, 0), G::windowSize.toVec2(), flashbang_color);
		}
	}
};