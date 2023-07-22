#include "ConfigSelector.h"

bool VectorOfStringGetter(void* data, int n, const char** out_text)
{
	const std::vector<std::string>* v = (std::vector<std::string>*)data;
	*out_text = (*v)[n].c_str();
	return true;
}

ConfigSelector::ConfigSelector() : UIElement()
{
	config_files = std::vector<std::string>();
	/* Read the config directory */
	std::string path = "config";
	for (const auto& entry : std::filesystem::directory_iterator(path))
		config_files.push_back(entry.path().string());
}

void ConfigSelector::Render()
{
	ImGui::Begin("Select Configuration", 0, ImGuiWindowFlags_AlwaysAutoResize);

	ImGui::Text("Select Configuration");
	ImGui::SameLine();
	ImGui::ListBox("##cfg_selector", &chosen_idx, VectorOfStringGetter, (void*)&config_files, (int)config_files.size());

	if (ImGui::Button("Load"))
	{
		std::string chosen_file = config_files[chosen_idx];
		std::ofstream default_file("default", std::ofstream::out | std::ofstream::trunc);
		default_file << chosen_file;
		default_file.close();
	}

	if (ImGui::Button("Close"))
	{
		this->show = false;
	}

	ImGui::Text("Note: You must restart for changes to take effect.");
	ImGui::End();
}