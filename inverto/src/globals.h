#pragma once
#include <mem/memify.h>
#include "offsets.h"
#include "utils.hpp"
#include "modular.h"

#undef min

struct QAngle {
    float pitch, yaw, roll;
};

template <typename T>
struct C_UTL_VECTOR
{
    size_t Count = 0;
    T* Data = 0;
};

static constexpr uint64_t Hash(const char* str) {
    uint64_t hash = 5381;
    while (*str)
        hash = ((hash << 5) + hash) + *str++;
    return hash;
}

// might change later on
int fuzzy_score(const std::string& pattern, const std::string& text, size_t* failed = nullptr) {
    if (pattern.empty()) return 0;

    int score = 0;
    int consecutive = 0;
    int bonus = 0;
    size_t p = 0;

    for (size_t t = 0; t < text.size() && p < pattern.size(); t++) {
        if (std::tolower(text[t]) == std::tolower(pattern[p])) {
            int s = 1;

            if (consecutive > 0) {
                s += 5;
            }

            if (t == 0 || text[t - 1] == '_' || text[t - 1] == '-' || text[t - 1] == ' ') {
                s += 8;
            }

            if (std::isupper(text[t]) && std::islower(text[t - 1])) {
                s += 3;
            }

            score += s;
            consecutive++;
            p++;
        }
        else {
            consecutive = 0;
        }
    }

    if (p != pattern.size()) {
        if (failed) (*failed)++;
        return std::numeric_limits<int>::min();
    }

    return score;
}

void sort_by_fuzzy_score(std::vector<std::string>& items, const std::string& target) {
    std::sort(items.begin(), items.end(),
        [&](const std::string& a, const std::string& b) {
            return fuzzy_score(target, a) > fuzzy_score(target, b);
        });
}

struct console_message {
	std::string content;
	ImColor color;
	std::chrono::steady_clock::time_point issued;
};

Vector CalcAngles(Vector from, Vector to)
{
    float yaw;
    float pitch;

    float deltaX = to.x - from.x;
    float deltaY = to.y - from.y;
    yaw = (float)(atan2(deltaY, deltaX) * 180 / M_PI);

    float deltaZ = to.z - from.z;
    double distance = sqrt(pow(deltaX, 2) + pow(deltaY, 2));
    pitch = -(float)(atan2(deltaZ, distance) * 180 / M_PI);

    return Vector(yaw, pitch, 0);
}

struct PlayerInfos {
    uint64_t steam_id = 0;
    std::string note = "";
};

//Settings
class Settings {
public:
	bool teamCheck = true;
	bool rcs = true;
	bool bhop = true;
	int BHOP_KEY = VK_LMENU;

	int aimbotspeed = 3000;
	float default_shoot_delay = 300.f;

	int JUMPSHOT_HOTKEY = VK_XBUTTON2;
	bool jumpShotHack = true;

    private:
	    float __pad = 0.f; // was jumpShotThreshold

    public:

	bool anti_flashbang = true;
	ImColor anti_flashbang_color = ImColor(255, 142, 255);

	bool triggerbot = false;

    int menu_key = VK_INSERT;

	int AIMBOT_KEY = VK_XBUTTON2;
	bool aimbot = true;
	float maxAngleDiffAimbot = 100.f;
	bool disableAngleDiff = false;

	bool directionTracer = true;
    float directionTracerMaxLength = 10.f;
	float width = 1.5f;
	bool esp = true;
	bool name = true;
	bool healthBox = true;
	bool healthText = true;
	float text_padding = 3.f;
	bool bone_esp = true;
	bool show_only_nearest_info = true;
	bool absolute_text_size = true;
    bool radarHack = false;
    bool ignoreVisible = false;
    bool boxEsp = true;
    float boxEspWidth = 1.f;
    bool chams = true;
    float chamsWidth = 2.f;
    bool espOnlyWhenVisible = false;
    bool showVelocity = false;
    bool showVisibilityCollisions = false;
    bool onlyShootWhenStill = true;
    bool autoAimWhenVisible = false;

	ImColor normalColor{ 1.f, 0.f, 0.f };
	ImColor directionCrosshair{ 1.f, 1.f, 0.f };
	ImColor closestEnemy{ 0.f, 0.f, 1.f };
	ImColor aimLockedEnemy{ 0.f, 1.f, 0.f };
	ImColor playerText{ 1.f, 1.f, 1.f };
	ImColor health{ 0.f, 1.f, 0.f };
	ImColor boneColor{ 1.f, 0.f, 1.f };
    ImColor boxColor{ 0.f, 0.f, 1.f, 0.25f };
    ImColor chamsColor{ 1.f, 0.f, 1.f, 0.5f };

    bool vsync = true;
    int frame_cap = 100;
    bool fancy_title = true;
    
    bool spotify_module = false;
    float spotify_module_gradient_speed = 1.f;
    ImColor spotify_module_color_start = ImColor(255, 0, 255);
    ImColor spotify_module_color_end = ImColor(255, 255, 255);
    ImVec2 spotify_module_pos = { 10, 1000 };
    float spotify_module_font_size = 20.f;
    ImColor spotify_module_bg_color = ImColor(0, 0, 0, 128);

    bool fps_module = false;
    bool fps_module_tickspeed = true;
    bool fps_module_vistickspeed = true;
    float fps_module_gradient_speed = 1.f;
    ImColor fps_module_color_start = ImColor(255, 0, 255);
    ImColor fps_module_color_end = ImColor(255, 255, 255);
    ImVec2 fps_module_pos = { 10, 1000 };
    float fps_module_font_size = 20.f;
    ImColor fps_module_bg_color = ImColor(0, 0, 0, 128);

    bool kill_animation = true;
    float kill_animation_duration = 1.f;
    int kill_animation_size = 50;
    ImColor kill_animation_color = ImColor(255, 0, 255);

    bool thorough_vis_check = false;

