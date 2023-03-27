#include "pch.h"
#include "PlatformDisplay.h"


std::string PlatformDisplay::GetPluginName() {
	return "PlatformDisplay";
}

void PlatformDisplay::SetImGuiContext(uintptr_t ctx) {
	ImGui::SetCurrentContext(reinterpret_cast<ImGuiContext*>(ctx));
}

// Render the plugin settings here
// This will show up in bakkesmod when the plugin is loaded at
//  f2 -> plugins -> PlatformDisplay
void PlatformDisplay::RenderSettings() {
	ImGui::TextUnformatted("PlatformDisplay plugin settings");

	CVarWrapper colorpickerblue = cvarManager->getCvar("PlatformDisplay_ColorPickerBlueTeam");
	if (!colorpickerblue) { return; }
	LinearColor textColorBlue = colorpickerblue.getColorValue() / 255;

	CVarWrapper colorpickerorange = cvarManager->getCvar("PlatformDisplay_ColorPickerOrangeTeam");
	if (!colorpickerorange) { return; }
	LinearColor textColorOrange = colorpickerorange.getColorValue() / 255;

	CVarWrapper overrideTintCvar = cvarManager->getCvar("PlatformDisplay_OverrideTints");
	if (!overrideTintCvar) { return; }
	bool doOverride = overrideTintCvar.getBoolValue();

	if (ImGui::Checkbox("Override auto tinting of logos (set to white for no tinting", &doOverride)) {
		overrideTintCvar.setValue(doOverride);
	}
	if (doOverride) {
		if (ImGui::ColorEdit4("Blue Color", &textColorBlue.R)) {
			colorpickerblue.setValue(textColorBlue * 255);
		}
		if (ImGui::ColorEdit4("Orange Color", &textColorOrange.R)) {
			colorpickerorange.setValue(textColorOrange * 255);
		}
	}
}



// Do ImGui rendering here
void PlatformDisplay::Render()
{
	if (!ImGui::Begin(menuTitle_.c_str(), &isWindowOpen_, ImGuiWindowFlags_None))
	{
		// Early out if the window is collapsed, as an optimization.
		ImGui::End();
		return;
	}

	ImGui::End();

	if (!isWindowOpen_)
	{
		cvarManager->executeCommand("togglemenu " + GetMenuName());
	}
}

// Name of the menu that is used to toggle the window.
std::string PlatformDisplay::GetMenuName()
{
	return "PlatformDisplay";
}

// Title to give the menu
std::string PlatformDisplay::GetMenuTitle()
{
	return menuTitle_;
}
// Should events such as mouse clicks/key inputs be blocked so they won't reach the game
bool PlatformDisplay::ShouldBlockInput()
{
	return ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard;
}

// Return true if window should be interactive
bool PlatformDisplay::IsActiveOverlay()
{
	return true;
}

// Called when window is opened
void PlatformDisplay::OnOpen()
{
	isWindowOpen_ = true;
}

// Called when window is closed
void PlatformDisplay::OnClose()
{
	isWindowOpen_ = false;
}

