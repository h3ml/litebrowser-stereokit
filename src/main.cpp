#include "globals.h"
#include "defaults.h"
#include "browser_wnd.h"
#include "..\containers\cairo\cairo_font.h"
#include <sstream>
using namespace std;
using namespace Gdiplus;

browser_wnd browser(0);

CRITICAL_SECTION cairo_font::m_sync;

#pragma region Log

pose_t log_pose = { {-0.75f, -0.1f, -0.5f}, quat_lookat(vec3_zero, vec3_forward) };
vector<string> log_list;
string log_text = "";

void log_handler(log_ level, const char* text) {
	if (log_list.size() > 15) log_list.erase(log_list.end() - 1);
	log_list.insert(log_list.begin(), strlen(text) < 100 ? std::string(text) : std::string(text, 100) + "...");
	stringstream b;
	for (vector<string>::iterator iter = log_list.begin(); iter < log_list.end(); iter++) b << *iter;
	log_text = b.str();
}

void log_update() {
	ui_window_begin("Log", log_pose, vec2{ 40.0f, 0.0f }*0.01f);
	ui_text(log_text.c_str());
	ui_window_end();
}

#pragma endregion

#pragma region Floor

shader_t floor_shader;
material_t floor_material;

void floor_create() {
	floor_shader = shader_create_file("floor.hlsl");
	floor_material = material_create(floor_shader);
	material_set_transparency(floor_material, transparency_blend);
}

void floor_update() {
	if (sk_system_info().display_type != display_blend_opaque) return;
	matrix *transform;
	if (world_has_bounds()) {
		pose_t pose = world_get_bounds_pose();
		transform = &matrix_trs(pose.position, pose.orientation, { 30.f, 0.1f, 30.0f });
	}
	else {
		transform = &matrix_trs({ 0.0f, -1.5f, 0.0f }, quat_identity, { 30.f, 0.1f, 30.0f });
	}
	mesh_draw(defaults::meshCube, floor_material, *transform);
}

#pragma endregion

int main(int argc, char* argv[]) {
#if _WIN32
	CoInitialize(NULL);
	InitCommonControls();
	InitializeCriticalSectionAndSpinCount(&cairo_font::m_sync, 1000);
	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
#endif

	log_subscribe(log_handler);

	sk_settings_t settings = {};
	settings.app_name = "litebrowser";
	settings.assets_folder = "assets";
	settings.display_preference = display_mode_mixedreality;
	if (!sk_init(settings))
		return 1;

	floor_create();
	defaults::create();
	browser.create();
	browser.open(argc > 1 ? (LPCWSTR)argv[0] : L"http://www.litehtml.com/");

	sk_run([]() {
		floor_update();
		log_update();
		browser.update();
		});

#if _WIN32
	GdiplusShutdown(gdiplusToken);
#endif
	return 0;
}