    int QUICK_TOGGLE_HOTKEY = VK_END;

    bool c4_esp = false;
    ImColor c4_color = ImColor(255, 0, 0, 128);
    float c4_line_width = 1.f;
    bool c4_cross = true;

    bool anti_flashbang_world_render = false;
    float anti_flashbang_world_render_radius = 250.f;

    bool console_disabled = false;

    bool armorText = true;
    ImColor armorTextColor = ImColor(0, 255, 255);

    bool weaponText = true;
    ImColor weaponTextColor = ImColor(180, 180, 180);

    ImColor radarHackColor = ImColor(255, 0, 255, 180);
    float radarHackPointSize = 5.f;
    bool radarHackPointFilled = false;
    float radarOffset = 25.f;
    float radarSize = 250.f;
    float radarZoom = 0.5f;
    bool radarBorder = true;

    bool custom_text_module = false;
    float custom_text_module_gradient_speed = 1.f;
    ImColor custom_text_module_color_start = ImColor(255, 0, 255);
    ImColor custom_text_module_color_end = ImColor(255, 255, 255);
    ImVec2 custom_text_module_pos = { 50, 50 };
    float custom_text_module_font_size = 20.f;
    ImColor custom_text_module_bg_color = ImColor(0, 0, 0, 128);
    char custom_text_content[1024] { 0 };

    float custom_text_module_rounding = 5.f;
    float spotify_module_rounding = 5.f;
    float fps_module_rounding = 5.f;

    float custom_text_module_padding = 5.f;
    float spotify_module_padding = 5.f;
    float fps_module_padding = 5.f;

    ImColor weaponTextAmmoColor = ImColor(180, 180, 180);
    ImColor weaponTextReloadingColor = ImColor(255, 0, 255);
    ImColor weaponTextBombCarrierColor = ImColor(180, 0, 0);
    ImColor weaponTextDefuseKitColor = ImColor(0, 180, 0);

    bool strictMouseAim = false;
    float strictMouseAimThreshold = 0.3f;

    bool chams_filled = true;
    
    bool aimbotSmart = false;

    bool color_overlay = false;
    bool color_overlay_background = true;
    ImColor color_overlay_color = ImColor(0, 0, 0, 50);

    float chams_size = 1.f;

    bool hide_watermark = false;

    bool c4_esp_show_duration = true;
    bool c4_esp_show_damage = true;

    int tick_cap = 100;
    bool tick_capped = false;

    float triggerbotDelay = 50.f;
    float aimAtDelay = 0.f;

    bool ammoCircle = false;
    float ammoCircleSize = 20.f;
    ImColor ammoCircleColor= ImColor(255, 0, 255);
};

class Theme {
public:
    int menu_fontSize = 20;
    float menu_frameRounding = 0;
    float menu_windowRounding = 0;
    ImVec4 Colors[58];
};

struct Triangle {
    Vector p1, p2, p3;

    bool intersect(Vector ray_origin, Vector ray_end)
    {
        Vector edge1, edge2, h, s, q;
        float a, f, u, v;
        edge1 = p2.copy() - p1;
        edge2 = p3.copy() - p1;
        h = (ray_end.copy() - ray_origin).cross(edge2);
        a = edge1.dot(h);

        if (a > -FLT_EPSILON && a < FLT_EPSILON)
            return false;

        f = 1.0f / a;
        s = ray_origin.copy() - p1;
        u = f * s.dot(h);

        if (u < 0.0 || u > 1.0)
            return false;

        q = s.cross(edge1);
        v = f * (ray_end.copy() - ray_origin).dot(q);

        if (v < 0.0 || u + v > 1.0)
            return false;

        float t = f * edge2.dot(q);
        return t > FLT_EPSILON && t < 1.0;
    }

    std::vector<ImVec2> to_screen(ViewMatrix currentVM, bool* success = nullptr) {
        Vector sp1(-10000, -10000, 0);
        Vector sp2(-10000, -10000, 0);
        Vector sp3(-10000, -10000, 0);
        bool b1 = world_to_screen(p1, sp1, currentVM);
        bool b2 = world_to_screen(p2, sp2, currentVM);
        bool b3 = world_to_screen(p3, sp3, currentVM);
        if (success) *success = b1 && b2 && b3;
        return { sp1.toVec2(), sp2.toVec2(), sp3.toVec2() };
    }
};

namespace G {
	memify memory("cs2.exe");
	uintptr_t client;

	Offsets offsets;
	Entity localPlayer;
	uintptr_t localPlayerController;

	Vector windowLocation = Vector(0, 0, 0);
	Vector windowSize = Vector(1920, 1080, 0);
	Vector windowCenter = windowSize.copy() / Vector(2, 2, 2);

    float FOV_conversion_factor = 1.f;
	std::list<console_message> console;

    std::unordered_map<std::string, bool> visibleMap;
    std::unordered_map<std::string, bool> lowerVisibleMap;
    std::unordered_map<std::string, float> time_alive;
    std::unordered_map<std::string, float> timeVisibleMap;

	std::vector<Entity> entities;
	std::vector<Entity> render_entities;

	std::map<int, std::list<int>> bone_connections{
	{ 0 , {4}			},
	{ 1 , {11, 14}		},
	{ 2 , {5, 8, 3}		},
	{ 3 , {1}			},
	{ 4 , {2}			},
	{ 5 , {6}			},
	{ 6 , {7}			},
	{ 8 , {9}			},
	{ 9 , {10}			},
	{ 11, {12}			},
	{ 12, {13}			},
	{ 14, {15}			},
	{ 15, {16}			}
	};

