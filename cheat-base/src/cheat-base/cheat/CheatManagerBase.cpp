#include <pch.h>
#include "CheatManagerBase.h"

#include <misc/cpp/imgui_stdlib.h>

#include <cheat-base/Event.h>

#include <cheat-base/render/renderer.h>
#include <cheat-base/render/gui-util.h>
#include <cheat-base/cheat/misc/Settings.h>

namespace cheat
{
	namespace events
	{
		TCancelableEvent<short> KeyUpEvent{};
		TCancelableEvent<HWND, UINT, WPARAM, LPARAM> WndProcEvent{};
	}

	void CheatManagerBase::Init(LPBYTE pFontData, DWORD dFontDataSize)
	{
		renderer::events::RenderEvent += MY_METHOD_HANDLER(CheatManagerBase::OnRender);
		cheat::events::KeyUpEvent += MY_METHOD_HANDLER(CheatManagerBase::OnKeyUp);
		cheat::events::WndProcEvent += MY_METHOD_HANDLER(CheatManagerBase::OnWndProc);

		renderer::Init(pFontData, dFontDataSize);
	}

	CheatManagerBase::CheatManagerBase():
		NF(m_SelectedSection, "", "General", 0),
		m_IsMenuShowed(false),
		m_IsBlockingInput(true),
		m_IsPrevCursorActive(false)
	{
	}

	void CheatManagerBase::DrawExternal() const
	{
		for (auto& feature : m_Features)
		{
			ImGui::PushID(&feature);
			feature->DrawExternal();
			ImGui::PopID();
		}
	}

	void CheatManagerBase::DrawMenu()
	{
		if (m_ModuleOrder.empty())
			return;

		static std::string* current = &m_ModuleOrder[m_SelectedSection];

		ImGui::SetNextWindowSize(ImVec2(600, 300), ImGuiCond_FirstUseEver);

		if (!ImGui::Begin("CCGenshin (By Callow)"))
		{
			ImGui::End();
			return;
		}

		ImGui::BeginGroup();

		if (ImGui::Checkbox("Block key/mouse", &m_IsBlockingInput))
		{
			renderer::SetInputLock(this, m_IsBlockingInput);
		}

		if (ImGui::BeginListBox("##listbox 2", ImVec2(175, -FLT_MIN)))
		{
			size_t index = 0;
			for (auto& moduleName : m_ModuleOrder)
			{
				const bool is_selected = (current == &moduleName);
				if (ImGui::Selectable(moduleName.c_str(), is_selected))
				{
					current = &moduleName;
					m_SelectedSection = index;
				}

				if (is_selected)
					ImGui::SetItemDefaultFocus();
				index++;
			}
			ImGui::EndListBox();
		}

		ImGui::EndGroup();

		ImGui::SameLine();

		ImGui::BeginGroup();

		DrawProfileLine();

		ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
		ImGui::BeginChild("ChildR", ImVec2(0, 0), true, window_flags);

		ImGui::Text(current->c_str());
		ImGui::Separator();

		auto& sections = m_FeatureMap[*current];
		auto emptyName = std::string();
		if (sections.count(emptyName) > 0)
			DrawMenuSection(emptyName, sections[""]);

		for (auto& [sectionName, features] : sections)
		{
			if (sectionName.empty())
				continue;

			DrawMenuSection(sectionName, features);
		}

		ImGui::EndChild();
		ImGui::PopStyleVar();

		ImGui::EndGroup();

		ImGui::End();
	}

	void CheatManagerBase::DrawMenuSection(const std::string& sectionName, const std::vector<Feature*>& features) const
	{
		if (!sectionName.empty())
			BeginGroupPanel(sectionName.c_str(), ImVec2(-1, 0));

		for (auto& feature : features)
		{
			ImGui::PushID(&feature);
			feature->DrawMain();
			ImGui::PopID();
		}

		if (!sectionName.empty())
			EndGroupPanel();
	}

