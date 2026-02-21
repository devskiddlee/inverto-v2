#include "modules/modules.h"
#include "offset_parser.h"
#include "cpu_raycast.hpp"

#define TIME_POINT std::chrono::high_resolution_clock::time_point
#define TIME_PERIOD std::chrono::high_resolution_clock::duration
#define NOW std::chrono::high_resolution_clock::now()
#define TIME_SINCE(p) std::chrono::duration<float, std::milli>(NOW - p).count()
#define SLEEP(t) std::this_thread::sleep_for(std::chrono::duration<float, std::milli>(t))

ImFont* console_font;
ImFont* menu_font;
ImGuiIO* io_ptr;
bool debug = false;
bool debug_map = false;
bool setup = false;

HWND cs2_hwnd = NULL;
BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
	DWORD windowPid = 0;
	GetWindowThreadProcessId(hwnd, &windowPid);

	if (windowPid == (DWORD)lParam) {
		if (GetWindow(hwnd, GW_OWNER) == NULL && IsWindowVisible(hwnd)) {
			cs2_hwnd = hwnd;
			return FALSE;
		}
	}
	return TRUE;
};

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK window_procedure(HWND window, UINT message, WPARAM w_param, LPARAM l_param) {
	if (ImGui_ImplWin32_WndProcHandler(window, message, w_param, l_param)) {
		return 0L;
	}

	if (message == WM_DESTROY) {
		PostQuitMessage(0);
		return 0L;
	}

	return DefWindowProc(window, message, w_param, l_param);
}

void ReadConfig(const char* name) {
	std::ostringstream ss;
	ss << "assets\\" << name << ".config";
	std::ifstream in(ss.str(), std::ios::binary);
	in.read(reinterpret_cast<char*>(&G::S), sizeof(G::S));
	G::current_config = std::string(name);
	confirm("Config " + G::current_config + " loaded");
}

void WriteConfig(const char* name) {
	std::ostringstream ss;
	ss << "assets\\" << name << ".config";
	std::ofstream out(ss.str(), std::ios::binary);
	out.write(reinterpret_cast<const char*>(&G::S), sizeof(G::S));
}

bool CheckConfig(const char* name) {
	std::ostringstream ss;
	ss << "assets\\" << name << ".config";
	return std::filesystem::exists(ss.str());
}

void ReadTheme(const char* name) {
	std::ostringstream ss;
	ss << "assets\\" << name << ".theme";
	std::ifstream in(ss.str(), std::ios::binary);
	in.read(reinterpret_cast<char*>(&G::T), sizeof(G::T));
	G::current_theme = std::string(name);
	confirm("Theme " + G::current_theme + " loaded");
}

void WriteTheme(const char* name) {
	std::ostringstream ss;
	ss << "assets\\" << name << ".theme";
	std::ofstream out(ss.str(), std::ios::binary);
	out.write(reinterpret_cast<const char*>(&G::T), sizeof(G::T));
}

bool CheckTheme(const char* name) {
	std::ostringstream ss;
	ss << "assets\\" << name << ".theme";
	return std::filesystem::exists(ss.str());
}

void padTrianglesToMultipleOf4(
	std::vector<Vec3>& v0,
	std::vector<Vec3>& v1,
	std::vector<Vec3>& v2)
{
	size_t n = v0.size();
	size_t remainder = n % 4;
	if (remainder == 0) return;

	size_t pad = 4 - remainder;

	Vec3 dummy = { 0.0f, 0.0f, 0.0f };
	for (size_t i = 0; i < pad; ++i) {
		v0.push_back(dummy);
		v1.push_back(dummy);
		v2.push_back(dummy);
	}
}

std::thread map_parser_thread;
std::string lastMapName;
std::vector<Vec3> loaded_triangles_p1;
std::vector<Vec3> loaded_triangles_p2;
std::vector<Vec3> loaded_triangles_p3;
void map_parse_loop() {
	while (map_parser_thread.joinable()) {
		if ((lastMapName != G::mapName && !debug_map) || (debug_map && G::triangles_loaded.size() == 0)) {
			lastMapName = G::mapName;
			if (debug_map) lastMapName = "de_inferno";

			if (!std::filesystem::exists("assets\\maps\\" + lastMapName + ".tri")) {
				if (lastMapName != "" && lastMapName != "<empty>" && lastMapName.find("_") != std::string::npos)
					warning("Current Map '" + lastMapName + "' could not be loaded, this may affect some features.");
				continue;
			}

			std::vector<Triangle> loaded;
			{
				std::ifstream in("assets\\maps\\" + lastMapName + ".tri", std::ios::binary);

				in.seekg(0, std::ios::end);
				std::streamsize fileSize = in.tellg();
				in.seekg(0, std::ios::beg);

				size_t count = fileSize / sizeof(Triangle);
				loaded.resize(count);

				in.read(reinterpret_cast<char*>(loaded.data()), count * sizeof(Triangle));
			}

			loaded_triangles_p1.clear();
			loaded_triangles_p2.clear();
			loaded_triangles_p3.clear();
			for (auto& T : loaded) {
				loaded_triangles_p1.emplace_back( T.p1.x, T.p1.y, T.p1.z );
				loaded_triangles_p2.emplace_back( T.p2.x, T.p2.y, T.p2.z );
				loaded_triangles_p3.emplace_back( T.p3.x, T.p3.y, T.p3.z );
			}
			padTrianglesToMultipleOf4(
				loaded_triangles_p1,
				loaded_triangles_p2,
				loaded_triangles_p3
			);

			G::triangles_loaded = loaded;
			info("Map '" + lastMapName + "' loaded (" + str(loaded.size()) + " triangles).");
		}
	}
}

