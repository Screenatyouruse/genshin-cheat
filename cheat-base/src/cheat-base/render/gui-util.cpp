#include <pch.h>
#include "gui-util.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <cheat-base/util.h>
#include <shellapi.h>


void ShowHelpText(const char* text)
{
	ImGui::BeginTooltip();
	ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
	ImGui::TextUnformatted(text);
	ImGui::PopTextWrapPos();
	ImGui::EndTooltip();
}

void HelpMarker(const char* desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
        ShowHelpText(desc);
}

bool InputHotkey(const char* label, Hotkey* hotkey, bool clearable)
{
    char hotkeyBuffer[50];

    auto hotkeyString = std::string(*hotkey);
    memcpy(hotkeyBuffer, hotkeyString.c_str(), hotkeyString.size() + 1);

    bool changed = false;

    if (clearable) {
        char labelBuffer[128];
        std::snprintf(labelBuffer, 128, "Clear ## %s_1", label);

        if (ImGui::Button(labelBuffer, ImVec2(75, 0)))
        {
            *hotkey = Hotkey();
            changed = true;
        }
        ImGui::SameLine();
    }

    changed = ImGui::HotkeyWidget(label, *hotkey, ImVec2(200, 0)) || changed;

    return changed;
}

float CalcWidth(const std::string_view& view)
{
	ImGuiContext& g = *GImGui;
	return ImGui::CalcTextSize(view.data()).x + g.Style.FramePadding.x * 2.0f + 25.0f;
}


#define END_CONFIG_WIDGET() if (result) field.FireChanged(); \
    if (desc != nullptr) { ImGui::SameLine(); HelpMarker(desc); } \
    return result;

bool ConfigWidget(const char* label, config::Field<bool>& field, const char* desc)
{
    bool result = ImGui::Checkbox(label, field);
    END_CONFIG_WIDGET();
}

bool ConfigWidget(const char* label, config::Field<int>& field, int step, int start, int end, const char* desc)
{
    bool result = false;

    if (start == end)
        result = ImGui::InputInt(label, field, step);
    else
        result = ImGui::DragInt(label, field, (float)step, start, end);

    END_CONFIG_WIDGET();
}

bool ConfigWidget(const char* label, config::Field<float>& field, float step, float start, float end, const char* desc)
{
    bool result = false;

    if (start == end)
        result = ImGui::InputFloat(label, field, step);
    else
        result = ImGui::DragFloat(label, field, step, start, end);

    END_CONFIG_WIDGET();
}

bool ConfigWidget(const char* label, config::Field<Hotkey>& field, bool clearable, const char* desc)
{
    bool result = InputHotkey(label, field, clearable);
    END_CONFIG_WIDGET();
}

bool ConfigWidget(const char* label, config::Field<std::string>& field, const char* desc)
{
    bool result = ImGui::InputText(label, field);
    END_CONFIG_WIDGET();
}

bool ConfigWidget(const char* label, config::Field<ImColor>& field, const char* desc /*= nullptr*/)
{
    bool result = ImGui::ColorEdit4(label, reinterpret_cast<float*>(field.pointer()));
    END_CONFIG_WIDGET();
}

bool ConfigWidget(const char* label, config::Field<config::ToggleHotkey>& field, const char* desc /*= nullptr*/, bool hotkey /*= false*/)
{
    bool result = hotkey ? InputHotkey(label, &field.value().hotkey, true) : ImGui::Checkbox(label, &field.value().enabled);
    END_CONFIG_WIDGET();
}

bool ConfigWidget(config::Field<bool>& field, const char* desc)
{
    return ConfigWidget(field.friendName().c_str(), field, desc);
}

bool ConfigWidget(config::Field<int>& field, int step, int start, int end, const char* desc)
{
    return ConfigWidget(field.friendName().c_str(), field, step, start, end, desc);
}

bool ConfigWidget(config::Field<float>& field, float step, float start, float end, const char* desc)
{
    return ConfigWidget(field.friendName().c_str(), field, step, start, end, desc);
}

bool ConfigWidget(config::Field<Hotkey>& field, bool clearable, const char* desc)
{
    return ConfigWidget(field.friendName().c_str(), field, clearable, desc);
}

bool ConfigWidget(config::Field<std::string>& field, const char* desc)
{
    return ConfigWidget(field.friendName().c_str(), field, desc);
}

bool ConfigWidget(config::Field<ImColor>& field, const char* desc /*= nullptr*/)
{
    return ConfigWidget(field.friendName().c_str(), field, desc);
}

