#pragma once
// Glory theme: لون خلفية + صورة خلفية لقائمة ReShade (Fork)
#include <windows.h>
#include <objbase.h>
#include <commdlg.h>
#include <wincodec.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <atomic>
#include <mutex>
#include <thread>
#include <vector>
#include <cstdio>
#include <cstring>
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "comdlg32.lib")

namespace glory
{
	struct state
	{
		float bg[3] = { 0.08f, 0.08f, 0.10f };
		bool use_color = true, use_image = false, cfg_loaded = false, reload = false, has_picked = false;
		float opacity = 0.25f;
		char path[512] = "", picked[512] = "", ini[MAX_PATH] = "";
		reshade::api::resource tex = { 0 };
		reshade::api::resource_view srv = { 0 };
		reshade::api::device *dev = nullptr;
		std::atomic<bool> browsing { false };
		std::mutex mtx;
	};
	inline state g;

	inline void save_cfg()
	{
		char b[128];
		std::snprintf(b, sizeof(b), "%f,%f,%f", g.bg[0], g.bg[1], g.bg[2]);
		WritePrivateProfileStringA("GLORY", "Color", b, g.ini);
		WritePrivateProfileStringA("GLORY", "UseColor", g.use_color ? "1" : "0", g.ini);
		WritePrivateProfileStringA("GLORY", "UseImage", g.use_image ? "1" : "0", g.ini);
		std::snprintf(b, sizeof(b), "%f", g.opacity);
		WritePrivateProfileStringA("GLORY", "Opacity", b, g.ini);
		WritePrivateProfileStringA("GLORY", "Path", g.path, g.ini);
	}