	void CheatManagerBase::DrawProfileGlobalActivities()
	{
		if (ImGui::Button("Add new profile"))
		{
			std::unordered_set<std::string> profileNameSet = { config::GetProfiles().begin(), config::GetProfiles().end() };
			size_t index = 0;
			std::string name {};
			do 
			{
				index++;
				std::string newName = fmt::format("Profile #{}", index);
				if (profileNameSet.count(newName) == 0)
					name = newName;

			} while (name.empty());

			config::CreateProfile(name, false);
		}
	}

	void CheatManagerBase::DrawProfileEntryActivities(const std::string& profileName)
	{
		bool isPopupOpen = ImGui::IsPopupOpen("RenamePopup");

		if (isPopupOpen)
			ImGui::BeginDisabled();

		static std::string nameBuffer;

		if (ImGui::Button("Rename"))
		{
			ImGui::OpenPopup("RenamePopup", ImGuiPopupFlags_NoOpenOverExistingPopup);
			if (ImGui::IsPopupOpen("RenamePopup"))
				nameBuffer = profileName;
		}

		if (isPopupOpen)
			ImGui::EndDisabled();

		if (ImGui::BeginPopup("RenamePopup", ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("To save press `Enter`.\nTo close without saving press `Esc`.");

			if (!ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0))
				ImGui::SetKeyboardFocusHere(0);

			ImGui::InputText("Name", &nameBuffer);

			if (ImGui::IsKeyPressed(ImGuiKey_Enter, false))
			{
				config::RenameProfile(profileName, nameBuffer);
				ImGui::CloseCurrentPopup();
			}

			if (ImGui::IsKeyPressed(ImGuiKey_Escape, false))
			{
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		ImGui::SameLine();

		if (ImGui::Button("Remove"))
		{
			config::RemoveProfile(profileName);
		}
	}

	void CheatManagerBase::DrawProfileEntry(const std::string& profileName)
	{
		ImGui::Text(profileName.c_str());
	}

	void CheatManagerBase::DrawProfileTableHeader()
	{
		ImGui::TableSetupColumn("Name");
	}

	int CheatManagerBase::GetProfileTableColumnCount()
	{
		return 1;
	}

	void CheatManagerBase::DrawProfileConfiguration()
	{
		ImGui::SetNextWindowSize({ 0, ImGui::GetTextLineHeightWithSpacing() * 11 }, ImGuiCond_FirstUseEver);
		if (!ImGui::Begin("Config profile configuration", &m_IsProfileConfigurationShowed))
		{
			ImGui::End();
			return;
		}

		static ImGuiTableFlags flags =
			ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable
			| ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_NoBordersInBody
			| ImGuiTableFlags_ScrollY;
		if (ImGui::BeginTable("ProfileTable", GetProfileTableColumnCount() + 1, flags,
			ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing() * 10), 0.0f))
		{
			DrawProfileTableHeader();
			ImGui::TableSetupColumn("Actions");
			ImGui::TableSetupScrollFreeze(0, 1);
			ImGui::TableHeadersRow();

			// Copy profiles names
			auto profiles = config::GetProfiles();
			for (auto& profile : profiles)
			{
				ImGui::TableNextRow();
				ImGui::TableNextColumn();

				ImGui::PushID(profile.c_str());
				DrawProfileEntry(profile);
				ImGui::TableNextColumn();

				DrawProfileEntryActivities(profile);
				ImGui::PopID();
			}

			ImGui::EndTable();
		}
		DrawProfileGlobalActivities();
		ImGui::End();
	}

	void CheatManagerBase::DrawProfileLine()
	{
		if (m_IsProfileConfigurationShowed)
			ImGui::BeginDisabled();

		bool buttonPressed = ImGui::Button("Configure...");

		if (m_IsProfileConfigurationShowed)
			ImGui::EndDisabled();

		if (buttonPressed)
			m_IsProfileConfigurationShowed = !m_IsProfileConfigurationShowed;

		ImGui::SameLine();

		auto& profiles = config::GetProfiles();
		auto& currentProfile = config::CurrentProfileName();

		constexpr float width = 200.0f;
		ImGui::SetNextItemWidth(width);
		if (ImGui::BeginCombo("Profile", currentProfile.c_str()))
		{
			for (auto& name : profiles)
			{
				bool is_selected = (currentProfile == name);
				if (ImGui::Selectable(name.c_str(), is_selected))
					config::ChangeProfile(name);

				if (ImGui::IsItemHovered() && CalcWidth(name) > width)
					ShowHelpText(name.c_str());

				if (is_selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
	}

	void CheatManagerBase::DrawStatus() const
	{
		// Drawing status window
		ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground |
			ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus |
			ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse;

		auto& settings = feature::Settings::GetInstance();
		if (!settings.f_StatusMove)
			flags |= ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoMove;

		ImGui::Begin("Cheat status", nullptr, flags);

		static ImGuiTableFlags tabFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg;

		if (ImGui::BeginTable("activesTable", 1, tabFlags))
		{
			ImGui::TableSetupColumn("Active features");
			ImGui::TableHeadersRow();

			int row = 0;

			for (auto& feature : m_Features)
			{
				if (feature->NeedStatusDraw())
				{
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);

					feature->DrawStatus();

					ImU32 row_bg_color = ImGui::GetColorU32(
						ImVec4(0.2f + row * 0.1f, 0.1f + row * 0.05f, 0.1f + row * 0.03f, 0.85f));
					ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, row_bg_color);
					row++;
				}
			}
			ImGui::EndTable();
		}

		ImGui::End();
	}

	void CheatManagerBase::DrawInfo()
	{
		auto& settings = feature::Settings::GetInstance();

		// Drawing status window
		ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration |
			ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus |
			ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse;

		if (!settings.f_InfoMove)
			flags |= ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoMove;

		auto checkLambda = [](const Feature* feat) { return feat->NeedInfoDraw(); };
		bool showAny = std::any_of(m_Features.begin(), m_Features.end(), checkLambda);
		if (!showAny && !settings.f_StatusMove)
			return;

		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.04f, 0.05f, 0.05f, 0.90f));
		ImGui::Begin("Info window", nullptr, flags);
		ImGui::PopStyleColor();

		if (!showAny)
		{
			ImGui::Text("Nothing here");
			ImGui::End();
			return;
		}

		for (auto& moduleName : m_ModuleOrder)
		{
			auto& sections = m_FeatureMap[moduleName];
			bool moduleShowAny = std::any_of(sections.begin(), sections.end(),
			                                 [](const auto& iter)
			                                 {
				                                 return std::any_of(iter.second.begin(), iter.second.end(),
				                                                    [](const auto feat)
				                                                    {
					                                                    return feat->NeedInfoDraw();
				                                                    });
			                                 }
			);
			if (!moduleShowAny)
				continue;

			BeginGroupPanel(moduleName.c_str(), ImVec2(-1, 0));

			for (auto& [sectionName, features] : sections)
			{
				for (auto& feature : features)
				{
					if (!feature->NeedInfoDraw())
						continue;

					ImGui::PushID(&feature);
					feature->DrawInfo();
					ImGui::PopID();
				}
			}

			EndGroupPanel();
		}

		ImGui::End();
	}

