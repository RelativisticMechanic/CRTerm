#ifndef CONFIGEDITOR_H
#define CONFIGEDITOR_H

#include "CRTermUI.h"
#include "CRTermConfig.h"
#include "imgui_stdlib.h"

class ConfigEditor : public UIElement
{
public:
    CRTermConfiguration* cfg;
	void Render(void) override;
};
#endif