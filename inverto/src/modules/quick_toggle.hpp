#include "module_includes.h"

// [https://github.com/ocornut/imgui/blob/master/misc/cpp/imgui_stdlib.cpp]
struct InputTextCallback_UserData {
    std::string* Str;
    ImGuiInputTextCallback  ChainCallback;
    void* ChainCallbackUserData;
};

static int InputTextCallback(ImGuiInputTextCallbackData* data) {
    InputTextCallback_UserData* user_data = (InputTextCallback_UserData*)data->UserData;
    if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
    {
        std::string* str = user_data->Str;
        IM_ASSERT(data->Buf == str->c_str());
        str->resize(data->BufTextLen);
        data->Buf = (char*)str->c_str();
    }
    else if (user_data->ChainCallback)
    {
        data->UserData = user_data->ChainCallbackUserData;
        return user_data->ChainCallback(data);
    }
    return 0;
}

bool InputText(const char* label, std::string* str, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data) {
    IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
    flags |= ImGuiInputTextFlags_CallbackResize;

    InputTextCallback_UserData cb_user_data;
    cb_user_data.Str = str;
    cb_user_data.ChainCallback = callback;
    cb_user_data.ChainCallbackUserData = user_data;
    return ImGui::InputText(label, (char*)str->c_str(), str->capacity() + 1, flags, InputTextCallback, &cb_user_data);
}

class QuickToggle {
public:
    inline static bool wants_to_exit = false;
private:
	inline static float max_width = 600.f;
	inline static float height = 250.f;
    inline static float height_adjust = 0.f;

    inline static std::string content;
    inline static int index = 0;

	static void DrawSearch() {
        ImGui::Text("Search");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::SetKeyboardFocusHere();
        InputText("##", &content, 0, 0, 0);
	}

    static inline std::map<std::string, bool*> cheat_toggles {
        { "Triggerbot", &G::S.triggerbot },
        { "Aimbot", &G::S.aimbot },
        { "Ignore Visibility Check", &G::S.ignoreVisible },
        { "Thorough Visibility Check", &G::S.thorough_vis_check },
        { "Auto-Aim when Visible?", &G::S.autoAimWhenVisible },
        { "Disable Range", &G::S.disableAngleDiff },
        { "Only shoot when still?", &G::S.onlyShootWhenStill },
        { "Recoil Control", &G::S.rcs },
        { "Bunnyhop", &G::S.bhop },
        { "Jump Shot", &G::S.jumpShotHack },
        { "Wallhack", &G::S.esp },
        { "Direction Tracers", &G::S.directionTracer },
        { "Health Text", &G::S.healthText },
        { "Health Box" , &G::S.healthBox },
        { "Player Names", &G::S.name },
        { "Bones", &G::S.bone_esp },
        { "3D Box", &G::S.boxEsp },
        { "Chams", &G::S.chams },
        { "Absolute Text Size?", &G::S.absolute_text_size },
        { "Wallhack > Show only if visible", &G::S.espOnlyWhenVisible },
        { "Wallhack > Show only nearest info", &G::S.show_only_nearest_info },
        { "Kill Animation", &G::S.kill_animation },
        { "Music / Media Module", &G::S.spotify_module },
        { "FPS Module", &G::S.fps_module },
        { "FPS Module > Display Tickspeed", &G::S.fps_module_tickspeed },
        { "FPS Module > Display VisTickspeed", &G::S.fps_module_vistickspeed },
        { "Anti Flashbang", &G::S.anti_flashbang },
        { "Check Team?", &G::S.teamCheck },
        { "Exit Inverto", &wants_to_exit },
        { "VSync", &G::S.vsync },
        { "Fancy Title", &G::S.fancy_title },
        { "Show Velocity", &G::S.showVelocity },
        { "Planted C4", &G::S.c4_esp },
        { "Planted C4 > Cross / Box", &G::S.c4_cross },
        { "Render World when flashed", &G::S.anti_flashbang_world_render }
    };

    inline static float gradient_offset = 1.f;
public:

    static void OnToggle(bool pressed) {
        if (pressed) {
            G::quick_toggle_enabled = !G::quick_toggle_enabled;
            SetWindowInteractivity(G::window, G::quick_toggle_enabled);
            if (G::quick_toggle_enabled) {
                G::render_ui = false;
                content = "";
                index = 0;
            }
        }
    }

    static void OnEnter(bool pressed) {
        if (!G::quick_toggle_enabled) return;
        if (content.empty()) return;

        std::vector<std::string> to_compare;
        for (const auto& c : cheat_toggles) {
            to_compare.push_back(c.first);
        }
        sort_by_fuzzy_score(to_compare, content);

        if (pressed) {
            bool* b = cheat_toggles[to_compare[index]];
            *b = !*b;
        }
    }

    static void OnUp(bool pressed) {
        if (!G::quick_toggle_enabled) return;
        if (pressed) index--;
    }

    static void OnDown(bool pressed) {
        if (!G::quick_toggle_enabled) return;
        if (pressed) index++;
    }

    static void OnScroll(short y) {
        if (!G::quick_toggle_enabled) return;
        if (y > 0) index--;
        if (y < 0) index++;
    }

	static void OnRender(RenderEvent event) {
		if (!G::quick_toggle_enabled) return;

        gradient_offset -= event.last_draw_time;
        if (gradient_offset < 0.f) gradient_offset = 1.f;

		ImGuiWindowFlags flags = 
			ImGuiWindowFlags_NoTitleBar		|
			ImGuiWindowFlags_NoResize		|
			ImGuiWindowFlags_NoMove
		;

		float width  = fminf(max_width , G::windowSize.x / 2.f);

        PushMenuStyle();
		ImGui::SetNextWindowPos ({ G::windowCenter.x - width / 2.f, G::windowCenter.y - height / 2.f - height_adjust });
		ImGui::SetNextWindowSize({ width, -1 });
		ImGui::Begin("QuickToggle", 0, flags);

        DrawSearch();
        if (content.empty()) {
            PopMenuStyle();
            ImGui::End();
            return;
        }

        std::vector<std::string> to_compare;
        for (const auto& c : cheat_toggles) {
            to_compare.push_back(c.first);
        }
        sort_by_fuzzy_score(to_compare, content);

        index = std::min({ (int)to_compare.size() - 1, max(0, index) });

        ImGui::Separator();
        for (size_t i = 0; i < to_compare.size(); i++) {
            int a = 128;
            int r = 255;
            int g = 0;
            if (*cheat_toggles[to_compare[i]]) {
                g = 255;
                r = 0;
            }
            if (i == index) {
                a = 255;
                ImGui::TextColored(ImColor(r, g, 0, a), "~");
                ImGui::SameLine();
                DrawGradientText(
                    to_compare[i].c_str(),
                    ImGui::GetCursorScreenPos(),
                    ImColor(255, 0, 255),
                    ImColor(r, g, 0, a),
                    gradient_offset,
                    G::default_font,
                    G::T.menu_fontSize,
                    ImGui::GetForegroundDrawList()
                );
                ImGui::Text("");
            }
            else {
                ImGui::TextColored(ImColor(r, g, 0, a), to_compare[i].c_str());
            }
        }

        PopMenuStyle();
		ImGui::End();
	}
};