	void CheatManagerBase::DrawFps()
	{
		auto& settings = feature::Settings::GetInstance();

		ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoFocusOnAppearing
			| ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize;

		if (!settings.f_FpsMove)
			flags |= ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoMove;

		if (ImGui::Begin("FPS", nullptr, flags))
		{
			ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
			ImGui::End();
		}
	}

	void CheatManagerBase::DrawNotifications()
	{
		ImGui::RenderNotifications();
	}

	void CheatManagerBase::OnRender()
	{
		auto& settings = feature::Settings::GetInstance();

		DrawExternal();

		if (m_IsMenuShowed)
			DrawMenu();

		if (m_IsProfileConfigurationShowed)
			DrawProfileConfiguration();

		if (settings.f_StatusShow)
			DrawStatus();

		if (settings.f_InfoShow)
			DrawInfo();

		if (settings.f_FpsShow)
			DrawFps();

		if (settings.f_NotificationsShow)
			DrawNotifications();

		if (settings.f_MenuKey.value().IsReleased() && !ImGui::IsAnyItemActive())
			ToggleMenuShow();
	}

	void CheatManagerBase::CheckToggles(short key) const
	{
		if (m_IsMenuShowed || renderer::IsInputLocked())
			return;

		auto& settings = feature::Settings::GetInstance();
		if (!settings.f_HotkeysEnabled)
			return;

		for (auto& field : config::GetFields<config::ToggleHotkey>())
		{
			auto& value = field.value();
			if (value.hotkey.IsPressed(key))
			{
				value.enabled = !value.enabled;
				field.FireChanged();

				std::string title = fmt::format("{}: {}", field.friendName(), (value ? "Enabled" : "Disabled"));
				ImGuiToast toast(ImGuiToastType_None, settings.f_NotificationsDelay);
				toast.set_title(title.c_str());
				ImGui::InsertNotification(toast);
			}
		}
	}

