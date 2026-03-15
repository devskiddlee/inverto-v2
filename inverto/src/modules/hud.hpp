#include "module_includes.h"

class HUD {
private:
	inline static float custom_text_gradient_offset = 1.f;
	inline static float spotify_gradient_offset = 1.f;
	inline static float fsp_gradient_offset = 1.f;
	inline static std::pair<std::string, std::string> media = { "", "" };
	inline static float media_cooldown = 1.f;

	static void drawArcCircle(const ImVec2& position, float perc, float maxPerc, float radius, const ImColor& color) {
		float a_max = ((float)M_PI * 2.0f);
		float v1 = perc / maxPerc;
		float difference = v1 - 1.0f;
		ImGui::GetBackgroundDrawList()->PathArcTo(
			position, radius,
			(-(a_max / 4.0f)) + (a_max / maxPerc) * (maxPerc - perc),
			a_max - (a_max / 4.0f),
			200 - 1
		);
		ImGui::GetBackgroundDrawList()->PathStroke(
			color,
			ImDrawFlags_None,
			2.0f
		);
	}
public:
	static void OnTick(const TickEvent& event) {
		media_cooldown -= event.delta_time;
		if (media_cooldown < 0.f) {
			media_cooldown = 1.f;
			media = get_media();
		}
	}

	static void OnRender(const RenderEvent& event) {
		if (G::S.ammoCircle) {
			uintptr_t clippingWeapon = G::memory.Read<uintptr_t>(G::localPlayer.address + G::offsets.clippingWeapon);
			uintptr_t weaponData = getWeaponVData(clippingWeapon);
			int32_t m_iMaxClip1 = G::memory.Read<int32_t>(weaponData + G::offsets.m_iMaxClip1);
			int32_t m_iClip1 = G::memory.Read<int32_t>(clippingWeapon + G::offsets.m_iClip1);

			drawArcCircle(
				G::windowCenter.toVec2(),
				(float)m_iClip1,
				(float)m_iMaxClip1,
				G::S.ammoCircleSize,
				G::S.ammoCircleColor
			);
		}

		if (G::S.spotify_module) {
			std::string s = format("Playing: {} - {}", media.first, media.second);
			const char* text = s.c_str();

			float pad = G::S.spotify_module_padding;
			ImVec2 text_size = G::menu_font->CalcTextSizeA(G::S.spotify_module_font_size, FLT_MAX, 0.f, text);
			event.drawList->AddRectFilled(
				{ G::S.spotify_module_pos.x - pad * 2.f, G::S.spotify_module_pos.y - pad },
				{ G::S.spotify_module_pos.x + text_size.x + pad * 2.f, G::S.spotify_module_pos.y + text_size.y + pad },
				G::S.spotify_module_bg_color,
				G::S.spotify_module_rounding
			);

			auto end = G::S.spotify_module_color_end;
			if (G::S.spotify_module_gradient_speed == 0.f) end = G::S.spotify_module_color_start;

			DrawGradientText(
				text,
				G::S.spotify_module_pos,
				G::S.spotify_module_color_start,
				end,
				spotify_gradient_offset,
				G::menu_font,
				G::S.spotify_module_font_size
			);

			spotify_gradient_offset -= event.last_draw_time * G::S.spotify_module_gradient_speed;
			if (spotify_gradient_offset < 0.f) spotify_gradient_offset = 1.f;
		}

		if (G::S.fps_module) {
			std::string s = str((int)(1000 / G::avg_frame_time)) + "f/s";
			if (G::S.fps_module_tickspeed) s += "  " + str((int)(1000 / Modular::GetAverageTickTime())) + "t/s";
			if (G::S.fps_module_vistickspeed) s += "  " + str((int)(1000 / G::avg_vis_time)) + "vt/s";
			const char* text = s.c_str();

			float pad = G::S.fps_module_padding;
			ImVec2 text_size = G::menu_font->CalcTextSizeA(G::S.fps_module_font_size, FLT_MAX, 0.f, text);
			event.drawList->AddRectFilled(
				{ G::S.fps_module_pos.x - pad * 2.f, G::S.fps_module_pos.y - pad },
				{ G::S.fps_module_pos.x + text_size.x + pad * 2.f, G::S.fps_module_pos.y + text_size.y + pad },
				G::S.fps_module_bg_color,
				G::S.fps_module_rounding
			);

			auto end = G::S.fps_module_color_end;
			if (G::S.fps_module_gradient_speed == 0.f) end = G::S.fps_module_color_start;

			DrawGradientText(
				text,
				G::S.fps_module_pos,
				G::S.fps_module_color_start,
				end,
				fsp_gradient_offset,
				G::menu_font,
				G::S.fps_module_font_size
			);

			fsp_gradient_offset -= event.last_draw_time * G::S.fps_module_gradient_speed;
			if (fsp_gradient_offset < 0.f) fsp_gradient_offset = 1.f;
		}

		if (G::S.custom_text_module) {
			const char* text = G::S.custom_text_content;

			float pad = G::S.custom_text_module_padding;
			ImVec2 text_size = G::menu_font->CalcTextSizeA(G::S.custom_text_module_font_size, FLT_MAX, 0.f, text);
			event.drawList->AddRectFilled(
				{ G::S.custom_text_module_pos.x - pad * 2.f, G::S.custom_text_module_pos.y - pad },
				{ G::S.custom_text_module_pos.x + text_size.x + pad * 2.f, G::S.custom_text_module_pos.y + text_size.y + pad },
				G::S.custom_text_module_bg_color,
				G::S.custom_text_module_rounding
			);

			auto end = G::S.custom_text_module_color_end;
			if (G::S.custom_text_module_gradient_speed == 0.f) end = G::S.custom_text_module_color_start;

			DrawGradientText(
				text,
				G::S.custom_text_module_pos,
				G::S.custom_text_module_color_start,
				end,
				custom_text_gradient_offset,
				G::menu_font,
				G::S.custom_text_module_font_size
			);

			custom_text_gradient_offset -= event.last_draw_time * G::S.custom_text_module_gradient_speed;
			if (custom_text_gradient_offset < 0.f) custom_text_gradient_offset = 1.f;
		}
	}
};