    std::vector<Vector> bonesSample {
        { -6.36206, -0.0710449, 37.2302 },
        { 1181, 754, -120.181 },
        { -5.72314, 0.139709, 42.0068 },
        { -5.20105, 0.557251, 46.5822 },
        { -3.4884, 0.702271, 53.1574 },
        { -0.358887, 1.13763, 58.5164 },
        { 2.6554, 2.57465, 62.9042 },
        { 1181, 754, -120.181 },
        { -4.13977, 8.21942, 55.7809 },
        { 1.55408, 13.268, 47.0161 },
        { 12.3718, 13.3132, 50.8795 },
        { 1181, 754, -120.181 },
        { 1181, 754, -120.181 },
        { 3.16077, -5.29462, 54.5313 },
        { 11.7013, -0.366455, 48.3783 },
        { 13.8082, 10.6881, 50.6822 },
        { 1181, 754, -120.181 },
        { 1181, 754, -120.181 },
        { 1181, 754, -120.181 },
        { 1181, 754, -120.181 },
        { 1181, 754, -120.181 },
        { 1181, 754, -120.181 },
        { -5.26392, 3.93561, 33.5404 },
        { 4.75952, 12.7694, 21.6235 },
        { 1181, 754, -120.181 },
        { -5.23462, -3.37817, 32.3953 },
        { 1.88123, -9.15826, 17.0177 },
        { 1181, 754, -120.181 },
        { 1181, 754, -120.181 },
        { 1181, 754, -120.181 },
        { 2.23608, 15.1546, 5.04601 },
        { 1181, 754, -120.181 }
    };

	ImFont* default_font;
    ImFont* menu_font;

	float fov = 90.f;
	bool shoot = false;
	std::string weaponName;
	bool render_ui = false;

    std::string current_config;
    std::string current_theme;

    std::string mapName;
    std::vector<Triangle> triangles_loaded;

    bool use_AVX_512 = false;
    float avg_frame_time = 0.f;
    float avg_vis_time = 0.f;

    bool quick_toggle_enabled = false;

    HWND window = 0;

    float jumpShotThreshold = 50.f;

    float radarConstant = 7.5f;
    bool renderRadarBox = false;

	Settings S{};
    Theme T{};
}

static uintptr_t getWeaponVData(const uintptr_t& weaponEntity) {
    return G::memory.Read<uintptr_t>(weaponEntity + G::offsets.m_nSubclassID + 0x8);
}

enum HitGroup : int {
    HitGroup_Head,
    HitGroup_Chest_Arm_Neck,
    HitGroup_Stomach,
    HitGroup_Leg
};

int GetDamageOfCurrentWeapon(const HitGroup& hitGroup, Entity* e) {
    uintptr_t clippingWeapon = G::memory.Read<uintptr_t>(G::localPlayer.address + G::offsets.clippingWeapon);
    uintptr_t weaponData = getWeaponVData(clippingWeapon);
    int damage = G::memory.Read<int>(weaponData + G::offsets.m_nDamage);

    float m_flHeadshotMultiplier = G::memory.Read<float>(weaponData + G::offsets.m_flHeadshotMultiplier);

    switch (hitGroup) {
    case HitGroup_Head:
        damage *= m_flHeadshotMultiplier;
        break;
    case HitGroup_Chest_Arm_Neck:
        damage *= 1.f;
        break;
    case HitGroup_Stomach:
        damage *= 1.25f;
        break;
    case HitGroup_Leg:
        damage *= 0.75f;
        break;
    default:
        break;
    }

    float m_flRangeModifier = G::memory.Read<float>(weaponData + G::offsets.m_flRangeModifier);
    damage *= powf(m_flRangeModifier, e->dist / 500.f);

    uint32_t armor = G::memory.Read<uint32_t>(e->address + G::offsets.m_ArmorValue);

    if (!armor)
        return damage;

    float m_flArmorRatio = G::memory.Read<float>(weaponData + 0x748);

    float armor_bonus = 0.5f;
    float armor_ratio = m_flArmorRatio * 0.5f;

    const float armor_value = (float)(armor);
    const float damage_to_armor = (damage - damage * armor_ratio) * armor_bonus;

    if (damage_to_armor > armor_value) {
        damage = damage - armor_value / armor_bonus;
    }
    else {
        damage *= armor_ratio;
    }

    return damage;
}

float GetFovScaleFactor(float default_fov) {
    float refRad = default_fov * (M_PI / 180.0f);
    float curRad = G::fov * (M_PI / 180.0f);

    return 1.f / (std::tan(curRad * 0.5f) / std::tan(refRad * 0.5f));
}

void PushMenuStyle() {
    for (int i = 0; i < 58; i++)
        ImGui::PushStyleColor(i, G::T.Colors[i]);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, G::T.menu_windowRounding);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, G::T.menu_frameRounding);
    ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, G::T.menu_frameRounding);
}

void PopMenuStyle() {
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(58);
}

void RenderWorldInRadius(ImDrawList* drawList, float min, float max) {
    ViewMatrix currentVM = G::memory.Read<ViewMatrix>(G::client + G::offsets.viewmatrix);
    for (Triangle& T : G::triangles_loaded) {
        if (CalcMagnitude(T.p1, G::localPlayer.head) > max) continue;
        if (CalcMagnitude(T.p2, G::localPlayer.head) > max) continue;
        if (CalcMagnitude(T.p3, G::localPlayer.head) > max) continue;

        if (CalcMagnitude(T.p1, G::localPlayer.head) < min) continue;
        if (CalcMagnitude(T.p2, G::localPlayer.head) < min) continue;
        if (CalcMagnitude(T.p3, G::localPlayer.head) < min) continue;

        float x = (T.p1.x + T.p2.x + T.p3.x) / 3.f;
        float y = (T.p1.y + T.p2.y + T.p3.y) / 3.f;
        float z = (T.p1.z + T.p2.z + T.p3.z) / 3.f;
        Vector pos(x, y, z);

        float a = (CalcMagnitude(pos, G::localPlayer.head) - min) / max;
        bool success = false;
        std::vector<ImVec2> points = T.to_screen(currentVM, &success);
        if (!success) continue;

        drawList->AddTriangleFilled(points[0], points[1], points[2], ImColor(0.f, 0.f, 0.f, a));
    }
}