std::vector<Vector> collisionPoints;
std::vector<Triangle> collisionTriangles;
std::thread enemy_visibility_thread;

int vis_ticks = 0;
float vis_time = 0.f;

void enemy_visibility_loop(){
	while (enemy_visibility_thread.joinable()) {
		auto start = std::chrono::high_resolution_clock::now();
		bool invalid_map = G::triangles_loaded.size() == 0;

		if (invalid_map && !G::S.thorough_vis_check) {
			std::this_thread::sleep_for(std::chrono::duration<int, std::milli>(50));
			continue;
		}

		std::list<Entity> entities = Reader::GetEntities();

		if (debug_map) {
			Entity e;
			e.head = Vector(500.f, 500.f, 500.f);
			e.id = "0";
			entities.push_back(e);
		}

		for (auto& e : entities) {
			Vector S = G::localPlayer.head;
			Vector E = e.head;
			if (debug_map) S = Vector(100.f, 100.f, 100.f);

			bool hit = false;
			if (!invalid_map) {
				if (G::use_AVX_512) {
					hit = anyHitAVX512(
						{ S.x, S.y, S.z },
						{ E.x, E.y, E.z },
						loaded_triangles_p1,
						loaded_triangles_p2,
						loaded_triangles_p3
					);
				}
				else {
					hit = anyHitSIMD(
						{ S.x, S.y, S.z },
						{ E.x, E.y, E.z },
						loaded_triangles_p1,
						loaded_triangles_p2,
						loaded_triangles_p3
					);
				}
			}

			bool thorough = G::memory.Read<bool>(e.address + G::offsets.spottedState + G::offsets.spotted);

			G::visibleMap[e.id] = !hit && (thorough || !G::S.thorough_vis_check);
		}

		auto end = std::chrono::high_resolution_clock::now();

		auto elapsed_time = std::chrono::duration<float, std::milli>(end - start).count();

		vis_ticks++;
		vis_time += elapsed_time;
		if (vis_time > 1000) {
			G::avg_vis_time = vis_time / vis_ticks;
			vis_time = 0;
			vis_ticks = 0;
		}
	}
}

void op() {
	std::string off;
	off = parseOffsets();

	G::offsets.entityList = getOffset("dwEntityList", off);
	G::offsets.localPlayer = getOffset("dwLocalPlayerPawn", off);
	G::offsets.localController = getOffset("dwLocalPlayerController", off);
	G::offsets.viewmatrix = getOffset("dwViewMatrix", off);
	G::offsets.viewangles = getOffset("dwViewAngles", off);
	G::offsets.gameRules = getOffset("dwGameRules", off);
	G::offsets.globalVars = getOffset("dwGlobalVars", off);
	G::offsets.planted_c4 = getOffset("dwPlantedC4", off);
	G::offsets.dwWeaponC4 = getOffset("dwWeaponC4", off);
	G::offsets.playerpawn = getOffset("CCSPlayerController->m_hPlayerPawn", off);
	G::offsets.m_nKillCount = getOffset("CCSPlayerController->m_nKillCount", off);

	G::offsets.eyeAngles = getOffset("C_CSPlayerPawn->m_angEyeAngles", off);
	G::offsets.teamNum = getOffset("C_BaseEntity->m_iTeamNum", off);
	G::offsets.jumpFlag = getOffset("C_BaseEntity->m_fFlags", off);
	G::offsets.health = getOffset("C_BaseEntity->m_iHealth", off);
	G::offsets.origin = getOffset("C_BasePlayerPawn->m_vOldOrigin", off);
	G::offsets.weapon_services = getOffset("C_BasePlayerPawn->m_pWeaponServices", off);
	G::offsets.m_pViewModelServices = getOffset("C_CSPlayerPawnBase->m_pViewModelServices", off);
	G::offsets.m_hViewModel = getOffset("CCSPlayer_ViewModelServices->m_hViewModel", off);
	G::offsets.m_nViewModelIndex = getOffset("C_BaseViewModel->m_nViewModelIndex", off);
	G::offsets.modelState = getOffset("CSkeletonInstance->m_modelState", off);
	G::offsets.gameScene = getOffset("C_BaseEntity->m_pGameSceneNode", off);
	G::offsets.spottedState = getOffset("C_CSPlayerPawn->m_entitySpottedState", off);
	G::offsets.lifeState = getOffset("C_BaseEntity->m_lifeState", off);

	G::offsets.camService = getOffset("C_BasePlayerPawn->m_pCameraServices", off);
	G::offsets.scoped = getOffset("C_CSPlayerPawn->m_bIsScoped", off);
	G::offsets.fov = getOffset("CCSPlayerBase_CameraServices->m_iFOV", off);
	G::offsets.absVelocity = getOffset("C_BaseEntity->m_vecAbsVelocity", off);
	G::offsets.IDEntIndex = getOffset("C_CSPlayerPawn->m_iIDEntIndex", off);

	G::offsets.playersAliveCT = getOffset("C_CSPlayerPawn->m_nLastKillerIndex", off);
	G::offsets.playersAliveT = getOffset("C_CSPlayerPawn->m_flHitHeading", off);
	G::offsets.m_hActiveWeapon = getOffset("CPlayer_WeaponServices->m_hActiveWeapon", off);
	G::offsets.aimPunchAngle = getOffset("C_CSPlayerPawn->m_aimPunchAngle", off);
	G::offsets.iShotsFired = getOffset("C_CSPlayerPawn->m_iShotsFired", off);

	G::offsets.clippingWeapon = getOffset("C_CSPlayerPawn->m_pClippingWeapon", off);
	G::offsets.m_iAmmoLastCheck = getOffset("C_CSWeaponBase->m_iAmmoLastCheck", off);
	G::offsets.vOldOrigin = getOffset("C_BasePlayerPawn->m_vOldOrigin", off);
	G::offsets.AttributeManager = getOffset("C_EconEntity->m_AttributeManager", off);

	G::offsets.m_hController = getOffset("C_BasePlayerPawn->m_hController", off);
	G::offsets.steamid = getOffset("CBasePlayerController->m_steamID", off);
	G::offsets.playerName = getOffset("CBasePlayerController->m_iszPlayerName", off);

	G::offsets.m_pActionTrackingServices = getOffset("CCSPlayerController->m_pActionTrackingServices", off);
	G::offsets.m_unTotalRoundDamageDealt = getOffset("CCSPlayerController_ActionTrackingServices->m_unTotalRoundDamageDealt", off);
	G::offsets.m_matchStats = getOffset("CCSPlayerController_ActionTrackingServices->m_matchStats", off);
	G::offsets.m_bInReload = getOffset("C_CSWeaponBase->m_bInReload", off);

	G::offsets.m_flFlashOverlayAlpha = getOffset("C_CSPlayerPawnBase->m_flFlashOverlayAlpha", off);

	G::offsets.m_vecAbsOrigin = getOffset("CGameSceneNode->m_vecAbsOrigin", off);

	G::offsets.m_flC4Blow = getOffset("C_PlantedC4->m_flC4Blow", off);

	G::offsets.m_ArmorValue = getOffset("C_CSPlayerPawn->m_ArmorValue", off);

	G::offsets.m_hOwnerEntity = getOffset("C_BaseEntity->m_hOwnerEntity", off);

	std::this_thread::sleep_for(std::chrono::milliseconds(250));

	confirm("All offsets have been loaded!");
	highlight("");

	if (!CheckConfig("default") || debug) {
		warning("Default Config not found... creating one...");
		WriteConfig("default");
	}

	ReadConfig("default");

	if (!CheckTheme("default") || debug) {
		warning("Default Theme not found... creating one...");
		WriteTheme("default");
	}

	ReadTheme("default");

	map_parser_thread = std::thread{ map_parse_loop };
	enemy_visibility_thread = std::thread{ enemy_visibility_loop };

	if (!cpuSupportsAVX512())
		warning("AVX-512 not supported on your CPU, fallback to AVX2");
	else
		G::use_AVX_512 = true;

	Modular::StartTickLoop();

	setup = true;
}