bool ConfigWidget(config::Field<config::ToggleHotkey>& field, const char* desc /*= nullptr*/, bool hotkey /*= false*/)
{
    return ConfigWidget(field.friendName().c_str(), field, desc, hotkey);
}

#undef ShowDesc

// https://github.com/ocornut/imgui/issues/1496#issuecomment-655048353

struct GroupPanelInfo
{
    ImRect labelRect;
    ImRect selectRect;    
};
static ImVector<GroupPanelInfo> s_GroupPanelLabelStack;

bool GroupPanelIsOpen(ImGuiID id)
{
	ImGuiContext& g = *GImGui;
	ImGuiWindow* window = g.CurrentWindow;
    ImGuiStorage* storage = window->DC.StateStorage;

    return storage->GetInt(id, 1) != 0;
}

void GroupPanelSetOpen(ImGuiID id, bool open)
{
	ImGuiContext& g = *GImGui;
	ImGuiWindow* window = g.CurrentWindow;
	ImGuiStorage* storage = window->DC.StateStorage;

    storage->SetInt(id, open ? 1 : 0);
}

bool BeginGroupPanel(const char* name, const ImVec2& size, bool node, SelectData* selectData)
{
	ImGuiContext& g = *GImGui;
	ImGuiWindow* window = g.CurrentWindow;

    ImGui::PushID(name);
	ImGui::BeginGroup();
	auto cursorPos = ImGui::GetCursorScreenPos();

    auto itemSpacing = ImGui::GetStyle().ItemSpacing;
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

    auto frameHeight = ImGui::GetFrameHeight();
    ImGui::BeginGroup();

    ImVec2 effectiveSize = size;
    if (size.x < 0.0f)
        effectiveSize.x = ImGui::GetContentRegionAvail().x;
    else
        effectiveSize.x = size.x;
    ImGui::Dummy(ImVec2(effectiveSize.x, 0.0f));

    ImVec2 startPos = window->DC.CursorPos;
    ImGui::Dummy(ImVec2(frameHeight * 0.5f, 0.0f));
    ImGui::SameLine(0.0f, 0.0f);
    ImGui::BeginGroup();
    ImGui::Dummy(ImVec2(frameHeight * 0.5f, 0.0f));
    ImGui::SameLine(0.0f, 0.0f);
    ImGui::TextUnformatted(name);

    auto labelMin = ImGui::GetItemRectMin();
    auto labelMax = ImGui::GetItemRectMax();
    ImGui::SameLine(0.0f, 0.0f);

    ImVec2 selectMin = {};
    ImVec2 selectMax = {};
    if (selectData != nullptr)
    {
		bool useText = true;
		const char* selectAll = "Select all";
		auto textSize = ImGui::CalcTextSize(selectAll);
		auto spaceSize = ImVec2(effectiveSize.x - textSize.x - 35.0f - labelMax.x + startPos.x, 0.0f);
		if (spaceSize.x <= 0)
		{
			spaceSize = ImVec2(effectiveSize.x - 35.0f - labelMax.x + startPos.x, 0.0f);
			useText = false;
		}
		ImGui::Dummy(spaceSize);
		ImGui::SameLine(0.0f, 0.0f);

        selectData->changed = ImGui::Checkbox(useText ? selectAll : "", &selectData->toggle);
        
        selectMin = ImGui::GetItemRectMin();
		selectMax = ImGui::GetItemRectMax();
    }

    ImGui::SameLine(0.0f, 0.0f);
    ImGui::Dummy(ImVec2(0.0, frameHeight + itemSpacing.y));
    
	if (node)
	{
		labelMin.x = startPos.x;

		const ImVec2 text_size = ImGui::CalcTextSize(name);
		const ImGuiID id = window->GetID(name);

		bool isOpen = GroupPanelIsOpen(id);

		bool hovered;
		bool toggled = ImGui::ButtonBehavior({ labelMin, labelMax }, id, &hovered, nullptr, ImGuiButtonFlags_PressedOnClick);
		if (toggled)
		{
			isOpen = !isOpen;
			GroupPanelSetOpen(id, isOpen);
		}

		const ImU32 text_col = ImGui::GetColorU32(ImGuiCol_Text);
		ImGui::RenderArrow(window->DrawList, { cursorPos.x, cursorPos.y + text_size.y * 0.15f }, text_col,
			isOpen ? ImGuiDir_Down : ImGuiDir_Right, 0.7f);

        if (!isOpen)
        {
            ImGui::PopStyleVar(2);
            ImGui::EndGroup();
            ImGui::EndGroup();
            ImGui::EndGroup();
            ImGui::PopID();
            return false;
        }
	}

    ImGui::BeginGroup();

    //ImGui::GetWindowDrawList()->AddRect(labelMin, labelMax, IM_COL32(255, 0, 255, 255));

    ImGui::PopStyleVar(2);

#if IMGUI_VERSION_NUM >= 17301
    ImGui::GetCurrentWindow()->ContentRegionRect.Max.x -= frameHeight * 0.5f;
    ImGui::GetCurrentWindow()->WorkRect.Max.x -= frameHeight * 0.5f;
    ImGui::GetCurrentWindow()->InnerRect.Max.x -= frameHeight * 0.5f;
#else
    ImGui::GetCurrentWindow()->ContentsRegionRect.Max.x -= frameHeight * 0.5f;
#endif
    ImGui::GetCurrentWindow()->Size.x -= frameHeight;

    auto itemWidth = ImGui::CalcItemWidth();
    ImGui::PushItemWidth(ImMax(0.0f, itemWidth - frameHeight));

    s_GroupPanelLabelStack.push_back({ ImRect(labelMin, labelMax) , ImRect(selectMin, selectMax)});

    return true;
}