void RenderWorldInRadius(ImDrawList* drawList, float max) {
    ViewMatrix currentVM = G::memory.Read<ViewMatrix>(G::client + G::offsets.viewmatrix);
    for (Triangle& T : G::triangles_loaded) {
        if (CalcMagnitude(T.p1, G::localPlayer.head) > max) continue;
        if (CalcMagnitude(T.p2, G::localPlayer.head) > max) continue;
        if (CalcMagnitude(T.p3, G::localPlayer.head) > max) continue;

        float x = (T.p1.x + T.p2.x + T.p3.x) / 3.f;
        float y = (T.p1.y + T.p2.y + T.p3.y) / 3.f;
        float z = (T.p1.z + T.p2.z + T.p3.z) / 3.f;
        Vector pos(x, y, z);

        float a = CalcMagnitude(pos, G::localPlayer.head) / max;
        bool success = false;
        std::vector<ImVec2> points = T.to_screen(currentVM, &success);
        if (!success) continue;

        drawList->AddTriangleFilled(points[0], points[1], points[2], ImColor(0.f, 0.f, 0.f, std::fabs(a - 1.f)));
    }
}

int console_show = 0;

void send_console_message(std::string content, ImColor color) {
	console_message cmsg;
	cmsg.content = content;
	cmsg.color = color;
	cmsg.issued = std::chrono::high_resolution_clock::now();
	G::console.push_front(cmsg);
	if (G::console.size() > 10) {
		G::console.pop_back();
	}
	console_show = 10000;
}

void info(std::string msg) {
	send_console_message("INFO: " + msg, ImColor(255, 255, 255));
}

void error(std::string msg) {
	send_console_message("ERROR: " + msg, ImColor(255, 0, 0));
}

void warning(std::string msg) {
	send_console_message("WARNING: " + msg, ImColor(255, 255, 0));
}

void highlight(std::string msg) {
	send_console_message("~ " + msg, ImColor(100, 100, 255));
}

void confirm(std::string msg) {
    send_console_message("CONFIRM: " + msg, ImColor(0, 255, 0));
}

bool IsPixelInsideScreen(Vector pixel)
{
	return pixel.x > 0 && pixel.x < G::windowSize.x && pixel.y > 0 && pixel.y < G::windowSize.y;
}

bool IsPixelInsideScreen(ImVec2 pixel)
{
    return pixel.x > 0 && pixel.x < G::windowSize.x && pixel.y > 0 && pixel.y < G::windowSize.y;
}

