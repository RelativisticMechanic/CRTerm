#ifndef CONFIG_LOADER_H
#define CONFIG_LOADER_H

#include "CRTermUI.h"
/*
	Allows the user to select a JSON configuration from the JSON files
	in config/.

	It just reads the config directory then overwrites the "default" file
	with the path of whatever the user selected. 
*/

#include "CRTermConfig.h"
#include <vector>
#include <fstream>
#include <filesystem>

class ConfigSelector : public UIElement
{
public:
	std::vector<std::string> config_files;
	int chosen_idx;
	ConfigSelector();
	void Render() override;
};

#endif 