void EndGroupPanel()
{
    ImGui::PopItemWidth();

    auto itemSpacing = ImGui::GetStyle().ItemSpacing;

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

    auto frameHeight = ImGui::GetFrameHeight();

    ImGui::EndGroup();

    //ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), IM_COL32(0, 255, 0, 64), 4.0f);

    ImGui::EndGroup();

    ImGui::SameLine(0.0f, 0.0f);
    ImGui::Dummy(ImVec2(frameHeight * 0.5f, 0.0f));
    ImGui::Dummy(ImVec2(0.0, frameHeight - frameHeight * 0.5f - itemSpacing.y));

    ImGui::EndGroup();

    auto itemMin = ImGui::GetItemRectMin();
    auto itemMax = ImGui::GetItemRectMax();
    //ImGui::GetWindowDrawList()->AddRectFilled(itemMin, itemMax, IM_COL32(255, 0, 0, 64), 4.0f);

    auto& info = s_GroupPanelLabelStack.back();
    s_GroupPanelLabelStack.pop_back();

    ImVec2 halfFrame = ImVec2(frameHeight * 0.25f, frameHeight) * 0.5f;
    ImRect frameRect = ImRect(itemMin + halfFrame, itemMax - ImVec2(halfFrame.x, 0.0f));

    auto& labelRect = info.labelRect;
    labelRect.Min.x -= itemSpacing.x;
    labelRect.Max.x += itemSpacing.x;

    bool hasSelect = info.selectRect.Min.x != 0;

    if (!hasSelect)
    {
		for (int i = 0; i < 3; ++i)
		{
			switch (i)
			{
				// left half-plane
			case 0: ImGui::PushClipRect(ImVec2(-FLT_MAX, -FLT_MAX), ImVec2(labelRect.Min.x, FLT_MAX), true); break;
				// right half-plane
			case 1: ImGui::PushClipRect(ImVec2(labelRect.Max.x, -FLT_MAX), ImVec2(FLT_MAX, FLT_MAX), true); break;
				// bottom
			case 2: ImGui::PushClipRect(ImVec2(labelRect.Min.x, labelRect.Max.y), ImVec2(labelRect.Max.x, FLT_MAX), true); break;
			}

			ImGui::GetWindowDrawList()->AddRect(
				frameRect.Min, frameRect.Max,
				ImColor(ImGui::GetStyleColorVec4(ImGuiCol_Border)),
				halfFrame.x);

			ImGui::PopClipRect();
		}
    } 
    else
    {
        auto& selectRect = info.selectRect;
        selectRect.Min.x -= itemSpacing.x;
        selectRect.Max.x += itemSpacing.x;
		for (int i = 0; i < 5; ++i)
		{
			switch (i)
			{
				// left half-plane
			case 0: ImGui::PushClipRect(ImVec2(-FLT_MAX, -FLT_MAX), ImVec2(labelRect.Min.x, FLT_MAX), true); break;
				// label - select
			case 1: ImGui::PushClipRect(ImVec2(labelRect.Max.x, -FLT_MAX), ImVec2(selectRect.Min.x, FLT_MAX), true); break;
				// bottom label
			case 2: ImGui::PushClipRect(ImVec2(labelRect.Min.x, labelRect.Max.y), ImVec2(labelRect.Max.x, FLT_MAX), true); break;
				// bottom select
			case 3: ImGui::PushClipRect(ImVec2(selectRect.Min.x, selectRect.Max.y), ImVec2(selectRect.Max.x, FLT_MAX), true); break;
				// right hand-plane
			case 4: ImGui::PushClipRect(ImVec2(selectRect.Max.x, -FLT_MAX), ImVec2(FLT_MAX, FLT_MAX), true); break;
			}

			ImGui::GetWindowDrawList()->AddRect(
				frameRect.Min, frameRect.Max,
				ImColor(ImGui::GetStyleColorVec4(ImGuiCol_Border)),
				halfFrame.x);

			ImGui::PopClipRect();
		}
    }


    ImGui::PopStyleVar(2);

#if IMGUI_VERSION_NUM >= 17301
    ImGui::GetCurrentWindow()->ContentRegionRect.Max.x += frameHeight * 0.5f;
    ImGui::GetCurrentWindow()->WorkRect.Max.x += frameHeight * 0.5f;
    ImGui::GetCurrentWindow()->InnerRect.Max.x += frameHeight * 0.5f;
#else
    ImGui::GetCurrentWindow()->ContentsRegionRect.Max.x += frameHeight * 0.5f;
#endif
    ImGui::GetCurrentWindow()->Size.x += frameHeight;

    ImGui::Dummy(ImVec2(0.0f, 0.0f));

    ImGui::EndGroup();
    ImGui::PopID();
}

