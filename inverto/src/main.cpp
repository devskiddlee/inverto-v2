#include "modules/modules.h"
#include "offset_parser.h"

using namespace std;

ImFont* console_font;
ImFont* menu_font;
ImGuiIO* io_ptr;
bool debug = false;
bool setup = false;

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
	ostringstream ss;
	ss << "assets\\" << name << ".config";
	std::ifstream in(ss.str(), std::ios::binary);
	in.read(reinterpret_cast<char*>(&G::S), sizeof(G::S));
	G::current_config = std::string(name);
	confirm("Config " + G::current_config + " loaded");
}

void WriteConfig(const char* name) {
	ostringstream ss;
	ss << "assets\\" << name << ".config";
	std::ofstream out(ss.str(), std::ios::binary);
	out.write(reinterpret_cast<const char*>(&G::S), sizeof(G::S));
}

bool CheckConfig(const char* name) {
	ostringstream ss;
	ss << "assets\\" << name << ".config";
	return std::filesystem::exists(ss.str());
}

void ReadTheme(const char* name) {
	ostringstream ss;
	ss << "assets\\" << name << ".theme";
	std::ifstream in(ss.str(), std::ios::binary);
	in.read(reinterpret_cast<char*>(&G::T), sizeof(G::T));
	G::current_theme = std::string(name);
	confirm("Theme " + G::current_theme + " loaded");
}

void WriteTheme(const char* name) {
	ostringstream ss;
	ss << "assets\\" << name << ".theme";
	std::ofstream out(ss.str(), std::ios::binary);
	out.write(reinterpret_cast<const char*>(&G::T), sizeof(G::T));
}

bool CheckTheme(const char* name) {
	ostringstream ss;
	ss << "assets\\" << name << ".theme";
	return std::filesystem::exists(ss.str());
}