static const char* KeyNames[] = {
    "OFF",
    "VK_LBUTTON",
    "VK_RBUTTON",
    "VK_CANCEL",
    "VK_MBUTTON",
    "VK_XBUTTON1",
    "VK_XBUTTON2",
    "Unknown",
    "VK_BACK",
    "VK_TAB",
    "Unknown",
    "Unknown",
    "VK_CLEAR",
    "VK_RETURN",
    "Unknown",
    "Unknown",
    "VK_SHIFT",
    "VK_CONTROL",
    "VK_MENU",
    "VK_PAUSE",
    "VK_CAPITAL",
    "VK_KANA",
    "Unknown",
    "VK_JUNJA",
    "VK_FINAL",
    "VK_KANJI",
    "Unknown",
    "VK_ESCAPE",
    "VK_CONVERT",
    "VK_NONCONVERT",
    "VK_ACCEPT",
    "VK_MODECHANGE",
    "VK_SPACE",
    "VK_PRIOR",
    "VK_NEXT",
    "VK_END",
    "VK_HOME",
    "VK_LEFT",
    "VK_UP",
    "VK_RIGHT",
    "VK_DOWN",
    "VK_SELECT",
    "VK_PRINT",
    "VK_EXECUTE",
    "VK_SNAPSHOT",
    "VK_INSERT",
    "VK_DELETE",
    "VK_HELP",
    "0",
    "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    "9",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "A",
    "B",
    "C",
    "D",
    "E",
    "F",
    "G",
    "H",
    "I",
    "J",
    "K",
    "L",
    "M",
    "N",
    "O",
    "P",
    "Q",
    "R",
    "S",
    "T",
    "U",
    "V",
    "W",
    "X",
    "Y",
    "Z",
    "VK_LWIN",
    "VK_RWIN",
    "VK_APPS",
    "Unknown",
    "VK_SLEEP",
    "VK_NUMPAD0",
    "VK_NUMPAD1",
    "VK_NUMPAD2",
    "VK_NUMPAD3",
    "VK_NUMPAD4",
    "VK_NUMPAD5",
    "VK_NUMPAD6",
    "VK_NUMPAD7",
    "VK_NUMPAD8",
    "VK_NUMPAD9",
    "VK_MULTIPLY",
    "VK_ADD",
    "VK_SEPARATOR",
    "VK_SUBTRACT",
    "VK_DECIMAL",
    "VK_DIVIDE",
    "VK_F1",
    "VK_F2",
    "VK_F3",
    "VK_F4",
    "VK_F5",
    "VK_F6",
    "VK_F7",
    "VK_F8",
    "VK_F9",
    "VK_F10",
    "VK_F11",
    "VK_F12",
    "VK_F13",
    "VK_F14",
    "VK_F15",
    "VK_F16",
    "VK_F17",
    "VK_F18",
    "VK_F19",
    "VK_F20",
    "VK_F21",
    "VK_F22",
    "VK_F23",
    "VK_F24",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "VK_NUMLOCK",
    "VK_SCROLL",
    "VK_OEM_NEC_EQUAL",
    "VK_OEM_FJ_MASSHOU",
    "VK_OEM_FJ_TOUROKU",
    "VK_OEM_FJ_LOYA",
    "VK_OEM_FJ_ROYA",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "VK_LSHIFT",
    "VK_RSHIFT",
    "VK_LCONTROL",
    "VK_RCONTROL",
    "VK_LMENU",
    "VK_RMENU"
};
static const int KeyCodes[] = {
    0x0,  //Undefined
    0x01,
    0x02,
    0x03,
    0x04,
    0x05,
    0x06,
    0x07, //Undefined
    0x08,
    0x09,
    0x0A, //Reserved
    0x0B, //Reserved
    0x0C,
    0x0D,
    0x0E, //Undefined
    0x0F, //Undefined
    0x10,
    0x11,
    0x12,
    0x13,
    0x14,
    0x15,
    0x16, //IME On
    0x17,
    0x18,
    0x19,
    0x1A, //IME Off
    0x1B,
    0x1C,
    0x1D,
    0x1E,
    0x1F,
    0x20,
    0x21,
    0x22,
    0x23,
    0x24,
    0x25,
    0x26,
    0x27,
    0x28,
    0x29,
    0x2A,
    0x2B,
    0x2C,
    0x2D,
    0x2E,
    0x2F,
    0x30,
    0x31,
    0x32,
    0x33,
    0x34,
    0x35,
    0x36,
    0x37,
    0x38,
    0x39,
    0x3A, //Undefined
    0x3B, //Undefined
    0x3C, //Undefined
    0x3D, //Undefined
    0x3E, //Undefined
    0x3F, //Undefined
    0x40, //Undefined
    0x41,
    0x42,
    0x43,
    0x44,
    0x45,
    0x46,
    0x47,
    0x48,
    0x49,
    0x4A,
    0x4B,
    0x4C,
    0x4B,
    0x4E,
    0x4F,
    0x50,
    0x51,
    0x52,
    0x53,
    0x54,
    0x55,
    0x56,
    0x57,
    0x58,
    0x59,
    0x5A,
    0x5B,
    0x5C,
    0x5D,
    0x5E, //Rservered
    0x5F,
    0x60, //Numpad1
    0x61, //Numpad2
    0x62, //Numpad3
    0x63, //Numpad4
    0x64, //Numpad5
    0x65, //Numpad6
    0x66, //Numpad7
    0x67, //Numpad8
    0x68, //Numpad8
    0x69, //Numpad9
    0x6A,
    0x6B,
    0x6C,
    0x6D,
    0x6E,
    0x6F,
    0x70, //F1
    0x71, //F2
    0x72, //F3
    0x73, //F4
    0x74, //F5
    0x75, //F6
    0x76, //F7
    0x77, //F8
    0x78, //F9
    0x79, //F10
    0x7A, //F11
    0x7B, //F12
    0x7C, //F13
    0x7D, //F14
    0x7E, //F15
    0x7F, //F16
    0x80, //F17
    0x81, //F18
    0x82, //F19
    0x83, //F20
    0x84, //F21
    0x85, //F22
    0x86, //F23
    0x87, //F24
    0x88, //Unkown
    0x89, //Unkown
    0x8A, //Unkown
    0x8B, //Unkown
    0x8C, //Unkown
    0x8D, //Unkown
    0x8E, //Unkown
    0x8F, //Unkown
    0x90,
    0x91,
    0x92, //OEM Specific
    0x93, //OEM Specific
    0x94, //OEM Specific
    0x95, //OEM Specific
    0x96, //OEM Specific
    0x97, //Unkown
    0x98, //Unkown
    0x99, //Unkown
    0x9A, //Unkown
    0x9B, //Unkown
    0x9C, //Unkown
    0x9D, //Unkown
    0x9E, //Unkown 
    0x9F, //Unkown
    0xA0,
    0xA1,
    0xA2,
    0xA3,
    0xA4,
    0xA5
};

