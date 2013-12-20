/****************************************************************************
 * libwiigui
 *
 * Pedro Aguiar 2012
 *
 * gui_toolbar.cpp
 *
 * GuiToolbar class definition
 ***************************************************************************/

#include "gui.h"

GuiToolbar::GuiToolbar()
{
	width = screenwidth;
	height = 90;
	yoffset = -height;
	xoffset = 0;
	focus = 0; // allow focus
	alpha = 255;
	animate = 0;
	time(&timer);
	ToolbarImgData = new GuiImageData(toolbar_png);
	ToolbarImg = new GuiImage(ToolbarImgData);
	ToolbarImg->SetTile((float)(screenwidth/4+1));
	ToolbarImg->SetPosition(0,0);
	ToolbarImg->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	this->Append(ToolbarImg);
}

GuiToolbar::~GuiToolbar()
{
	delete ToolbarImgData;
	delete ToolbarImg;
}

#define SLIDESPEED 15
void GuiToolbar::Update(GuiTrigger * t)
{
	if(_elements.size() == 0 || (state == STATE_DISABLED && parentElement) || !(parentElement->IsFocused()))
		return;

	if((t->wpad->ir.valid && t->wpad->ir.y < height) || animate || t->wpad->btns_d & WPAD_BUTTON_HOME){
		animate = 1;
		if(GetTop() < 0){
			SetPosition(xoffset, yoffset+SLIDESPEED);
		}else{
			animate = 0;
			time(&timer);
		}
	}else if(GetTop()+height > 0 && difftime(time(NULL), timer) >= 1.2f){
		SetPosition(xoffset, yoffset-SLIDESPEED);
	}

	u32 elemSize = _elements.size();
	for (u32 i = 0; i < elemSize; ++i)
	{
		try	{ _elements.at(i)->Update(t); }
		catch (const std::exception& e) { }
	}

	if(updateCB)
		updateCB(this);
}
