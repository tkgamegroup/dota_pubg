#include "view_settings.h"

ViewSettings view_settings;

ViewSettings::ViewSettings() :
	GuiView("Settings")
{
}

bool ViewSettings::on_begin()
{
	bool open = true;
	ImGui::PushStyleColor(ImGuiCol_WindowBg, (ImU32)ImColor(40, 40, 40));
	ImGui::Begin(name.c_str(), &open, ImGuiWindowFlags_NoCollapse);
	ImGui::PopStyleColor();
	return !open;
}

void ViewSettings::on_draw()
{

}
