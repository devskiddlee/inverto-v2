#include "module_includes.h"

class HUD {
private:
	inline static float spotify_gradient_offset = 1.f;
	inline static float fsp_gradient_offset = 1.f;
	inline static std::pair<std::string, std::string> media = { "", "" };
	inline static float media_cooldown = 1.f;
public:
	static void OnTick(TickEvent event) {
		media_cooldown -= event.delta_time;
		if (media_cooldown < 0.f) {
			media_cooldown = 1.f;
			media = get_media();
		}
	}

	static void OnRender(RenderEvent event) {
		if (G::S.spotify_module) {
			std::string s = format("Playing: {} - {}", media.first, media.second);
			const char* text = s.c_str();

			ImVec2 text_size = G::menu_font->CalcTextSizeA(G::S.spotify_module_font_size, FLT_MAX, 0.f, text);
			event.drawList->AddRectFilled(
				{ G::S.spotify_module_pos.x - 5, G::S.spotify_module_pos.y - 5 },
				{ G::S.spotify_module_pos.x + text_size.x + 5, G::S.spotify_module_pos.y + text_size.y + 5 },
				G::S.spotify_module_bg_color,
				5.f
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

			ImVec2 text_size = G::menu_font->CalcTextSizeA(G::S.fps_module_font_size, FLT_MAX, 0.f, text);
			event.drawList->AddRectFilled(
				{ G::S.fps_module_pos.x - 5, G::S.fps_module_pos.y - 5 },
				{ G::S.fps_module_pos.x + text_size.x + 5, G::S.fps_module_pos.y + text_size.y + 5 },
				G::S.fps_module_bg_color,
				5.f
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
	}
};