void AddUnderLine(ImColor col_)
{
	ImVec2 min = ImGui::GetItemRectMin();
	ImVec2 max = ImGui::GetItemRectMax();
	min.y = max.y;
	ImGui::GetWindowDrawList()->AddLine(min, max, col_, 1.0f);
}

void TextURL(const char* name_, const char* URL_, bool SameLineBefore_, bool SameLineAfter_)
{
	if (SameLineBefore_) { ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x); }
	ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered]);
	ImGui::Text(name_);
	ImGui::PopStyleColor();
	if (ImGui::IsItemHovered())
	{
		if (ImGui::IsMouseClicked(0))
		{
			ShellExecute(0, 0, URL_, 0, 0, SW_SHOW);
		}
		AddUnderLine(ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered]);
		ImGui::SetTooltip("Open in browser\n%s", URL_);
	}
	else
	{
		AddUnderLine(ImGui::GetStyle().Colors[ImGuiCol_Button]);
	}
	if (SameLineAfter_) { ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x); }
}

// Modified version of: https://github.com/spirthack/CSGOSimple/blob/master/CSGOSimple/UI.cpp#L287 
bool ImGui::HotkeyWidget(const char* label, Hotkey& hotkey, const ImVec2& size)
{
    // Init ImGui
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	ImGuiIO& io = g.IO;
	const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);

    const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);
    const ImVec2 item_size = ImGui::CalcItemSize(size, ImGui::CalcItemWidth(), label_size.y + style.FramePadding.y * 2.0f);

    const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + item_size);
	const ImRect total_bb(window->DC.CursorPos, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));

	ImGui::ItemSize(total_bb, style.FramePadding.y);
	if (!ImGui::ItemAdd(total_bb, id))
		return false;


    const bool focus_requested = (ImGui::GetItemStatusFlags() & ImGuiItemStatusFlags_FocusedByTabbing) != 0 || g.NavActivateInputId == id;
    const bool hovered = ImGui::ItemHoverable(frame_bb, id);
	if (hovered) 
    {
		ImGui::SetHoveredID(id);
		g.MouseCursor = ImGuiMouseCursor_TextInput;
	}

	static Hotkey _initHotkey;
	static Hotkey _currHotkey;
    static Hotkey _prevHotkey;

	const bool user_clicked = hovered && io.MouseClicked[0];
	if (focus_requested || user_clicked) 
    {
		if (g.ActiveId != id) 
        {
			memset(io.MouseDown, 0, sizeof(io.MouseDown));
			memset(io.KeysDown, 0, sizeof(io.KeysDown));

            _initHotkey = hotkey;
            _currHotkey = Hotkey();
            _prevHotkey = Hotkey();
		}

		ImGui::SetActiveID(id, window);
		ImGui::FocusWindow(window);
	}
	else if (io.MouseClicked[0] && g.ActiveId == id)
    {
		ImGui::ClearActiveID();
	}

    bool valueChanged = false;

	if (g.ActiveId == id)
	{
		if (ImGui::IsKeyPressed(ImGuiKey_Escape))
		{
			ImGui::ClearActiveID();
            if (hotkey != _initHotkey)
            {
                hotkey = _initHotkey;
                valueChanged = true;
            }
		}
        else
        {
			ImGui::NavMoveRequestCancel();

			auto newHotkey = Hotkey::GetPressedHotkey();

            if (newHotkey.IsEmpty() && !_currHotkey.IsEmpty())
            {
                ImGui::ClearActiveID();
                valueChanged = false;
            } 
            else if (newHotkey - _prevHotkey)
			{
                _currHotkey = newHotkey;
                hotkey = newHotkey;
                valueChanged = true;
			}

            _prevHotkey = newHotkey;
        }

    }

	// Render
	// Select which buffer we are going to display. When ImGuiInputTextFlags_NoLiveEdit is Set 'buf' might still be the old value. We Set buf to NULL to prevent accidental usage from now on.

	char buf_display[128] = "Empty";

    const ImU32 frame_col = ImGui::GetColorU32(g.ActiveId == id ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
	ImGui::RenderFrame(frame_bb.Min, frame_bb.Max, frame_col, true, style.FrameRounding);

	if ((g.ActiveId == id && !_currHotkey.IsEmpty()) || g.ActiveId != id)
		strcpy_s(buf_display, static_cast<std::string>(hotkey).c_str());
	else if (g.ActiveId == id) 
		strcpy_s(buf_display, "<Press a key>");

	const ImRect clip_rect(frame_bb.Min.x, frame_bb.Min.y, frame_bb.Min.x + item_size.x, frame_bb.Min.y + item_size.y); // Not using frame_bb.Max because we have adjusted size
	ImVec2 render_pos = frame_bb.Min + style.FramePadding;
	ImGui::RenderTextClipped(frame_bb.Min + style.FramePadding, frame_bb.Max - style.FramePadding, buf_display, NULL, NULL, style.ButtonTextAlign, &clip_rect);
	//RenderTextClipped(frame_bb.Min + style.FramePadding, frame_bb.Max - style.FramePadding, buf_display, NULL, NULL, GetColorU32(ImGuiCol_Text), style.ButtonTextAlign, &clip_rect);
	//draw_window->DrawList->AddText(g.Font, g.FontSize, render_pos, GetColorU32(ImGuiCol_Text), buf_display, NULL, 0.0f, &clip_rect);
    if (label_size.x > 0)
		ImGui::RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y), label);

	return valueChanged;
}