	inline void load_cfg()
	{
		g.cfg_loaded = true;
		HMODULE self = nullptr;
		GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
			reinterpret_cast<LPCSTR>(&glory::load_cfg), &self);
		GetModuleFileNameA(self, g.ini, MAX_PATH);
		char *slash = std::strrchr(g.ini, '\\');
		if (slash) *(slash + 1) = '\0';
		std::strcat(g.ini, "Glory.ini");
		char b[512];
		GetPrivateProfileStringA("GLORY", "Color", "", b, sizeof(b), g.ini);
		if (b[0]) std::sscanf(b, "%f,%f,%f", &g.bg[0], &g.bg[1], &g.bg[2]);
		g.use_color = GetPrivateProfileIntA("GLORY", "UseColor", 1, g.ini) != 0;
		g.use_image = GetPrivateProfileIntA("GLORY", "UseImage", 0, g.ini) != 0;
		GetPrivateProfileStringA("GLORY", "Opacity", "0.25", b, sizeof(b), g.ini);
		std::sscanf(b, "%f", &g.opacity);
		GetPrivateProfileStringA("GLORY", "Path", "", g.path, sizeof(g.path), g.ini);
		g.reload = g.path[0] != '\0';
	}

	// يطبّق لون الخلفية على كل نوافذ القائمة (يُستدعى قبل ImGui::NewFrame)
	inline void apply_style()
	{
		if (!g.cfg_loaded) load_cfg();
		if (!g.use_color && !g.use_image) return;
		ImGuiStyle &s = ImGui::GetStyle();
		if (g.use_image && g.srv.handle)
		{
			// الخلفية (لون + صورة) ترسم خلف النافذة، فنخلي النافذة شفافة
			s.Colors[ImGuiCol_WindowBg] = ImVec4(0, 0, 0, 0);
			s.Colors[ImGuiCol_ChildBg] = ImVec4(0, 0, 0, 0);
		}
		else if (g.use_color)
		{
			const ImVec4 c(g.bg[0], g.bg[1], g.bg[2], 0.96f);
			s.Colors[ImGuiCol_WindowBg] = c;
			s.Colors[ImGuiCol_ChildBg] = ImVec4(c.x, c.y, c.z, 0.0f);
		}
		if (g.use_color)
			s.Colors[ImGuiCol_PopupBg] = ImVec4(g.bg[0], g.bg[1], g.bg[2], 0.98f);
	}

	inline void unload_image()
	{
		if (!g.dev) return;
		if (g.srv.handle) { g.dev->destroy_resource_view(g.srv); g.srv = { 0 }; }
		if (g.tex.handle) { g.dev->destroy_resource(g.tex); g.tex = { 0 }; }
	}

	inline bool load_image(const char *utf8)
	{
		unload_image();
		wchar_t w[1024];
		if (!MultiByteToWideChar(CP_UTF8, 0, utf8, -1, w, 1024)) return false;
		CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED); // قد يرجع RPC_E_CHANGED_MODE وهذا عادي
		IWICImagingFactory *f = nullptr;
		if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&f)))) return false;
		IWICBitmapDecoder *d = nullptr; IWICBitmapFrameDecode *fr = nullptr; IWICFormatConverter *c = nullptr;
		std::vector<BYTE> px; UINT W = 0, H = 0; bool ok = false;
		if (SUCCEEDED(f->CreateDecoderFromFilename(w, nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &d)) &&
			SUCCEEDED(d->GetFrame(0, &fr)) && SUCCEEDED(f->CreateFormatConverter(&c)) &&
			SUCCEEDED(c->Initialize(fr, GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom)) &&
			SUCCEEDED(c->GetSize(&W, &H)) && W > 0 && H > 0 && W <= 8192 && H <= 8192)
		{
			px.resize(size_t(W) * H * 4);
			ok = SUCCEEDED(c->CopyPixels(nullptr, W * 4, static_cast<UINT>(px.size()), px.data()));
		}
		if (c) c->Release(); if (fr) fr->Release(); if (d) d->Release(); f->Release();
		if (!ok) return false;

		reshade::api::subresource_data data = {};
		data.data = px.data(); data.row_pitch = W * 4; data.slice_pitch = W * H * 4;
		const reshade::api::resource_desc desc(W, H, 1, 1, reshade::api::format::r8g8b8a8_unorm, 1,
			reshade::api::memory_heap::gpu_only, reshade::api::resource_usage::shader_resource);
		return g.dev->create_resource(desc, &data, reshade::api::resource_usage::shader_resource, &g.tex) &&
			g.dev->create_resource_view(g.tex, reshade::api::resource_usage::shader_resource,
				reshade::api::resource_view_desc(reshade::api::resource_view_type::texture_2d, reshade::api::format::r8g8b8a8_unorm, 0, 1, 0, 1), &g.srv);
	}

	// نافذة الاختيار تشتغل في thread منفصل عشان ما تعلّق اللعبة
	inline void browse_thread(HWND owner)
	{
		CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
		wchar_t file[1024] = L"";
		OPENFILENAMEW o = {};
		o.lStructSize = sizeof(o); o.hwndOwner = owner;
		o.lpstrFilter = L"Images (*.png;*.jpg;*.jpeg;*.bmp)\0*.png;*.jpg;*.jpeg;*.bmp\0";
		o.lpstrFile = file; o.nMaxFile = 1024;
		o.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
		if (GetOpenFileNameW(&o))
		{
			char out[512];
			if (WideCharToMultiByte(CP_UTF8, 0, file, -1, out, sizeof(out), nullptr, nullptr) > 0)
			{
				std::lock_guard<std::mutex> l(g.mtx);
				std::strcpy(g.picked, out); g.has_picked = true;
			}
		}
		CoUninitialize();
		g.browsing = false;
	}

	// محتوى تبويب Background (أو النافذة العائمة)
	inline void draw_controls()
	{
		bool ch = false;
		ImGui::TextUnformatted("Glory - Background");
		ImGui::Separator();
		ch |= ImGui::Checkbox("Use background color", &g.use_color);
		ch |= ImGui::ColorEdit3("Background color", g.bg);
		ImGui::Spacing();
		ch |= ImGui::Checkbox("Use background image", &g.use_image);
		ch |= ImGui::SliderFloat("Image opacity", &g.opacity, 0.05f, 1.0f);
		ImGui::InputText("Image path", g.path, sizeof(g.path));
		if (ImGui::Button(g.browsing ? "Browsing..." : "Browse...") && !g.browsing)
		{
			g.browsing = true;
			std::thread(browse_thread, GetForegroundWindow()).detach();
		}
		ImGui::SameLine();
		if (ImGui::Button("Load image")) { g.reload = true; g.use_image = true; ch = true; }
		ImGui::SameLine();
		if (ImGui::Button("Remove image")) { g.use_image = false; g.path[0] = '\0'; unload_image(); ch = true; }
		if (ch) save_cfg();
	}

	// يُستدعى كل فريم قبل ImGui::Render: تحميل الصورة + رسم الخلفية خلف نافذة القائمة
	inline void frame(bool show, reshade::api::device *dev, bool floating)
	{
		g.dev = dev;
		if (!g.cfg_loaded) load_cfg();
		{
			std::lock_guard<std::mutex> l(g.mtx);
			if (g.has_picked) { std::strcpy(g.path, g.picked); g.has_picked = false; g.reload = true; g.use_image = true; save_cfg(); }
		}
		if (g.reload) { g.reload = false; if (!load_image(g.path)) g.use_image = false; }
		if (!show) return;

		if (g.use_color || (g.use_image && g.srv.handle))
		{
			ImGuiContext &c = *ImGui::GetCurrentContext();
			ImGuiWindow *best = nullptr; float best_area = 0.f;
			for (ImGuiWindow *w : c.Windows)
			{
				if (!w->Active || w->Hidden || (w->Flags & ImGuiWindowFlags_ChildWindow) || w->Size.x < 300 || w->Size.y < 200) continue;
				if (!std::strcmp(w->Name, "Glory Theme")) continue;
				if (!std::strstr(w->Name, "Glory") && !std::strstr(w->Name, "ReShade")) continue;
				const float area = w->Size.x * w->Size.y;
				if (area > best_area) { best = w; best_area = area; }
			}
			if (best)
			{
				ImDrawList *bg = ImGui::GetBackgroundDrawList();
				const ImVec2 p0 = best->Pos, p1(best->Pos.x + best->Size.x, best->Pos.y + best->Size.y);
				if (g.use_image && g.srv.handle)
				{
					if (g.use_color)
						bg->AddRectFilled(p0, p1, ImGui::ColorConvertFloat4ToU32(ImVec4(g.bg[0], g.bg[1], g.bg[2], 1.0f)));
					bg->AddImage((ImTextureID)g.srv.handle, p0, p1, ImVec2(0, 0), ImVec2(1, 1),
						IM_COL32(255, 255, 255, static_cast<int>(g.opacity * 255.f)));
				}
			}
		}

		if (floating)
		{
			ImGui::SetNextWindowPos(ImVec2(40, 40), ImGuiCond_FirstUseEver);
			if (ImGui::Begin("Glory Theme", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
				draw_controls();
			ImGui::End();
		}
	}
}
