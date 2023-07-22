#include "imgui_internal.h"

#include "ContextMenu.h"

ContextMenu::ContextMenu(ContextMenuCallback cb, void* data)
{
	this->element_names = std::vector<std::string>();
	this->callback = cb;
	this->callback_data = data;
	this->menu_toggle = false;
}

void ContextMenu::Render()
{
	if (ImGui::GetIO().MouseClicked[1])
		this->menu_toggle = !this->menu_toggle;

	if (!this->menu_toggle)
		return;

	ImGui::OpenPopup("Context Menu##menu");
	if (ImGui::BeginPopup("Context Menu##menu"))
	{
		for (int i = 0; i < this->element_names.size(); i++)
		{
			std::string button_label = this->element_names[i] + "##" + std::to_string(i);
			if (ImGui::Button(button_label.c_str()))
			{
				if (this->callback != NULL)
				{
					this->callback(i, this->callback_data);
					this->menu_toggle = false;
				}
			}
		}
		ImGui::EndPopup();
	}
}

void ContextMenu::Add(std::string s)
{
	this->element_names.push_back(s);
}