	bool menuToggled = false;

	void CheatManagerBase::ToggleMenuShow()
	{
		m_IsMenuShowed = !m_IsMenuShowed;
		renderer::SetInputLock(this, m_IsMenuShowed && m_IsBlockingInput);
		menuToggled = true;
	}

	void CheatManagerBase::OnKeyUp(short key, bool& cancelled)
	{
		auto& settings = feature::Settings::GetInstance();
		if (!settings.f_MenuKey.value().IsPressed(key))
		{
			CheckToggles(key);
			return;
		}
	}

	void CheatManagerBase::OnWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool& canceled)
	{
		if (!menuToggled)
			return;

		menuToggled = false;

		if (m_IsMenuShowed)
		{
			m_IsPrevCursorActive = CursorGetVisibility();
			if (!m_IsPrevCursorActive)
				CursorSetVisibility(true);
		}
		else if (!m_IsPrevCursorActive)
			CursorSetVisibility(false);
	}

	bool CheatManagerBase::IsMenuShowed() const
	{
		return m_IsMenuShowed;
	}

	void CheatManagerBase::PushFeature(Feature* feature)
	{
		m_Features.push_back(feature);

		auto& info = feature->GetGUIInfo();
		if (m_FeatureMap.count(info.moduleName) == 0)
		{
			m_FeatureMap[info.moduleName] = {};
			m_ModuleOrder.push_back(info.moduleName);
		}

		auto& sectionMap = m_FeatureMap[info.moduleName];
		std::string sectionName = info.isGroup ? info.name : std::string();
		if (sectionMap.count(sectionName) == 0)
			sectionMap[sectionName] = {};

		auto& featureList = sectionMap[sectionName];
		featureList.push_back(feature);
	}

	void CheatManagerBase::AddFeature(Feature* feature)
	{
		PushFeature(feature);
	}

	void CheatManagerBase::AddFeatures(std::vector<Feature*> features)
	{
		for (auto& feature : features)
		{
			PushFeature(feature);
		}
	}

	void CheatManagerBase::SetModuleOrder(std::vector<std::string> moduleOrder)
	{
		std::unordered_set<std::string> moduleSet;
		moduleSet.insert(m_ModuleOrder.begin(), m_ModuleOrder.end());

		m_ModuleOrder.clear();

		for (auto& moduleName : moduleOrder)
		{
			if (m_FeatureMap.count(moduleName) == 0)
				continue;

			m_ModuleOrder.push_back(moduleName);
			moduleSet.erase(moduleName);
		}

		for (auto& moduleName : moduleSet)
		{
			m_ModuleOrder.push_back(moduleName);
		}
	}
}