bool ColorPicker(ImColor* color) {
	ImVec4 c = *color;

	float vec[4];

	vec[0] = c.x;
	vec[1] = c.y;
	vec[2] = c.z;
	vec[3] = c.w;

	bool result = ImGui::ColorPicker4("Color", vec);

	*color = ImColor(vec[0], vec[1], vec[2], vec[3]);
	
	return result;
}

bool ColorPicker(ImVec4* color) {
	float vec[4];

	vec[0] = color->x;
	vec[1] = color->y;
	vec[2] = color->z;
	vec[3] = color->w;

	bool result = ImGui::ColorPicker4("Color", vec);

	*color = ImVec4(vec[0], vec[1], vec[2], vec[3]);

	return result;
}

void DebugStat(const char* label, float value, const char* suffix) {
	ImGui::TextColored(ImColor(200, 200, 200), label);
	std::ostringstream tt;
	tt << value;
	tt << suffix;
	ImGui::SameLine();
	ImGui::TextColored(ImColor(255, 0, 255), tt.str().c_str());
}

void DebugStat(const char* label, int value, const char* suffix) {
	ImGui::TextColored(ImColor(200, 200, 200), label);
	std::ostringstream tt;
	tt << value;
	tt << suffix;
	ImGui::SameLine();
	ImGui::TextColored(ImColor(255, 0, 255), tt.str().c_str());
}

std::unordered_map<std::string, bool> waiting_for_key;

static void Hotkey(const std::string& name, int* k, const ImVec2& size_arg = ImVec2(0, 0)) {
	if (!waiting_for_key[name]) {
		if (ImGui::Button((name + ": " + std::string(KeyNames[*(int*)k])).c_str(), size_arg))
			waiting_for_key[name] = true;
	}
	else {
		ImGui::Button((name + ": " + "...").c_str(), size_arg);
		for (auto& Key : KeyCodes)
		{
			if (GetAsyncKeyState(Key)) {
				*(int*)k = Key;
				waiting_for_key[name] = false;
			}
		}
	}
}

void PressKey(int vk) {
	INPUT ip;
	ip.type = INPUT_KEYBOARD;
	ip.ki.wScan = 0;
	ip.ki.time = 0;
	ip.ki.dwExtraInfo = 0;

	ip.ki.wVk = vk;
	ip.ki.dwFlags = 0;
	SendInput(1, &ip, sizeof(INPUT));

	ip.ki.dwFlags = KEYEVENTF_KEYUP;
	SendInput(1, &ip, sizeof(INPUT));
}

char config_input[255];
char theme_input[255];

