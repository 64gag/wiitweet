/****************************************************************************
 * libwiigui
 *
 * Pedro Aguiar 2012
 *
 * gui_infomsg.cpp
 *
 * GuiInfomsg class definition
 ***************************************************************************/

#include "gui.h"

static GuiImageData *InfoImgData = NULL;

GuiInfomsg::GuiInfomsg()
{
	if(InfoImgData == NULL)
	{
		InfoImgData = new GuiImageData(infomessage_png);
	}

	width = screenwidth;
	height = 90;
	yoffset = screenheight*0.8;
	xoffset = 0;
	focus = 0; // allow focus
	alpha = 0;
	animate = 0;
	time(&timer);

	Message = new GuiText;
	Message->SetPosition(0, 0);
	Message->SetAlignment(ALIGN_CENTRE, ALIGN_MIDDLE);

	InfoImg.SetImage(InfoImgData);
	InfoImg.SetAlignment(ALIGN_CENTRE, ALIGN_MIDDLE);
	InfoImg.SetPosition(0, 0);

	Append(&InfoImg);
	Append(Message);
}

GuiInfomsg::~GuiInfomsg()
{
	delete Message;
}

void GuiInfomsg::Display(const char * msg){
	time(&timer);
	SetAlpha(255);
	Message->SetText(msg);
}

void GuiInfomsg::Update(GuiTrigger * t)
{
	if(GetAlpha() == 0){
		return;
	}

	if(difftime(time(NULL), timer) >= 2.0f){
		SetAlpha(GetAlpha()-10);
		if(GetAlpha() < 0){ SetAlpha(0); }
	}

}