std::thread map_parser_thread;
std::string lastMapName;
void map_parse_loop() {
	while (map_parser_thread.joinable()) {
		if (lastMapName != G::mapName) {
			lastMapName = G::mapName;

			if (!std::filesystem::exists("assets\\maps\\" + lastMapName + ".tri")) {
				if (lastMapName != "" && lastMapName != "<empty>")
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
float avg_vis_time = 0.f;

void enemy_visibility_loop(){
	while (enemy_visibility_thread.joinable()) {
		auto start = std::chrono::high_resolution_clock::now();

		if (G::triangles_loaded.size() == 0) {
			std::this_thread::sleep_for(std::chrono::duration<int, std::milli>(50));
			continue;
		}

		std::list<Entity> entities = Reader::GetEntities();

		for (auto& e : entities) {
			Vector S = G::localPlayer.head;
			Vector E = e.head;

			bool v = true;

			float t;
			for (Triangle& T : G::triangles_loaded)
				if (T.intersect(S, E, &t)) {
					v = false;
					break;
				}

			G::visibleMap[e.id] = v;
		}

		auto end = std::chrono::high_resolution_clock::now();

		auto elapsed_time = std::chrono::duration<float, std::milli>(end - start).count();

		vis_ticks++;
		vis_time += elapsed_time;
		if (vis_time > 1000) {
			avg_vis_time = vis_time / vis_ticks;
			vis_time = 0;
			vis_ticks = 0;
		}
	}
}

void op() {
	string off;
	off = parseOffsets();

	G::offsets.entityList = getOffset("dwEntityList", off);
	G::offsets.localPlayer = getOffset("dwLocalPlayerPawn", off);
	G::offsets.localController = getOffset("dwLocalPlayerController", off);
	G::offsets.viewmatrix = getOffset("dwViewMatrix", off);
	G::offsets.viewangles = getOffset("dwViewAngles", off);
	G::offsets.gameRules = getOffset("dwGameRules", off);
	G::offsets.globalVars = getOffset("dwGlobalVars", off);
	G::offsets.playerpawn = getOffset("CCSPlayerController->m_hPlayerPawn", off);

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
	G::offsets.IDEntIndex = getOffset("C_CSPlayerPawnBase->m_iIDEntIndex", off);

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

	G::offsets.actionTrackingServices = getOffset("CCSPlayerController->m_pActionTrackingServices", off);
	G::offsets.damageDealt = getOffset("CCSPlayerController_ActionTrackingServices->m_unTotalRoundDamageDealt", off);
	G::offsets.m_bInReload = getOffset("C_CSWeaponBase->m_bInReload", off);

	G::offsets.m_flFlashOverlayAlpha = getOffset("C_CSPlayerPawnBase->m_flFlashOverlayAlpha", off);

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

	Modular::StartTickLoop();

	map_parser_thread = std::thread{ map_parse_loop };
	enemy_visibility_thread = std::thread{ enemy_visibility_loop };

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

void Hotkey(const std::string& name, int* k, const ImVec2& size_arg = ImVec2(0, 0))
{
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

void SetWindowInteractivity(HWND hwnd, bool interactive)
{
	LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);

	if (interactive)
	{
		exStyle &= ~WS_EX_TRANSPARENT;
	}
	else
	{
		exStyle |= WS_EX_TRANSPARENT;
	}

	SetWindowLong(hwnd, GWL_EXSTYLE, exStyle);

	if (interactive)
	{
		SetForegroundWindow(hwnd);
		SetFocus(hwnd);
	}
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

int frames = 0;
float frame_time = 0.f;
float avg_frame_time = 0.f;

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

	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX11_Init(device, device_context);

	ImGuiIO& io = ImGui::GetIO();
	console_font = io.Fonts->AddFontFromFileTTF("assets\\default_font.ttf");
	menu_font = io.Fonts->AddFontFromFileTTF("assets\\menu_font.ttf");
	G::default_font = console_font;
	io_ptr = &io;

	for (int i = 0; i < 58; i++)
		G::T.Colors[i] = ImGui::GetStyle().Colors[i];

	bool running = true;

	int fps = 0;
	float elapsed_second = 0.f;

	Modular::AddRenderEventHandler(Reader::OnRender);
	Modular::AddRenderEventHandler(Misc::OnRender);
	Modular::AddRenderEventHandler(ESP::OnRender);

	Modular::AddTickEventHandler(Reader::OnTick);
	Modular::AddTickEventHandler(Aimbot::OnTick);
	Modular::AddTickEventHandler(Misc::OnTick);

	Modular::AddKeyEventHandler(&G::S.menu_key, [window, &io](bool pressed) {
		if (pressed) {
			G::render_ui = !G::render_ui;
			SetWindowInteractivity(window, G::render_ui);
		}
	});

	//load offsets
	std::thread offset_parse_thread(op);

	while (running) {
		auto t_start = std::chrono::high_resolution_clock::now();

		MSG msg;
		while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);

			if (msg.message == WM_QUIT)
				running = false;
		}

		if (!running)
			break;

		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();

		ImGui::NewFrame();

		ImDrawList* drawList = ImGui::GetBackgroundDrawList();

		if (!setup && OP::offset_parse_operation_update != "")
			info(OP::offset_parse_operation_update);

		if (console_show > 0) {
			int current_y = 100;
			float bg_width = 0;
			std::list<console_message> console_to_render;
			for (auto cmsg : G::console) {
				auto n = chrono::high_resolution_clock::now();
				double elapsed_time_ms = std::chrono::duration<double, std::milli>(n - cmsg.issued).count();
				if (elapsed_time_ms > 10000)
					continue;

				ImVec2 text_size = console_font->CalcTextSizeA(20.f, 1000.f, 1000.f, cmsg.content.c_str());
				bg_width = max(bg_width, text_size.x);
				console_to_render.push_back(cmsg);
			}
			if (console_to_render.size() > 0)
				drawList->AddRectFilled(ImVec2(90, 90), ImVec2(100 + bg_width + 10, 100 + 21 * (float)console_to_render.size() + 10), ImColor(0, 0, 0));
			for (auto cmsg : console_to_render) {
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
					1.0, 0.0, 5.f
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
			Modular::CallRenderEvent(event);
		}

		if (G::render_ui) {
			PushMenuStyle();

			ImGui::PushFont(menu_font, G::T.menu_fontSize);

			ImGui::Begin("inverto");

			if (ImGui::BeginTabBar("Tabs"))
			{
				if (ImGui::BeginTabItem("General"))
				{
					ImGui::Checkbox("Aimbot", &G::S.aimbot);
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
					ImGui::Checkbox("Absolute Text Size?", &G::S.absolute_text_size);
					ImGui::Checkbox("Show only if visible", &G::S.espOnlyWhenVisible);
					ImGui::Checkbox("Show only nearest info", &G::S.show_only_nearest_info);
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Misc")) {
					if (ImGui::BeginMenu("Anti Flashbang")) {
						ImGui::Checkbox("Anti Flashbang", &G::S.anti_flashbang);
						ColorPicker(&G::S.anti_flashbang_color);
						ImGui::EndMenu();
					}
					ImGui::Checkbox("Check Team?", &G::S.teamCheck);
					ImGui::Text("");
					if (ImGui::Button("Exit Inverto")) {
						running = false;
					}
					ImGui::Text("");
					ImGui::Checkbox("VSync", &G::S.vsync);
					if (!G::S.vsync)
						ImGui::SliderInt("max FPS", &G::S.frame_cap, 30, 999, "%d FPS");

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

					DebugStat("Time per Frame: ", avg_frame_time, "ms");
					DebugStat("Frames per Second: ", (int)(1000 / avg_frame_time), "f/s");

					ImGui::SeparatorText("Collision Detection");

					DebugStat("Time per VisTick: ", avg_vis_time, "ms");
					DebugStat("VisTicks per Second: ", (int)(1000 / avg_vis_time), "vt/s");

					ImGui::SeparatorText("Velocity");

					ImGui::Checkbox("Show Velocity", &G::S.showVelocity);

					/*
					ImGui::SeparatorText("-insecure");

					ImGui::TextColored(ImColor(255, 0, 0), "NOTICE");
					ImGui::SameLine();
					ImGui::TextWrapped("Please enable -insecure before using these options, as these write to the game and will get you banned otherwise!");
					*/

					ImGui::EndTabItem();
				}
			}

			PopMenuStyle();
			ImGui::PopFont();
			ImGui::EndTabBar();
			ImGui::End();
		}

		ImGui::Render();

		constexpr float color[4]{ 0.f, 0.f, 0.f, 0.f };
		device_context->OMSetRenderTargets(1U, &render_target_view, nullptr);
		device_context->ClearRenderTargetView(render_target_view, color);

		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		if (G::S.vsync)
			swap_chain->Present(1U, 0U);
		else
			swap_chain->Present(0U, 0U);

		auto t_end = std::chrono::high_resolution_clock::now();

		double elapsed_time_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();

		if (console_show > 0) {
			console_show -= (int)elapsed_time_ms;
		}

		frames++;
		frame_time += elapsed_time_ms;
		if (frame_time > 1000) {
			avg_frame_time = frame_time / frames;
			frame_time = 0;
			frames = 0;
		}

		if (!G::S.vsync) {
			float to_sleep = 1000.f / G::S.frame_cap;
			to_sleep -= elapsed_time_ms;
			std::this_thread::sleep_for(std::chrono::duration<float, std::milli>(to_sleep));
		}
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