INT APIENTRY WinMain(HINSTANCE instance, HINSTANCE, PSTR, INT cmd_show) {

	if (!std::filesystem::exists("assets\\default_font.ttf")) {
		MessageBoxA(0, "Please install inverto correctly", "default_font.ttf not found", MB_ICONERROR);
		return 0;
	}

	if (!std::filesystem::exists("assets\\menu_font.ttf")) {
		MessageBoxA(0, "Please install inverto correctly", "menu_font.ttf not found", MB_ICONERROR);
		return 0;
	}

	if (!std::filesystem::exists("assets\\maps")) {
		MessageBoxA(0, "Please install inverto correctly", "maps folder not found", MB_ICONERROR);
		return 0;
	}

	if (!G::memory.ProcessIsOpen("cs2.exe") && !debug) {
		MessageBoxA(0, "Please open Counter-Strike", "cs2.exe not found", MB_ICONERROR);
		return 0;
	}

	EnumWindows(EnumWindowsProc, G::memory.GetProcessID());
	G::client = G::memory.GetBase("client.dll");

	WNDCLASSEX wc{};
	wc.cbSize = sizeof(WNDCLASSEXW);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = window_procedure;
	wc.hInstance = instance;
	wc.lpszClassName = "Overlay Class";

	RegisterClassEx(&wc);

	const HWND window = CreateWindowEx(
		WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED,
		wc.lpszClassName,
		"Inverto",
		WS_POPUP,
		0,
		0,
		1920,
		1080,
		nullptr,
		nullptr,
		wc.hInstance,
		nullptr
	);

	SetLayeredWindowAttributes(window, RGB(0, 0, 0), BYTE(255), LWA_ALPHA);

	{
		RECT client_area{};
		GetClientRect(window, &client_area);

		RECT window_area{};
		GetWindowRect(window, &window_area);

		POINT diff{};
		ClientToScreen(window, &diff);

		const MARGINS margins{
			window_area.left + (diff.x - window_area.left),
			window_area.top + (diff.y - window_area.top),
			client_area.right,
			client_area.bottom
		};

		DwmExtendFrameIntoClientArea(window, &margins);
	}

	DXGI_SWAP_CHAIN_DESC sd{};
	sd.BufferDesc.RefreshRate.Numerator = 120U;
	sd.BufferDesc.RefreshRate.Denominator = 1U;
	sd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	sd.SampleDesc.Count = 1U;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = 2U;
	sd.OutputWindow = window;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	constexpr D3D_FEATURE_LEVEL levels[2]{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_0,
	};

	ID3D11Device* device{ nullptr };
	ID3D11DeviceContext* device_context{ nullptr };
	IDXGISwapChain* swap_chain{ nullptr };
	ID3D11RenderTargetView* render_target_view{ nullptr };
	D3D_FEATURE_LEVEL level{};

	D3D11CreateDeviceAndSwapChain(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		0U,
		levels,
		2U,
		D3D11_SDK_VERSION,
		&sd,
		&swap_chain,
		&device,
		&level,
		&device_context
	);

	ID3D11Texture2D* back_buffer{ nullptr };
	swap_chain->GetBuffer(0U, IID_PPV_ARGS(&back_buffer));

	if (back_buffer) {
		device->CreateRenderTargetView(back_buffer, nullptr, &render_target_view);
		back_buffer->Release();
	}
	else {
		return 1;
	}

	if (render_target_view == 0)
		return 1;

	ShowWindow(window, cmd_show);
	UpdateWindow(window);

	G::window = window;

	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX11_Init(device, device_context);

	ImGuiIO& io = ImGui::GetIO();
	console_font = io.Fonts->AddFontFromFileTTF("assets\\default_font.ttf");
	menu_font = io.Fonts->AddFontFromFileTTF("assets\\menu_font.ttf");
	G::default_font = console_font;
	G::menu_font = menu_font;
	io_ptr = &io;

	for (int i = 0; i < 58; i++)
		G::T.Colors[i] = ImGui::GetStyle().Colors[i];

	bool running = true;

	Modular::AddRenderEventHandler(Reader::OnRender);
	Modular::AddRenderEventHandler(Misc::OnRender);
	Modular::AddRenderEventHandler(PlantedC4::OnRender);
	Modular::AddRenderEventHandler(ESP::OnRender);
	Modular::AddRenderEventHandler(HUD::OnRender);
	Modular::AddRenderEventHandler(GameEvents::OnRender);
	Modular::AddRenderEventHandler(QuickToggle::OnRender);

	Modular::AddTickEventHandler(Reader::OnTick);
	Modular::AddTickEventHandler(Aimbot::OnTick);
	Modular::AddTickEventHandler(Misc::OnTick);
	Modular::AddTickEventHandler(HUD::OnTick);
	Modular::AddTickEventHandler(GameEvents::OnTick);

	Modular::AddKeyEventHandler(&G::S.menu_key, [window, &io](bool pressed) {
		if (pressed) {
			G::render_ui = !G::render_ui;
			SetWindowInteractivity(window, G::render_ui);
			if (G::render_ui) G::quick_toggle_enabled = false;
		}
	});

	Modular::AddKeyEventHandler(&G::S.QUICK_TOGGLE_HOTKEY, QuickToggle::OnToggle);
	Modular::AddKeyEventHandler(VK_RETURN, QuickToggle::OnEnter);
	Modular::AddKeyEventHandler(VK_LBUTTON, QuickToggle::OnEnter);
	Modular::AddKeyEventHandler(VK_UP, QuickToggle::OnUp);
	Modular::AddKeyEventHandler(VK_DOWN, QuickToggle::OnDown);

	//load offsets
	std::thread offset_parse_thread(op);
	float gradient_offset = 1.f;
	std::string last_offset_update = "";

	TIME_POINT lastFrameTP = NOW;

	#define LAST_FRAME_TIME_SIZE 64
	float lastFrameTimes[LAST_FRAME_TIME_SIZE] { 0 };
	size_t currentFrameTimeIndex = 0;
	auto nextFrameTime = NOW;

	while (running) {
		auto t_start = std::chrono::high_resolution_clock::now();
		auto frame_start = std::chrono::high_resolution_clock::now();

		MSG msg;
		while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);

			if (msg.message == WM_MOUSEWHEEL) {
				QuickToggle::OnScroll(GET_WHEEL_DELTA_WPARAM(msg.wParam));
			}

			if (msg.message == WM_QUIT)
				running = false;
		}

		if (!running || QuickToggle::wants_to_exit)
			break;

		float lastFrameTime = TIME_SINCE(lastFrameTP);
		lastFrameTP = NOW;

		if (lastFrameTime > 0.f) {
			lastFrameTimes[currentFrameTimeIndex++] = lastFrameTime;
			if (currentFrameTimeIndex == LAST_FRAME_TIME_SIZE) {
				float all = 0.f;
				for (size_t i = 0; i < LAST_FRAME_TIME_SIZE; i++) {
					all += lastFrameTimes[i];
				}
				G::avg_frame_time = all / LAST_FRAME_TIME_SIZE;
				currentFrameTimeIndex = 0;
			}
		}

		if (!G::S.vsync) {
			nextFrameTime += std::chrono::milliseconds((int)(1000.f / G::S.frame_cap));
			std::this_thread::sleep_until(nextFrameTime);
		}

		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();

		ImGui::NewFrame();

		ImDrawList* drawList = ImGui::GetBackgroundDrawList();

		bool cs2_focused = (GetForegroundWindow() == cs2_hwnd) || (GetForegroundWindow() == window);
		if (!cs2_focused)
			goto jmp_frame_end;

		if (!setup && OP::offset_parse_operation_update != "" && OP::offset_parse_operation_update != last_offset_update) {
			info(OP::offset_parse_operation_update);
			last_offset_update = OP::offset_parse_operation_update;
		}

		if (console_show > 0 && !G::S.console_disabled) {
			int current_y = 100;
			float bg_width = 0;
			std::list<console_message> console_to_render;
			for (auto& cmsg : G::console) {
				auto n = std::chrono::high_resolution_clock::now();
				double elapsed_time_ms = std::chrono::duration<double, std::milli>(n - cmsg.issued).count();
				if (elapsed_time_ms > 10000) {
					continue;
				}

				ImVec2 text_size = console_font->CalcTextSizeA(20.f, 1000.f, 1000.f, cmsg.content.c_str());
				bg_width = max(bg_width, text_size.x);
				console_to_render.push_back(cmsg);
			}
			if (console_to_render.size() > 0)
				drawList->AddRectFilled(ImVec2(90, 90), ImVec2(100 + bg_width + 10, 100 + 21 * (float)console_to_render.size() + 10), ImColor(0, 0, 0));
			for (auto& cmsg : console_to_render) {
				const char* c_str = cmsg.content.c_str();
				drawList->AddText(console_font, 20.f, ImVec2(100, (float)current_y), cmsg.color, c_str, 0, 1000.f);
				current_y += 21;
			}
		}

		if (G::S.showVisibilityCollisions) {
			ViewMatrix currentVM = G::memory.Read<ViewMatrix>(G::client + G::offsets.viewmatrix);
			std::list<float> distances;
			for (Vector& P : collisionPoints) {
				distances.push_back(CalcMagnitude(P, G::localPlayer.head));
				draw3dBoxAroundLine(
					drawList, currentVM,
					P.copy() + Vector(0, 0, 5),
					P.copy() - Vector(0, 0, 5),
					1.0, 0.0, 5.f, G::S.boxEspWidth, G::S.boxColor
				);
			}
			distances.sort();
			float wallbang_dist = distances.back() - distances.front();
			std::string wallbang_str = "WB: " + str(wallbang_dist);
			float text_width = G::default_font->CalcTextSizeA(30.f, 1000, 1000, wallbang_str.c_str()).x;
			drawList->AddText(G::default_font, 30.f, { G::windowSize.x / 2 - text_width / 2, 300 }, ImColor(255, 0, 255), wallbang_str.c_str());
		}

		if (setup) {
			RenderEvent event;
			event.drawList = drawList;
			event.last_draw_time = lastFrameTime / 1000.f;
			Modular::CallRenderEvent(event);
		}

		if (G::render_ui) {
			PushMenuStyle();

			ImGui::PushFont(menu_font, G::T.menu_fontSize);

			ImVec2 title_pos;
			ImGui::Begin(" ", 0, 0, &title_pos);

			ImDrawList* windowDrawList = ImGui::GetForegroundDrawList();

			ImColor title_color = G::T.Colors[ImGuiCol_Text];

			ImColor end;
			if (G::S.fancy_title)
				end = ContrastBrightnessHSV(title_color);
			else
				end = title_color;

			DrawGradientText(
				"inverto",
				title_pos,
				title_color,
				end,
				gradient_offset,
				G::menu_font,
				G::T.menu_fontSize,
				windowDrawList
			);

			if (ImGui::BeginTabBar("Tabs"))
			{
				if (ImGui::BeginTabItem("General"))
				{
					ImGui::Checkbox("Aimbot", &G::S.aimbot);
					ImGui::Checkbox("Thorough Visibility Check", &G::S.thorough_vis_check);
					ImGui::TextColored(ImColor(255, 0, 255), "INFO");
					ImGui::SameLine();
					ImGui::TextWrapped("This option will account for smokes, unsupported maps etc., because it checks the CS2 Map Visiblity Status of a player, this will add a slight delay");
					ImGui::Checkbox("Ignore Visibility Check", &G::S.ignoreVisible);
					ImGui::Checkbox("Auto-Aim when Visible?", &G::S.autoAimWhenVisible);
					if (!G::S.disableAngleDiff)
						ImGui::SliderFloat("Max Range", &G::S.maxAngleDiffAimbot, 10, 1000);
					ImGui::Checkbox("Disable Range", &G::S.disableAngleDiff);
					ImGui::SliderInt("Aimbot Speed", &G::S.aimbotspeed, 500, 4000);
					ImGui::SeparatorText("");
					ImGui::Checkbox("Triggerbot", &G::S.triggerbot);
					ImGui::Checkbox("Only shoot when still?", &G::S.onlyShootWhenStill);
					ImGui::SliderFloat("Default Shoot Delay", &G::S.default_shoot_delay, 50, 1000, "%.3f ms");
					ImGui::SeparatorText("");
					ImGui::Checkbox("Recoil Control", &G::S.rcs);
					ImGui::Checkbox("Bunnyhop", &G::S.bhop);
					ImGui::Checkbox("Jump Shot", &G::S.jumpShotHack);
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("ESP")) {
					if (ImGui::BeginMenu("Wallhack"))
					{
						ImGui::Checkbox("Wallhack", &G::S.esp);
						ColorPicker(&G::S.normalColor);
						ImGui::EndMenu();
					}
					if (ImGui::BeginMenu("Closest Enemy"))
					{
						ColorPicker(&G::S.closestEnemy);
						ImGui::EndMenu();
					}
					if (ImGui::BeginMenu("Aimlocked Enemy"))
					{
						ColorPicker(&G::S.aimLockedEnemy);
						ImGui::EndMenu();
					}
					if (ImGui::BeginMenu("Direction Tracers"))
					{
						ImGui::Checkbox("Direction Tracers", &G::S.directionTracer);
						ImGui::SliderFloat("Max Length", &G::S.directionTracerMaxLength, 10.f, 1000.f);
						ColorPicker(&G::S.directionCrosshair);
						ImGui::EndMenu();
					}
					if (ImGui::BeginMenu("Health"))
					{
						ImGui::Checkbox("Health Text", &G::S.healthText);
						ImGui::Checkbox("Health Box" , &G::S.healthBox );
						ColorPicker(&G::S.health);
						ImGui::EndMenu();
					}
					if (ImGui::BeginMenu("Player Names"))
					{
						ImGui::Checkbox("Player Names", &G::S.name);
						ColorPicker(&G::S.playerText);
						ImGui::EndMenu();
					}
					if (ImGui::BeginMenu("Player Armor"))
					{
						ImGui::Checkbox("Player Armor", &G::S.armorText);
						ColorPicker(&G::S.armorTextColor);
						ImGui::EndMenu();
					}
					if (ImGui::BeginMenu("Weapon Info"))
					{
						ImGui::Checkbox("Weapon Info", &G::S.weaponText);
						ColorPicker(&G::S.weaponTextColor);
						ImGui::EndMenu();
					}
					if (ImGui::BeginMenu("Bones"))
					{
						ImGui::Checkbox("Bones", &G::S.bone_esp);
						ImGui::SliderFloat("Line Width", &G::S.width, 1.f, 10.f);
						ColorPicker(&G::S.boneColor);
						ImGui::EndMenu();
					}
					if (ImGui::BeginMenu("3D Box"))
					{
						ImGui::Checkbox("3D Box", &G::S.boxEsp);
						ImGui::SliderFloat("Line Width", &G::S.boxEspWidth, 1.f, 10.f);
						ColorPicker(&G::S.boxColor);
						ImGui::EndMenu();
					}
					if (ImGui::BeginMenu("Chams"))
					{
						ImGui::Checkbox("Chams", &G::S.chams);
						ImGui::SliderFloat("Line Width", &G::S.chamsWidth, 1.f, 10.f);
						ColorPicker(&G::S.chamsColor);
						ImGui::EndMenu();
					}
					if (ImGui::BeginMenu("Planted C4"))
					{
						ImGui::Checkbox("Planted C4", &G::S.c4_esp);
						ImGui::Checkbox("Cross / Box", &G::S.c4_cross);
						ImGui::SliderFloat("Line Width", &G::S.c4_line_width, 1.f, 10.f);
						ColorPicker(&G::S.c4_color);
						ImGui::EndMenu();
					}
					ImGui::Checkbox("Absolute Text Size?", &G::S.absolute_text_size);
					ImGui::Checkbox("Show only if visible", &G::S.espOnlyWhenVisible);
					ImGui::Checkbox("Show only nearest info", &G::S.show_only_nearest_info);
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("HUD")) {
					
					ImGui::PushID(1);
					ImGui::SeparatorText("Kill Animation");

					ImGui::Checkbox("Enable", &G::S.kill_animation);
					ImGui::SliderFloat("Duration", &G::S.kill_animation_duration, 0.1f, 5.f, "%.1f seconds");
					ImGui::SliderInt("Size", &G::S.kill_animation_size, 5, 100, "%dpx");
					if (ImGui::BeginMenu("Color")) {
						ColorPicker(&G::S.kill_animation_color);
						ImGui::EndMenu();
					}
					ImGui::PopID();

					ImGui::PushID(2);
					ImGui::SeparatorText("Music / Media Module");
					ImGui::Checkbox("Enable", &G::S.spotify_module);
					ImGui::SliderFloat("Gradient Speed", &G::S.spotify_module_gradient_speed, 0.f, 10.f);

					ImGui::SliderFloat("X", &G::S.spotify_module_pos.x, 0, G::windowSize.x);
					ImGui::SliderFloat("Y", &G::S.spotify_module_pos.y, 0, G::windowSize.y);

					ImGui::SliderFloat("Font Size", &G::S.spotify_module_font_size, 5.f, 50.f, "%.0fpt");

					if (ImGui::BeginMenu("Gradient Color 1")) {
						ColorPicker(&G::S.spotify_module_color_start);
						ImGui::EndMenu();
					}

					if (ImGui::BeginMenu("Gradient Color 2")) {
						ColorPicker(&G::S.spotify_module_color_end);
						ImGui::EndMenu();
					}

					if (ImGui::BeginMenu("Background")) {
						ColorPicker(&G::S.spotify_module_bg_color);
						ImGui::EndMenu();
					}
					ImGui::PopID();

					ImGui::PushID(3);
					ImGui::SeparatorText("FPS Module");
					ImGui::Checkbox("Enable", &G::S.fps_module);
					ImGui::Checkbox("Also Display Tickspeed", &G::S.fps_module_tickspeed);
					ImGui::Checkbox("Also Display VisTickspeed", &G::S.fps_module_vistickspeed);
					ImGui::SliderFloat("Gradient Speed", &G::S.fps_module_gradient_speed, 0.f, 10.f);

					ImGui::SliderFloat("X", &G::S.fps_module_pos.x, 0, G::windowSize.x);
					ImGui::SliderFloat("Y", &G::S.fps_module_pos.y, 0, G::windowSize.y);

					ImGui::SliderFloat("Font Size", &G::S.fps_module_font_size, 5.f, 50.f, "%.0fpt");

					if (ImGui::BeginMenu("Gradient Color 1")) {
						ColorPicker(&G::S.fps_module_color_start);
						ImGui::EndMenu();
					}

					if (ImGui::BeginMenu("Gradient Color 2")) {
						ColorPicker(&G::S.fps_module_color_end);
						ImGui::EndMenu();
					}

					if (ImGui::BeginMenu("Background")) {
						ColorPicker(&G::S.fps_module_bg_color);
						ImGui::EndMenu();
					}

					ImGui::PopID();

					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Misc")) {
					if (ImGui::BeginMenu("Anti Flashbang")) {
						ImGui::Checkbox("Anti Flashbang", &G::S.anti_flashbang);
						ColorPicker(&G::S.anti_flashbang_color);
						ImGui::Checkbox("Render World when flashed", &G::S.anti_flashbang_world_render);
						ImGui::SliderFloat("Render World Radius", &G::S.anti_flashbang_world_render_radius, 100.f, 1000.f);

						ImGui::TextColored(ImColor(255, 0, 0), "NOTICE");
						ImGui::SameLine();
						ImGui::TextWrapped("This option can be incredibly laggy depending on your cpu");

						ImGui::EndMenu();
					}
					ImGui::Checkbox("Check Team?", &G::S.teamCheck);
					ImGui::Text("");
					if (ImGui::Button("Exit Inverto")) {
						running = false;
					}
					ImGui::Text("");
					ImGui::Checkbox("VSync", &G::S.vsync);
					if (!G::S.vsync) {
						if (ImGui::SliderInt("max FPS", &G::S.frame_cap, 30, 999, "%d FPS")) {
							nextFrameTime = NOW;
						}
					}

					ImGui::TextColored(ImColor(255, 0, 0), "NOTICE");
					ImGui::SameLine();
					ImGui::TextWrapped("It is important that the CS2 frames and Inverto render frames are in-sync. Therefore use VSync in both or cap it to the same FPS.");
					
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Configs")) {
					ImGui::TextColored(ImVec4(0.f, 1.f, 0.f, 1.f), ("Current Config: " + G::current_config).c_str());
					if (ImGui::Button(("Save " + G::current_config).c_str())) {
						WriteConfig(G::current_config.c_str());
					}
					ImGui::SeparatorText("Create Config");
					ImGui::InputText("##xx", config_input, sizeof(config_input));
					if (std::string(config_input).length() > 0) {
						if (is_valid_filename(config_input) && !CheckConfig(config_input)) {
							if (ImGui::Button(("Create " + std::string(config_input) + ".config").c_str())) {
								WriteConfig(config_input);
								confirm(std::string(config_input) + ".config created");
								config_input[0] = 0;
							}
						}
						else {
							ImGui::TextColored(ImColor(255, 0, 0), "Already exists or Invalid Filename");
						}
					}
					ImGui::SeparatorText("Configs");
					int ix = 0;
					for (auto& f : get_files_with_extension("assets", ".config")) {
						std::ostringstream ss;
						ss << f;
						std::string cfg_name = getBetween(ss.str(), "assets\\\\", ".config");
						ImGui::Text(cfg_name.c_str());
						ImGui::PushID(ix);
						ImGui::SameLine();
						if (ImGui::Button("Load")) {
							ReadConfig(cfg_name.c_str());
						}
						if (cfg_name != "default") {
							ImGui::SameLine();
							if (ImGui::Button("Delete")) {
								if (cfg_name == G::current_config)
									ReadConfig("default");
								std::filesystem::remove(f);
							}
						}
						ImGui::PopID();
						ix++;
					}
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Themes")) {

					ImGui::TextColored(ImVec4(0.f, 1.f, 0.f, 1.f), ("Current Theme: " + G::current_theme).c_str());
					if (ImGui::Button(("Save " + G::current_theme).c_str())) {
						WriteTheme(G::current_theme.c_str());
					}
					ImGui::SeparatorText("Create Theme");
					ImGui::InputText("##xx", theme_input, sizeof(theme_input));
					if (std::string(theme_input).length() > 0) {
						if (is_valid_filename(theme_input) && !CheckTheme(theme_input)) {
							if (ImGui::Button(("Create " + std::string(theme_input) + ".theme").c_str())) {
								WriteTheme(theme_input);
								confirm(std::string(theme_input) + ".theme created");
								theme_input[0] = 0;
							}
						}
						else {
							ImGui::TextColored(ImColor(255, 0, 0), "Already exists or Invalid Filename");
						}
					}
					ImGui::SeparatorText("Themes");
					int ix = 0;
					for (auto& f : get_files_with_extension("assets", ".theme")) {
						std::ostringstream ss;
						ss << f;
						std::string theme_name = getBetween(ss.str(), "assets\\\\", ".theme");
						ImGui::Text(theme_name.c_str());

						ImGui::PushID(ix);
						ImGui::SameLine();
						if (ImGui::Button("Load")) {
							ReadTheme(theme_name.c_str());
						}

						if (theme_name != "default") {
							ImGui::SameLine();
							if (ImGui::Button("Delete")) {
								if (theme_name == G::current_theme)
									ReadTheme("default");
								std::filesystem::remove(f);
							}
						}
						ImGui::PopID();
						ix++;
					}

					ImGui::SeparatorText("Edit UI Settings");

					ImGui::Checkbox("Fancy Title", &G::S.fancy_title);
					ImGui::SliderInt("Font Size", &G::T.menu_fontSize, 5, 50, "%dpt");
					ImGui::SliderFloat("Frame Rounding", &G::T.menu_frameRounding, 0.f, 20.f);
					ImGui::SliderFloat("Window Rounding", &G::T.menu_windowRounding, 0.f, 20.f);

					ImGui::SeparatorText("Edit Colors");

					if (ImGui::Button("Dark Preset")) {
						ImGui::StyleColorsDark();
						for (int i = 0; i < 58; i++)
							G::T.Colors[i] = ImGui::GetStyle().Colors[i];
					}

					ImGui::SameLine();
					if (ImGui::Button("Light Preset")) {
						ImGui::StyleColorsLight();
						for (int i = 0; i < 58; i++)
							G::T.Colors[i] = ImGui::GetStyle().Colors[i];
					}

					ImGui::SameLine();
					if (ImGui::Button("Classic Preset")) {
						ImGui::StyleColorsClassic();
						for (int i = 0; i < 58; i++)
							G::T.Colors[i] = ImGui::GetStyle().Colors[i];
					}

					for (int i = 0; i < 58; i++) {
						const char* name = ImGuiColToString(i);
						if (name != "" && ImGui::BeginMenu(name)) {
							ColorPicker(&G::T.Colors[i]);
							ImGui::EndMenu();
						}
					}

					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Players")) {
					int ix = 0;
					for (Entity& player : G::render_entities) {
						if (player.steam_id == 0)
							continue;

						ImGui::PushID(ix);
						if (ImGui::BeginMenu(player.name.c_str())) {
							ImGui::TextColored(ImColor(200, 200, 200), "SteamID: ");
							ImGui::SameLine();
							ImGui::TextColored(ImColor(200, 0, 200), str(player.steam_id).c_str());
							if (ImGui::Button("Open Stats [csst.at]")) {
								ShellExecute(0, 0, ("https://csst.at/profile/" + str(player.steam_id)).c_str(), 0, 0, SW_SHOW);
							}
							ImGui::EndMenu();
						}
						ImGui::PopID();
						ix++;
					}
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Controls")) {
					Hotkey("Toggle Menu", &G::S.menu_key);
					Hotkey("Quick Toggle Menu", &G::S.QUICK_TOGGLE_HOTKEY);
					Hotkey("Aimbot", &G::S.AIMBOT_KEY);
					Hotkey("Bhop", &G::S.BHOP_KEY);
					Hotkey("Jump Shot", &G::S.JUMPSHOT_HOTKEY);
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Debug Info")) {

					ImGui::SeparatorText("Tick Timing");

					DebugStat("Time per Tick: ", Modular::GetAverageTickTime(), "ms");
					DebugStat("Ticks per Second: ", (int)(1000 / Modular::GetAverageTickTime()), "t/s");

					ImGui::SeparatorText("Frame Timing");

					DebugStat("Time per Frame: ", G::avg_frame_time, "ms");
					DebugStat("Frames per Second: ", (int)(1000 / G::avg_frame_time), "f/s");

					ImGui::SeparatorText("Collision Detection");

					DebugStat("Time per VisTick: ", G::avg_vis_time, "ms");
					DebugStat("VisTicks per Second: ", (int)(1000 / G::avg_vis_time), "vt/s");

					ImGui::SeparatorText("Velocity");

					ImGui::Checkbox("Show Velocity", &G::S.showVelocity);

					ImGui::SeparatorText("Console");

					ImGui::Checkbox("Disable Console", &G::S.console_disabled);

					ImGui::EndTabItem();
				}

			}

			PopMenuStyle();
			ImGui::PopFont();
			ImGui::EndTabBar();
			ImGui::End();
		}

		jmp_frame_end:

		ImGui::Render();

		constexpr float color[4]{ 0.f, 0.f, 0.f, 0.f };
		device_context->OMSetRenderTargets(1U, &render_target_view, nullptr);
		device_context->ClearRenderTargetView(render_target_view, color);

		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		if (G::S.vsync)
			swap_chain->Present(1U, 0U);
		else
			swap_chain->Present(0U, 0U);

		gradient_offset -= lastFrameTime / 1000.f;
		if (gradient_offset < 0.f) gradient_offset = 1.f;
	}

	map_parser_thread.detach();
	enemy_visibility_thread.detach();

	Modular::StopTickLoop();

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();

	ImGui::DestroyContext();

	if (swap_chain)
		swap_chain->Release();
	if (device_context)
		device_context->Release();
	if (device)
		device->Release();
	if (render_target_view)
		render_target_view->Release();

	DestroyWindow(window);
	UnregisterClass(wc.lpszClassName, wc.hInstance);

	return 0;
}