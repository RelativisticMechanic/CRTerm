#ifndef CONTEXTMENU_H
#define CONTEXTMENU_H

#include "CRTermUI.h"

typedef void (*ContextMenuCallback)(int, void*);

class ContextMenu : public UIElement
{
public:
	std::vector<std::string> element_names;
	void* callback_data;
	ContextMenuCallback callback;
	bool menu_toggle;

	ContextMenu(ContextMenuCallback, void*);
	void Render() override;
	void Add(std::string item);
};

#endif