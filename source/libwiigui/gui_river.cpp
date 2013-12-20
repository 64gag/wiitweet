/****************************************************************************
 * libwiigui
 *
 * Pedro Aguiar 2012
 *
 * gui_river.cpp
 *
 * GuiRiver class definition
 ***************************************************************************/
#include "gui.h"

#define NOSCROLLMARGIN 85
GuiRiver::GuiRiver()
{
	yoffset = 45;
	width = screenwidth;
	height = 0;
	focus = 0; // allow focus
	totalHeight = 0;
	margin = 15;
}

GuiRiver::~GuiRiver()
{
}

void GuiRiver::Append(GuiElement* e)
{
	if (e == NULL)
		return;

	e->SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
	e->SetPosition(0, totalHeight);
	totalHeight += (e->GetHeight() + margin);

	Remove(e);
	_elements.push_back(e);
	e->SetParent(this);
}

void GuiRiver::Remove(GuiElement* e)
{
	if (e == NULL)
		return;

	u32 elemSize = _elements.size();
	for (u32 i = 0; i < elemSize; ++i)
	{
		if(e == _elements.at(i))
		{
			totalHeight -= (e->GetHeight() + margin);
			_elements.erase(_elements.begin()+i);
			break;
		}
	}
}

void GuiRiver::RemoveAll()
{
	totalHeight = 0;
	_elements.clear();
}

void GuiRiver::Update(GuiTrigger * t)
{
	if(_elements.size() == 0 || (state == STATE_DISABLED && parentElement) || !(parentElement->IsFocused()))
		return;

	if(t->wpad->btns_h & WPAD_BUTTON_DOWN){
		if(this->GetTop()+totalHeight+NOSCROLLMARGIN > screenheight)
		{
			this->SetPosition(xoffset, yoffset - 10);
		}
	}else if(t->wpad->btns_h & WPAD_BUTTON_UP){
		if(this->GetTop()-NOSCROLLMARGIN < 0)
		{
			this->SetPosition(xoffset, yoffset + 10);
		}
	}

	int update = 1;
	if(this->IsClickable() && t->wpad->ir.valid && t->wpad->ir.y <= 90 ) update = 0; //Toolbar is on top (set clickable to true if you want this check to take effect)

	u32 elemSize = _elements.size();
	u32 i;
	for (i = 0; i < elemSize; ++i)
	{
		try	{
				if(_elements.at(i)->GetTop()+_elements.at(i)->GetHeight() < 0){ //All these are off-screen (above viewable area)
					_elements.at(i)->SetVisible(0); //Do not update them (instad of disabling them) and make them invisible so they don't waste draw() time
				}else{								//First viewable element!
					if(update) { _elements.at(i)->Update(t); }
					_elements.at(i)->SetVisible(1);
					break;
				}
			}
		catch (const std::exception& e) { }
	}

	for (; i < elemSize; ++i)
	{
		try	{
				if(_elements.at(i)->GetTop() < screenheight){			//All these are viewable
					if(update) { _elements.at(i)->Update(t); }
					_elements.at(i)->SetVisible(1);
				}else{								//This is not
					_elements.at(i)->SetVisible(0);
					break;
				}
			}
		catch (const std::exception& e) { }
	}

	for (; i < elemSize; ++i)								//None of these are
	{
		try	{
				_elements.at(i)->SetVisible(0);
			}
		catch (const std::exception& e) { }
	}

/*
	this->ToggleFocus(t);

	if(focus) // only send actions to this window if it's in focus
	{
		// pad/joystick navigation
		if(t->Right())
			this->MoveSelectionHor(1);
		else if(t->Left())
			this->MoveSelectionHor(-1);
		else if(t->Down())
			this->MoveSelectionVert(1);
		else if(t->Up())
			this->MoveSelectionVert(-1);
	}
*/
	if(updateCB)
		updateCB(this);
}
