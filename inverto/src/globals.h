#pragma once
#include <mem/memify.h>
#include "offsets.h"
#include "utils.hpp"
#include "modular.h"

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
	float jumpShotThreshold = 18.f;

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
    bool radarHack = true;
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

    std::vector<ImVec2> to_screen(ViewMatrix currentVM) {
        Vector sp1(-10000, -10000, 0);
        Vector sp2(-10000, -10000, 0);
        Vector sp3(-10000, -10000, 0);
        world_to_screen(p1, sp1, currentVM);
        world_to_screen(p2, sp2, currentVM);
        world_to_screen(p3, sp3, currentVM);
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
    std::unordered_map<std::string, float> time_alive;

	std::list<Entity> entities;
	std::list<Entity> render_entities;

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

	Entity nearest_player;
	Entity render_nearest_player;
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

	Settings S{};
    Theme T{};
}

int console_show = 0;

void send_console_message(std::string content, ImColor color)
{
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
    float m_flIntervalPerTick; //0x0030
    float m_flCurrentTime; //0x0034
    float m_flCurrentTime2; //0x0038
    char pad_003C[20]; //0x003C
    int32_t m_nTickCount; //0x0050
    char pad_0054[292]; //0x0054
    uint64_t m_uCurrentMap; //0x0178
    uint64_t m_uCurrentMapName; //0x0180
}; //Size: 0x0188

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

std::list<Vector> GetCuboidCorners(Vector start, Vector end, float width, float height)
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

void draw3dBoxAroundLine(ImDrawList* drawList, ViewMatrix currentViewMatrix, Vector start, Vector end, double cos, double sin, float size, bool outline = true, bool straight = true)
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

    drawList->AddConvexPolyFilled(pts, (int)o_points_vec2.size(), G::S.boxColor);

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

            drawList->AddLine(last_point.toVec2(), pt.toVec2(), color, G::S.boxEspWidth);
            last_point = pt;
        }
        drawList->AddLine(last_point.toVec2(), o_points[0].toVec2(), color, G::S.boxEspWidth);
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