ImGuiKey VkToImGuiKey(int vk) {
    switch (vk) {
        // Letters
    case 'A': return ImGuiKey_A;
    case 'B': return ImGuiKey_B;
    case 'C': return ImGuiKey_C;
    case 'D': return ImGuiKey_D;
    case 'E': return ImGuiKey_E;
    case 'F': return ImGuiKey_F;
    case 'G': return ImGuiKey_G;
    case 'H': return ImGuiKey_H;
    case 'I': return ImGuiKey_I;
    case 'J': return ImGuiKey_J;
    case 'K': return ImGuiKey_K;
    case 'L': return ImGuiKey_L;
    case 'M': return ImGuiKey_M;
    case 'N': return ImGuiKey_N;
    case 'O': return ImGuiKey_O;
    case 'P': return ImGuiKey_P;
    case 'Q': return ImGuiKey_Q;
    case 'R': return ImGuiKey_R;
    case 'S': return ImGuiKey_S;
    case 'T': return ImGuiKey_T;
    case 'U': return ImGuiKey_U;
    case 'V': return ImGuiKey_V;
    case 'W': return ImGuiKey_W;
    case 'X': return ImGuiKey_X;
    case 'Y': return ImGuiKey_Y;
    case 'Z': return ImGuiKey_Z;

        // Numbers (top row)
    case '0': return ImGuiKey_0;
    case '1': return ImGuiKey_1;
    case '2': return ImGuiKey_2;
    case '3': return ImGuiKey_3;
    case '4': return ImGuiKey_4;
    case '5': return ImGuiKey_5;
    case '6': return ImGuiKey_6;
    case '7': return ImGuiKey_7;
    case '8': return ImGuiKey_8;
    case '9': return ImGuiKey_9;

        // Function keys
    case VK_F1:  return ImGuiKey_F1;
    case VK_F2:  return ImGuiKey_F2;
    case VK_F3:  return ImGuiKey_F3;
    case VK_F4:  return ImGuiKey_F4;
    case VK_F5:  return ImGuiKey_F5;
    case VK_F6:  return ImGuiKey_F6;
    case VK_F7:  return ImGuiKey_F7;
    case VK_F8:  return ImGuiKey_F8;
    case VK_F9:  return ImGuiKey_F9;
    case VK_F10: return ImGuiKey_F10;
    case VK_F11: return ImGuiKey_F11;
    case VK_F12: return ImGuiKey_F12;

        // Modifiers
    case VK_TAB:      return ImGuiKey_Tab;
    case VK_LEFT:     return ImGuiKey_LeftArrow;
    case VK_RIGHT:    return ImGuiKey_RightArrow;
    case VK_UP:       return ImGuiKey_UpArrow;
    case VK_DOWN:     return ImGuiKey_DownArrow;
    case VK_PRIOR:    return ImGuiKey_PageUp;
    case VK_NEXT:     return ImGuiKey_PageDown;
    case VK_HOME:     return ImGuiKey_Home;
    case VK_END:      return ImGuiKey_End;
    case VK_INSERT:   return ImGuiKey_Insert;
    case VK_DELETE:   return ImGuiKey_Delete;
    case VK_BACK:     return ImGuiKey_Backspace;
    case VK_SPACE:    return ImGuiKey_Space;
    case VK_RETURN:   return ImGuiKey_Enter;
    case VK_ESCAPE:   return ImGuiKey_Escape;

    case VK_LCONTROL: return ImGuiKey_LeftCtrl;
    case VK_RCONTROL: return ImGuiKey_RightCtrl;
    case VK_LSHIFT:   return ImGuiKey_LeftShift;
    case VK_RSHIFT:   return ImGuiKey_RightShift;
    case VK_LMENU:    return ImGuiKey_LeftAlt;   // Alt
    case VK_RMENU:    return ImGuiKey_RightAlt;
    case VK_LWIN:     return ImGuiKey_LeftSuper; // Windows key
    case VK_RWIN:     return ImGuiKey_RightSuper;

        // Numpad
    case VK_NUMPAD0: return ImGuiKey_Keypad0;
    case VK_NUMPAD1: return ImGuiKey_Keypad1;
    case VK_NUMPAD2: return ImGuiKey_Keypad2;
    case VK_NUMPAD3: return ImGuiKey_Keypad3;
    case VK_NUMPAD4: return ImGuiKey_Keypad4;
    case VK_NUMPAD5: return ImGuiKey_Keypad5;
    case VK_NUMPAD6: return ImGuiKey_Keypad6;
    case VK_NUMPAD7: return ImGuiKey_Keypad7;
    case VK_NUMPAD8: return ImGuiKey_Keypad8;
    case VK_NUMPAD9: return ImGuiKey_Keypad9;

    case VK_MULTIPLY: return ImGuiKey_KeypadMultiply;
    case VK_ADD:      return ImGuiKey_KeypadAdd;
    case VK_SUBTRACT: return ImGuiKey_KeypadSubtract;
    case VK_DIVIDE:   return ImGuiKey_KeypadDivide;
    case VK_DECIMAL:  return ImGuiKey_KeypadDecimal;

    default: return ImGuiKey_None;
    }
}

DWORD GetProcessIdByName(const std::wstring& processName) {
    DWORD pid = 0;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;

    PROCESSENTRY32W entry = { 0 };
    entry.dwSize = sizeof(entry);

    if (Process32FirstW(snap, &entry)) {
        do {
            if (!_wcsicmp(entry.szExeFile, processName.c_str())) {
                pid = entry.th32ProcessID;
                break;
            }
        } while (Process32NextW(snap, &entry));
    }

    CloseHandle(snap);
    return pid;
}

HWND GetMainWindowFromPID(DWORD pid) {
    HWND hWnd = NULL;

    struct EnumData {
        DWORD pid;
        HWND hwnd;
    } data = { pid, NULL };

    auto EnumProc = [](HWND hwnd, LPARAM lParam) -> BOOL {
        EnumData* d = reinterpret_cast<EnumData*>(lParam);
        DWORD windowPID = 0;
        GetWindowThreadProcessId(hwnd, &windowPID);

        // Only consider visible top-level windows
        if (windowPID == d->pid && GetWindow(hwnd, GW_OWNER) == NULL && IsWindowVisible(hwnd)) {
            d->hwnd = hwnd;
            return FALSE; // stop enumeration
        }
        return TRUE; // continue
        };

    EnumWindows(EnumProc, reinterpret_cast<LPARAM>(&data));
    return data.hwnd;
}

class CGlobalVarsBase
{
public:
    float m_flRealTime; //0x0000
    int32_t m_iFrameCount; //0x0004
    float m_flAbsoluteFrameTime; //0x0008
    float m_flAbsoluteFrameStartTimeStdDev; //0x000C
    int32_t m_nMaxClients; //0x0010
    char pad_0014[28]; //0x0014
    float m_flCurrentTime; //0x0034
    float m_flIntervalPerTick; //0x0030
    float m_flCurrentTime2; //0x0038
    char pad_003C[20]; //0x003C
    int32_t m_nTickCount; //0x0050
    char pad_0054[292]; //0x0054
    uint64_t m_uCurrentMap; //0x0178
    uint64_t m_uCurrentMapName; //0x0180
}; //Size: 0x0188


struct CSMatchStats {
    char pad1[0x30];
    int32_t	kills;
    int32_t	deaths;
    int32_t	assists;
    int32_t	damage;
    int32_t	equipmentValue;
    int32_t	moneySaved;
    int32_t	killReward;
    int32_t	liveTime;
    int32_t	headShotKills;
    int32_t	objective;
    int32_t	cashEarned;
    int32_t	utilityDamage;
    int32_t	enemiesFlashed;
    char pad2[0x4];
    int32_t enemy5Ks;
    int32_t enemy4Ks;
    int32_t enemy3Ks;
};

