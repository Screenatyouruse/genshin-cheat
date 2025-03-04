#include <pch.h>
#include "renderer.h"

#include <backends/imgui_impl_dx11.h>
#include <backends/imgui_impl_win32.h>

#include <cheat-base/util.h>
#include <cheat-base/render/backend/dx11-hook.h>

#include <cheat-base/cheat/CheatManagerBase.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace renderer
{
	static std::unordered_set<void*> inputLockers;

	static ImFont* pFont;
	
	static LPBYTE pFontData;
	static DWORD dFontDataSize;

	static WNDPROC OriginalWndProcHandler;
	static ID3D11RenderTargetView* mainRenderTargetView;

	static void OnRender(ID3D11DeviceContext* pContext);
	static void OnDX11Initialize(HWND window, ID3D11Device* pDevice, ID3D11DeviceContext* pContext, IDXGISwapChain* pChain);

	void Init(LPBYTE fontData, DWORD fontDataSize)
	{
		pFontData = fontData;
		dFontDataSize = fontDataSize;

		LOG_DEBUG("Initialize IMGui...");


		backend::DX11Events::RenderEvent += FREE_METHOD_HANDLER(OnRender);
		backend::DX11Events::InitializeEvent += FREE_METHOD_HANDLER(OnDX11Initialize);

		backend::InitializeDX11Hooks();
	}

	void SetInputLock(void* id, bool value)
	{
		if (value)
			AddInputLocker(id);
		else
			RemoveInputLocker(id);
	}

	void AddInputLocker(void* id)
	{
		if (inputLockers.count(id) == 0)
			inputLockers.insert(id);
	}

	void RemoveInputLocker(void* id)
	{
		if (inputLockers.count(id) > 0)
			inputLockers.erase(id);
	}

	bool IsInputLocked()
	{
		return inputLockers.size() > 0;
	}

	static void SetupImGuiStyle();
	static LRESULT CALLBACK hWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	static void OnDX11Initialize(HWND window, ID3D11Device* pDevice, ID3D11DeviceContext* pContext, IDXGISwapChain* pChain)
	{

		LOG_DEBUG("ImGUI: DirectX11 backend initialized successfully.");

		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

		if (pFontData != nullptr)
			pFont = io.Fonts->AddFontFromMemoryTTF(pFontData, dFontDataSize, 16, nullptr, io.Fonts->GetGlyphRangesCyrillic());
		else
			pFont = io.FontDefault;

		SetupImGuiStyle();

		//Set OriginalWndProcHandler to the Address of the Original WndProc function
		OriginalWndProcHandler = reinterpret_cast<WNDPROC>(SetWindowLongPtr(window, GWLP_WNDPROC,
			reinterpret_cast<LONG_PTR>(hWndProc)));

		ImGui_ImplWin32_Init(window);
		ImGui_ImplDX11_Init(pDevice, pContext);

		ID3D11Texture2D* pBackBuffer;
		pChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<LPVOID*>(&pBackBuffer));
		pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &mainRenderTargetView);
		pBackBuffer->Release();

		io.SetPlatformImeDataFn = nullptr; // F**king bug take 4 hours of my life
	}

	static void OnRender(ID3D11DeviceContext* pContext)
	{
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();

		ImGui::NewFrame();
		ImGui::PushFont(pFont);

		renderer::events::RenderEvent();

		ImGui::PopFont();
		ImGui::EndFrame();
		ImGui::Render();

		pContext->OMSetRenderTargets(1, &mainRenderTargetView, nullptr);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	}

	static LRESULT CALLBACK hWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		ImGuiIO& io = ImGui::GetIO();
		POINT mPos;
		GetCursorPos(&mPos);
		ScreenToClient(hWnd, &mPos);
		ImGui::GetIO().MousePos.x = mPos.x;
		ImGui::GetIO().MousePos.y = mPos.y;

		ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);

		if (!cheat::events::WndProcEvent(hWnd, uMsg, wParam, lParam))
			return true;

		short key;
		bool keyUpEvent = true;
		switch (uMsg)
		{
		case WM_LBUTTONUP:
			key = VK_LBUTTON;
			break;
		case WM_RBUTTONUP:
			key = VK_RBUTTON;
			break;
		case WM_MBUTTONUP:
			key = VK_MBUTTON;
			break;
		case WM_XBUTTONUP:
			key = GET_XBUTTON_WPARAM(wParam);
			break;
		case WM_KEYUP:
			key = wParam;
			break;
		default:
			keyUpEvent = false;
			break;
		}

		bool canceled = false;
		if (keyUpEvent)
			canceled = !cheat::events::KeyUpEvent(key);

		if (IsInputLocked() || canceled)
			return true;

		return CallWindowProc(OriginalWndProcHandler, hWnd, uMsg, wParam, lParam);
	}


	static void SetupImGuiStyle()
	{
		ImGui::GetStyle().FrameRounding = 4.0f;
		ImGui::GetStyle().GrabRounding = 4.0f;

		ImVec4* colors = ImGui::GetStyle().Colors;
		colors[ImGuiCol_Text] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
		colors[ImGuiCol_TextDisabled] = ImVec4(0.17f, 0.21f, 0.24f, 1.00f);
		colors[ImGuiCol_WindowBg] = ImVec4(0.12f, 0.15f, 0.14f, 1.00f);
		colors[ImGuiCol_ChildBg] = ImVec4(0.12f, 0.15f, 0.17f, 1.00f);
		colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.09f, 0.11f, 0.94f);
		colors[ImGuiCol_Border] = ImVec4(0.15f, 0.22f, 0.25f, 1.00f);
		colors[ImGuiCol_BorderShadow] = ImVec4(0.20f, 0.20f, 0.20f, 0.00f);
		colors[ImGuiCol_FrameBg] = ImVec4(0.18f, 0.25f, 0.27f, 1.00f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.15f, 0.19f, 0.24f, 1.00f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.19f, 0.22f, 0.24f, 1.00f);
		colors[ImGuiCol_TitleBg] = ImVec4(0.14f, 0.18f, 0.22f, 0.65f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.13f, 0.16f, 0.19f, 1.00f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.15f, 0.15f, 0.15f, 0.51f);
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.12f, 0.14f, 0.18f, 1.00f);
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.11f, 0.11f, 0.11f, 0.39f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.53f, 0.37f, 0.37f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.49f, 0.23f, 0.23f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.56f, 0.10f, 0.10f, 1.00f);
		colors[ImGuiCol_CheckMark] = ImVec4(0.65f, 0.74f, 0.86f, 1.00f);
		colors[ImGuiCol_SliderGrab] = ImVec4(0.35f, 0.47f, 0.68f, 1.00f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(0.68f, 0.80f, 1.00f, 1.00f);
		colors[ImGuiCol_Button] = ImVec4(0.14f, 0.19f, 0.24f, 1.00f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.27f, 0.30f, 0.44f, 1.00f);
		colors[ImGuiCol_ButtonActive] = ImVec4(0.28f, 0.29f, 0.41f, 1.00f);
		colors[ImGuiCol_Header] = ImVec4(0.00f, 0.00f, 0.00f, 0.24f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.12f, 0.16f, 0.20f, 0.80f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.00f, 0.44f, 0.92f, 1.00f);
		colors[ImGuiCol_Separator] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
		colors[ImGuiCol_SeparatorHovered] = ImVec4(0.31f, 0.45f, 0.60f, 0.78f);
		colors[ImGuiCol_SeparatorActive] = ImVec4(0.42f, 0.57f, 0.75f, 1.00f);
		colors[ImGuiCol_ResizeGrip] = ImVec4(0.64f, 0.79f, 0.98f, 0.25f);
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.65f, 0.75f, 0.87f, 0.67f);
		colors[ImGuiCol_ResizeGripActive] = ImVec4(0.43f, 0.55f, 0.70f, 0.95f);
		colors[ImGuiCol_Tab] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
		colors[ImGuiCol_TabHovered] = ImVec4(0.59f, 0.59f, 0.59f, 0.80f);
		colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
		colors[ImGuiCol_TabUnfocused] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
		colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
		colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
		colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
		colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
		colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
		colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
		colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
		colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
		colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
		colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
		colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
		colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
	}
}