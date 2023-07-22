/*
	Allows the user to create their own JSON configuration and save it.
*/
#ifndef CONFIGEDITOR_H
#define CONFIGEDITOR_H

#include "CRTermUI.h"
#include "CRTermConfig.h"
#include "imgui_stdlib.h"

class ConfigEditor : public UIElement
{
public:
	/* TODO: Create a copy consturctor for cfg maybe? */
    CRTermConfiguration* cfg;
	/* 
		Showing 15 color pickers is too much, so the user
		will have to first select a colour from the integer
		selection, and then modify it.
	*/
	int selected_color = 0;
	void Render(void) override;
};
#endif