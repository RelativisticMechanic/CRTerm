#ifndef CONFIG_LOADER_H
#define CONFIG_LOADER_H

#include "CRTermUI.h"
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