// https://github.com/ocornut/imgui/issues/3798
float CalcContrastRatio(const ImU32& backgroundColor, const ImU32& foreGroundColor)
{
    // real code https://www.w3.org/TR/WCAG20/#relativeluminancedef
    /*const auto colBG = ImGui::ColorConvertU32ToFloat4(backgroundColor);
    const auto colFG = ImGui::ColorConvertU32ToFloat4(foreGroundColor);
    float lumBG = 0.2126 * colBG.x + 0.7152 * colBG.y + 0.0722 * colBG.z;
    float lumFG = 0.2126 * colFG.x + 0.7152 * colFG.y + 0.0722 * colFG.z;
    return (ImMax(lumBG, lumFG) + 0.05) / (ImMin(lumBG, lumFG) + 0.05);*/

    float sa0 = ((backgroundColor >> IM_COL32_A_SHIFT) & 0xFF);
    float sa1 = ((foreGroundColor >> IM_COL32_A_SHIFT) & 0xFF);
    static float sr = 0.2126f / 255.0f;
    static float sg = 0.7152f / 255.0f;
    static float sb = 0.0722f / 255.0f;
    const float contrastRatio =
        (sr * sa0 * ((backgroundColor >> IM_COL32_R_SHIFT) & 0xFF) +
            sg * sa0 * ((backgroundColor >> IM_COL32_G_SHIFT) & 0xFF) +
            sb * sa0 * ((backgroundColor >> IM_COL32_B_SHIFT) & 0xFF) + 0.05f) /
        (sr * sa1 * ((foreGroundColor >> IM_COL32_R_SHIFT) & 0xFF) +
            sg * sa1 * ((foreGroundColor >> IM_COL32_G_SHIFT) & 0xFF) +
            sb * sa1 * ((foreGroundColor >> IM_COL32_B_SHIFT) & 0xFF) + 0.05f);
    if (contrastRatio < 1.0f)
        return 1.0f / contrastRatio;
    return contrastRatio;
}

bool ImGui::PushStyleColorWithContrast(ImU32 backGroundColor, ImGuiCol foreGroundColor, ImU32 invertedColor, float maxContrastRatio)
{
	const float contrastRatio = CalcContrastRatio(backGroundColor, GetColorU32(foreGroundColor));
	if (contrastRatio < maxContrastRatio)
	{
		ImGui::PushStyleColor(foreGroundColor, invertedColor);
		return true;
	}
	return false;
}