std::vector<Vector> GetConvexHull(std::vector<Vector>& points) {
    if (points.size() <= 1) return points;

    std::sort(points.begin(), points.end(), [](const Vector& a, const Vector& b) {
        return a.x < b.x || (a.x == b.x && a.y < b.y);
        });

    std::vector<Vector> hull;

    for (auto& p : points) {
        while (hull.size() >= 2 &&
            orientation(hull[hull.size() - 2], hull.back(), p) <= 0) {
            hull.pop_back();
        }
        hull.push_back(p);
    }

    size_t lower_size = hull.size();
    for (int i = (int)points.size() - 2; i >= 0; i--) {
        auto& p = points[i];
        while (hull.size() > lower_size &&
            orientation(hull[hull.size() - 2], hull.back(), p) <= 0) {
            hull.pop_back();
        }
        hull.push_back(p);
    }

    hull.pop_back();
    return hull;
}

std::vector<Vector> GetCuboidCorners(Vector start, Vector end, float width, float height)
{
    Vector direction = end.copy() - start.copy();
    float length = direction.length();
    Vector forward = direction.normalized();

    Vector worldUp = Vector(0, 1, 0);
    if (forward.dot(worldUp) > 0.999f)
        worldUp = Vector(1, 0, 0);

    Vector right = worldUp.cross(forward).normalized();
    Vector up = forward.cross(right).normalized();

    float w = width / 2.f;
    float h = height / 2.f;

    Vector p0 = start.copy() + (right.copy() * -w) + (up.copy() * -h);
    Vector p1 = start.copy() + (right.copy() *  w) + (up.copy() * -h);
    Vector p2 = start.copy() + (right.copy() *  w) + (up.copy() *  h);
    Vector p3 = start.copy() + (right.copy() * -w) + (up.copy() *  h);

    Vector offset = forward.copy() * length;
    Vector p4 = p0.copy() + offset.copy();
    Vector p5 = p1.copy() + offset.copy();
    Vector p6 = p2.copy() + offset.copy();
    Vector p7 = p3.copy() + offset.copy();

    return { p0, p1, p2, p3, p4, p5, p6, p7 };
}

void draw3dCross(ImDrawList* drawList, ViewMatrix currentViewMatrix, Vector pos, float size, float line_width, ImColor color) {
    Vector v_x1 = pos.copy() + Vector(size, 0, 0);
    Vector v_x2 = pos.copy() - Vector(size, 0, 0);

    Vector v_y1 = pos.copy() + Vector(0, size, 0);
    Vector v_y2 = pos.copy() - Vector(0, size, 0);

    Vector v_z1 = pos.copy() + Vector(0, 0, size);
    Vector v_z2 = pos.copy() - Vector(0, 0, size);

    Vector p_x1;
    world_to_screen(v_x1, p_x1, currentViewMatrix);
    Vector p_x2;
    world_to_screen(v_x2, p_x2, currentViewMatrix);

    Vector p_y1;
    world_to_screen(v_y1, p_y1, currentViewMatrix);
    Vector p_y2;
    world_to_screen(v_y2, p_y2, currentViewMatrix);

    Vector p_z1;
    world_to_screen(v_z1, p_z1, currentViewMatrix);
    Vector p_z2;
    world_to_screen(v_z2, p_z2, currentViewMatrix);

    std::vector<Vector> points = { p_x1, p_x2, p_y1, p_y2, p_z1, p_z2 };

    bool valid = true;

    for (Vector pt : points)
        if (!IsPixelInsideScreen(pt))
            valid = false;

    if (!valid)
        return;

    drawList->AddLine(p_x1.toVec2(), p_x2.toVec2(), color, line_width);
    drawList->AddLine(p_y1.toVec2(), p_y2.toVec2(), color, line_width);
    drawList->AddLine(p_z1.toVec2(), p_z2.toVec2(), color, line_width);
}

void draw3dBoxAroundLine(ImDrawList* drawList, ViewMatrix currentViewMatrix, Vector start, Vector end, double cos, double sin, float size, float line_width, ImColor color, bool outline = true, bool straight = true)
{
    float x_offset = end.x - start.x;
    float y_offset = end.y - start.y;

    Vector v1 = start.copy() + Vector((float)(-size * cos - -size * sin), (float)(-size * sin + -size * cos), 0);
    Vector v2 = start.copy() + Vector((float)(size * cos - -size * sin), (float)(size * sin + -size * cos), 0);
    Vector v3 = start.copy() + Vector((float)(-size * cos - size * sin), (float)(-size * sin + size * cos), 0);
    Vector v4 = start.copy() + Vector((float)(size * cos - size * sin), (float)(size * sin + size * cos), 0);

    Vector v5 = Vector(v1.x, v1.y, end.z);
    Vector v6 = Vector(v2.x, v2.y, end.z);
    Vector v7 = Vector(v3.x, v3.y, end.z);
    Vector v8 = Vector(v4.x, v4.y, end.z);

    if (!straight)
    {
        v5 + Vector(x_offset, y_offset, 0);
        v6 + Vector(x_offset, y_offset, 0);
        v7 + Vector(x_offset, y_offset, 0);
        v8 + Vector(x_offset, y_offset, 0);
    }

    Vector p1;
    world_to_screen(v1, p1, currentViewMatrix);
    Vector p2;
    world_to_screen(v2, p2, currentViewMatrix);
    Vector p3;
    world_to_screen(v3, p3, currentViewMatrix);
    Vector p4;
    world_to_screen(v4, p4, currentViewMatrix);
    Vector p5;
    world_to_screen(v5, p5, currentViewMatrix);
    Vector p6;
    world_to_screen(v6, p6, currentViewMatrix);
    Vector p7;
    world_to_screen(v7, p7, currentViewMatrix);
    Vector p8;
    world_to_screen(v8, p8, currentViewMatrix);

    std::vector<Vector> vectors = { v1, v2, v3, v4, v5, v6, v7, v8 };
    std::vector<Vector> points  = { p1, p2, p3, p4, p5, p6, p7, p8 };

    bool valid = true;

    for (Vector pt : points)
        if (!IsPixelInsideScreen(pt))
            valid = false;

    if (!valid)
        return;

    std::vector<Vector> o_points = GetConvexHull(points);
    std::vector<ImVec2> o_points_vec2;
    for (Vector pt : o_points)
        o_points_vec2.push_back(pt.toVec2());

    ImVec2* pts = &o_points_vec2[0];

    drawList->AddConvexPolyFilled(pts, (int)o_points_vec2.size(), color);

    if (outline)
    {
        auto color = ImColor(255, 255, 255);

        Vector last_point;
        for (int i = 0; i < o_points.size(); i++)
        {
            Vector pt = o_points[i];
            if (i == 0) {
                last_point = pt;
                continue;
            }

            drawList->AddLine(last_point.toVec2(), pt.toVec2(), color, line_width);
            last_point = pt;
        }
        drawList->AddLine(last_point.toVec2(), o_points[0].toVec2(), color, line_width);
    }
}

using Point = bg::model::d2::point_xy<float>;
using MultiPolygon = bg::model::multi_polygon<bg::model::polygon<Point>>;

static bg::model::polygon<Point> make_polygon(const std::vector<ImVec2>& pts) {
    bg::model::polygon<Point> poly;
    for (auto const& p : pts) poly.outer().emplace_back(p.x, p.y);
    // ensure closed
    if (!poly.outer().empty() && poly.outer().begin() != poly.outer().end())
        poly.outer().push_back(poly.outer().front());
    bg::correct(poly);
    return poly;
}

// this doesn't fucking work (no idea why)
std::vector<std::vector<ImVec2>> remove_intersections(const std::vector<std::vector<ImVec2>>& input)
{
    MultiPolygon acc;

    bool first = true;
    for (auto const& polygon : input) {
        bg::model::polygon<Point> p = make_polygon(polygon);

        if (first) {
            acc.push_back(p);
            first = false;
        }
        else {
            MultiPolygon next;
            bg::union_(acc, p, next);
            acc.swap(next);
        }
    }

    std::vector<std::vector<ImVec2>> out;
    for (auto const& poly : acc) {
        std::vector<ImVec2> ring;
        for (auto const& pt : poly.outer())
            ring.emplace_back(pt.x(), pt.y());
        out.push_back(std::move(ring));
    }
    return out;
}

bool compare_polygon(std::vector<ImVec2> p1, std::vector<ImVec2> p2) {
    if (p1.size() != p2.size())
        return false;

    for (int i = 0; i < p1.size(); i++) {
        ImVec2 v1 = p1[i];
        ImVec2 v2 = p2[i];
        if (v1.x != v2.x)
            return false;
        if (v1.y != v2.y)
            return false;
    }

    return true;
}

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

std::string getReadableNameFromID(short id) {
    switch (id) {
    case 1: return "Desert Eagle";
    case 2: return "Dual Berettas";
    case 3: return "Five-SeveN";
    case 4: return "Glock-18";
    case 7: return "AK-47";
    case 8: return "AUG";
    case 9: return "AWP";
    case 10: return "FAMAS";
    case 11: return "G3SG1";
    case 13: return "Galil AR";
    case 14: return "M249";
    case 16: return "M4A4";
    case 17: return "MAC-10";
    case 19: return "P90";
    case 20: return "Zone Repulsor";
    case 23: return "MP5-SD";
    case 24: return "UMP-45";
    case 25: return "XM1014";
    case 26: return "PP-Bizon";
    case 27: return "MAG-7";
    case 28: return "Negev";
    case 29: return "Sawed-Off";
    case 30: return "Tec-9";
    case 31: return "Zeus x27";
    case 32: return "P2000";
    case 33: return "MP7";
    case 34: return "MP9";
    case 35: return "Nova";
    case 36: return "P250";
    case 37: return "Riot Shield";
    case 38: return "SCAR-20";
    case 39: return "SG 553";
    case 40: return "SSG 08";
    case 41: return "Golden Knife";
    case 42: return "Knife";
    case 43: return "Flashbang";
    case 44: return "HE Grenade";
    case 45: return "Smoke Grenade";
    case 46: return "Molotov";
    case 47: return "Decoy";
    case 48: return "Incendiary Grenade";
    case 49: return "C4";
    case 50: return "Healthshot";
    case 59: return "Knife (T)";
    case 60: return "M4A1-S";
    case 61: return "USP-S";
    case 63: return "CZ75-Auto";
    case 64: return "R8 Revolver";
    case 68: return "Tactical Awareness Grenade";
    case 69: return "Fists";
    case 70: return "Breach Charge";
    case 72: return "Tablet";
    case 74: return "Melee";
    case 75: return "Axe";
    case 76: return "Hammer";
    case 78: return "Wrench";
    case 80: return "Ghost Knife";
    case 81: return "Firebomb";
    case 82: return "Diversion Device";
    case 83: return "Frag Grenade";
    case 84: return "Snowball";
    case 85: return "Bump Mine";
    
    case 500: return "Bayonet";
    case 503: return "Classic Knife";
    case 505: return "Flip Knife";
    case 506: return "Gut Knife";
    case 507: return "Karambit";
    case 508: return "M9 Bayonet";
    case 509: return "Huntsman Knife";
    case 512: return "Falchion Knife";
    case 514: return "Bowie Knife";
    case 515: return "Butterfly Knife";
    case 516: return "Shadow Daggers";
    case 517: return "Paracord Knife";
    case 518: return "Survival Knife";
    case 519: return "Ursus Knife";
    case 520: return "Navaja Knife";
    case 521: return "Nomad Knife";
    case 522: return "Stiletto Knife";
    case 523: return "Talon Knife";
    case 525: return "Skeleton Knife";
    case 526: return "Kukri Knife";

    default: return "Unknown";
    }
}


std::string getWeaponFromId(short weapon_id) {
    for (auto entry : weaponNames) {
        if (entry.second == weapon_id) {
            return entry.first;
        }
    }
    return "";
}