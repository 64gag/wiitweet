/****************************************************************************
 * libwiigui
 *
 * Tantric 2009-2012
 *
 * gui_keyboard.cpp
 *
 * GUI class definitions
 *
 * Modified by Pedro Aguiar
 ***************************************************************************/

#include "gui.h"
#include "../fileop.h"
#include "../wiitweet.h"

Keyboard * KeyboardMaps = NULL;
int KeyboardMapsNum = 0;

/*
If you see a lot of zeros it is because the same format is used to store 4-bytes-long unicode characters (which would need
4 bytes plus the null terminating character). The format could be designed to be smaller but that would require more processing.
*/

unsigned char default_keyboardmap[] = {
	0x31, 0x00, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00, 0x00, 0x32, 0x00, 0x00, 0x00, 0x00, 0x40, 
	0x00, 0x00, 0x00, 0x00, 0x33, 0x00, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00, 0x00, 0x34, 0x00, 
	0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x00, 0x35, 0x00, 0x00, 0x00, 0x00, 0x25, 0x00, 0x00, 
	0x00, 0x00, 0x36, 0x00, 0x00, 0x00, 0x00, 0x26, 0x00, 0x00, 0x00, 0x00, 0x37, 0x00, 0x00, 0x00, 
	0x00, 0x2a, 0x00, 0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x00, 
	0x39, 0x00, 0x00, 0x00, 0x00, 0x29, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x5f, 
	0x00, 0x00, 0x00, 0x00, 0x2d, 0x00, 0x00, 0x00, 0x00, 0x5e, 0x00, 0x00, 0x00, 0x00, 0x3d, 0x00, 
	0x00, 0x00, 0x00, 0x2b, 0x00, 0x00, 0x00, 0x00, 0x71, 0x00, 0x00, 0x00, 0x00, 0x51, 0x00, 0x00, 
	0x00, 0x00, 0x77, 0x00, 0x00, 0x00, 0x00, 0x57, 0x00, 0x00, 0x00, 0x00, 0x65, 0x00, 0x00, 0x00, 
	0x00, 0x45, 0x00, 0x00, 0x00, 0x00, 0x72, 0x00, 0x00, 0x00, 0x00, 0x52, 0x00, 0x00, 0x00, 0x00, 
	0x74, 0x00, 0x00, 0x00, 0x00, 0x54, 0x00, 0x00, 0x00, 0x00, 0x79, 0x00, 0x00, 0x00, 0x00, 0x59, 
	0x00, 0x00, 0x00, 0x00, 0x75, 0x00, 0x00, 0x00, 0x00, 0x55, 0x00, 0x00, 0x00, 0x00, 0x69, 0x00, 
	0x00, 0x00, 0x00, 0x49, 0x00, 0x00, 0x00, 0x00, 0x6f, 0x00, 0x00, 0x00, 0x00, 0x4f, 0x00, 0x00, 
	0x00, 0x00, 0x70, 0x00, 0x00, 0x00, 0x00, 0x50, 0x00, 0x00, 0x00, 0x00, 0x5b, 0x00, 0x00, 0x00, 
	0x00, 0x5d, 0x00, 0x00, 0x00, 0x00, 0x7b, 0x00, 0x00, 0x00, 0x00, 0x7d, 0x00, 0x00, 0x00, 0x00, 
	0x61, 0x00, 0x00, 0x00, 0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x73, 0x00, 0x00, 0x00, 0x00, 0x53, 
	0x00, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x00, 0x44, 0x00, 0x00, 0x00, 0x00, 0x66, 0x00, 
	0x00, 0x00, 0x00, 0x46, 0x00, 0x00, 0x00, 0x00, 0x67, 0x00, 0x00, 0x00, 0x00, 0x47, 0x00, 0x00, 
	0x00, 0x00, 0x68, 0x00, 0x00, 0x00, 0x00, 0x48, 0x00, 0x00, 0x00, 0x00, 0x6a, 0x00, 0x00, 0x00, 
	0x00, 0x4a, 0x00, 0x00, 0x00, 0x00, 0x6b, 0x00, 0x00, 0x00, 0x00, 0x4b, 0x00, 0x00, 0x00, 0x00, 
	0x6c, 0x00, 0x00, 0x00, 0x00, 0x4c, 0x00, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x00, 0x00, 0x00, 0x3a, 
	0x00, 0x00, 0x00, 0x00, 0x27, 0x00, 0x00, 0x00, 0x00, 0x22, 0x00, 0x00, 0x00, 0x00, 0x5c, 0x00, 
	0x00, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x00, 0x00, 0x7a, 0x00, 0x00, 0x00, 0x00, 0x5a, 0x00, 0x00, 
	0x00, 0x00, 0x78, 0x00, 0x00, 0x00, 0x00, 0x58, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 
	0x00, 0x43, 0x00, 0x00, 0x00, 0x00, 0x76, 0x00, 0x00, 0x00, 0x00, 0x56, 0x00, 0x00, 0x00, 0x00, 
	0x62, 0x00, 0x00, 0x00, 0x00, 0x42, 0x00, 0x00, 0x00, 0x00, 0x6e, 0x00, 0x00, 0x00, 0x00, 0x4e, 
	0x00, 0x00, 0x00, 0x00, 0x6d, 0x00, 0x00, 0x00, 0x00, 0x4d, 0x00, 0x00, 0x00, 0x00, 0x2c, 0x00, 
	0x00, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x2e, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x00, 
	0x00, 0x00, 0x2f, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x00, 0x00
};

#define MAPSIZE 480

void LoadKeyboardMaps(){
	if(appPath[0] == 0 || !ChangeInterface(appPath, NOTSILENT)){
		return;
	}

	char Path[256];

	sprintf(Path,"%s/kbmaps/keyboardmap0",appPath);
	if(file_exists(Path)){
		LoadFile((char *)(&(default_keyboardmap[0])), Path, MAPSIZE, NOTSILENT); //Overwrite the default keyboard
	}

	int i = 1;
	do{
		sprintf(Path,"%s/kbmaps/keyboardmap%d",appPath,i++);
	}while(file_exists(Path));

	KeyboardMapsNum = i-2;

	if(KeyboardMapsNum == 0) return;

	Keyboard * temp = NULL;

	temp = (Keyboard *)mem2_calloc( KeyboardMapsNum, sizeof(Keyboard), MEM2_GUI);

	if(!temp) return;

	i = 0;
	while(i < KeyboardMapsNum){
		sprintf(Path,"%s/kbmaps/keyboardmap%d",appPath,i+1);
		LoadFile((char *)(&(temp[i++])), Path, MAPSIZE, NOTSILENT);
	}

	KeyboardMaps = temp;
}


void GuiKeyboard::UTF8_analize(const char * utf8){
mem_len = 0; char_len = 0;

	while(utf8[mem_len]){
		if (utf8[mem_len] < 0x80){
		    mem_len++;
		}else if ((utf8[mem_len] >> 5) == 0x06){
		    mem_len += 2;
		}else if ((utf8[mem_len] >> 4) == 0x0e){
		    mem_len += 3;
		}else{
		    mem_len += 4;
		}

		char_len++;
	}

cursor_char_pos = char_len;
cursor_mem_pos = mem_len;
}
static char tmptxt[MAX_KEYBOARD_DISPLAY*4];

static char * GetDisplayText(char * t, int len)
{
	if(!t)
		return NULL;

	if(len <= MAX_KEYBOARD_DISPLAY)
		return t;

	//The last MAX_KEYBOARD_DISPLAY characters must be drawn!
	//Get the address of the len-MAX_KEYBOARD_DISPLAY character
	int i=0, c=0;
	while(i++ < len-MAX_KEYBOARD_DISPLAY){
		if (t[c] < 0x80){
			c++;
		}else if ((t[c] >> 5) == 0x06){
			c += 2;
		}else if ((t[c] >> 4) == 0x0e){
			c += 3;
		}else{
			c += 4;
		}		
	}

	sprintf(tmptxt,"%s",&t[c]);
	return tmptxt;
}
/**
 * Constructor for the GuiKeyboard class.
 */

GuiKeyboard::GuiKeyboard(char * t, u32 max, int tw)
{
	width = 540;
	height = 400;
	shift = 0;
	caps = 0;
	selectable = true;
	focus = 0; // allow focus
	alignmentHor = ALIGN_CENTRE;
	alignmentVert = ALIGN_MIDDLE;
	memset(kbtextstr, 0, sizeof(kbtextstr));
	strcpy(kbtextstr, t);
	UTF8_analize(t);
	kbtextmaxlen = max;

	isIR = 1;
	tweet = tw;
	CurrentKbIndex = 0;
	current_keyboard = (Keyboard *)(&default_keyboardmap);

	keyTextbox = new GuiImageData(keyboard_textbox_png);
	keyTextboxImg = new GuiImage(keyTextbox);
	keyTextboxImg->SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
	keyTextboxImg->SetPosition(0, 0);
	this->Append(keyTextboxImg);


	sprintf(char_len_str, "%d", char_len);
	TextLen = new GuiText(char_len_str, 20, (GXColor){0xff, 0xff, 0xff, 0xff});
	TextLen->SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
	TextLen->SetPosition(230, 13);
	TextLen->SetVisible(tw);
	this->Append(TextLen);

	kbText = new GuiText(GetDisplayText(kbtextstr, char_len), 20, (GXColor){0, 0, 0, 0xff});
	kbText->SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
	kbText->SetPosition(0, 13);
	this->Append(kbText);

	key = new GuiImageData(keyboard_key_png);
	keyOver = new GuiImageData(keyboard_key_over_png);
	ac_key = new GuiImageData(ac_keyboard_key_png);
	ac_keyOver = new GuiImageData(ac_keyboard_key_over_png);
	keyMedium = new GuiImageData(keyboard_mediumkey_png);
	keyMediumOver = new GuiImageData(keyboard_mediumkey_over_png);
	keyLarge = new GuiImageData(keyboard_largekey_png);
	keyLargeOver = new GuiImageData(keyboard_largekey_over_png);

	trigA = new GuiTrigger;
	trigA->SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A, PAD_BUTTON_A);
	trig1 = new GuiTrigger;
	trig1->SetSimpleTrigger(-1, WPAD_BUTTON_1, 0);
	trig2 = new GuiTrigger;
	trig2->SetSimpleTrigger(-1, WPAD_BUTTON_2, 0);

	irKeyboard = new GuiWindow(width, height);
	irKeyboard->SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
	irKeyboard->SetPosition(0, 0);

	acKeyboard = new GuiWindow(width, height);
	acKeyboard->SetAlignment(ALIGN_CENTRE, ALIGN_MIDDLE);
	acKeyboard->SetPosition(0, 0);
	acKeyboard->SetVisible(0);

	keyBackImg = new GuiImage(keyMedium);
	keyBackOverImg = new GuiImage(keyMediumOver);
	keyBackText = new GuiText("Back", 20, (GXColor){0, 0, 0, 0xff});
	keyBack = new GuiButton(keyMedium->GetWidth(), keyMedium->GetHeight());
	keyBack->SetImage(keyBackImg);
	keyBack->SetImageOver(keyBackOverImg);
	keyBack->SetLabel(keyBackText);
	keyBack->SetTrigger(trigA);
	keyBack->SetPosition(10*42+40, 0*42+80);
	keyBack->SetEffectGrow();
	irKeyboard->Append(keyBack);

	keyCapsImg = new GuiImage(keyMedium);
	keyCapsOverImg = new GuiImage(keyMediumOver);
	keyCapsText = new GuiText("Caps", 20, (GXColor){0, 0, 0, 0xff});
	keyCaps = new GuiButton(keyMedium->GetWidth(), keyMedium->GetHeight());
	keyCaps->SetImage(keyCapsImg);
	keyCaps->SetImageOver(keyCapsOverImg);
	keyCaps->SetLabel(keyCapsText);
	keyCaps->SetTrigger(trigA);
	keyCaps->SetPosition(0, 2*42+80);
	keyCaps->SetEffectGrow();
	irKeyboard->Append(keyCaps);

	keyShiftImg = new GuiImage(keyMedium);
	keyShiftOverImg = new GuiImage(keyMediumOver);
	keyShiftText = new GuiText("Shift", 20, (GXColor){0, 0, 0, 0xff});
	keyShift = new GuiButton(keyMedium->GetWidth(), keyMedium->GetHeight());
	keyShift->SetImage(keyShiftImg);
	keyShift->SetImageOver(keyShiftOverImg);
	keyShift->SetLabel(keyShiftText);
	keyShift->SetTrigger(trigA);
	keyShift->SetPosition(21, 3*42+80);
	keyShift->SetEffectGrow();
	irKeyboard->Append(keyShift);

	keySpaceImg = new GuiImage(keyLarge);
	keySpaceOverImg = new GuiImage(keyLargeOver);
	keySpace = new GuiButton(keyLarge->GetWidth(), keyLarge->GetHeight());
	keySpace->SetImage(keySpaceImg);
	keySpace->SetImageOver(keySpaceOverImg);
	keySpace->SetTrigger(trigA);
	keySpace->SetPosition(0, 4*42+80);
	keySpace->SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
	keySpace->SetEffectGrow();
	irKeyboard->Append(keySpace);

	char txt[12] = { 0 };

	for(int i=0; i<4; i++)
	{
		for(int j=0; j<11; j++)
		{
			if(j == 10 && (!i || i == 3)) continue;

			{
				strcpy(txt, current_keyboard->keys[i][j].ch);
				keyImg[i][j] = new GuiImage(key);
				keyImgOver[i][j] = new GuiImage(keyOver);
				keyTxt[i][j] = new GuiText(txt, 20, (GXColor){0, 0, 0, 0xff});
				keyTxt[i][j]->SetAlignment(ALIGN_CENTRE, ALIGN_BOTTOM);
				keyTxt[i][j]->SetPosition(0, -10);
				keyBtn[i][j] = new GuiButton(key->GetWidth(), key->GetHeight());
				keyBtn[i][j]->SetImage(keyImg[i][j]);
				keyBtn[i][j]->SetImageOver(keyImgOver[i][j]);
				keyBtn[i][j]->SetTrigger(trigA);
				keyBtn[i][j]->SetLabel(keyTxt[i][j]);
				keyBtn[i][j]->SetPosition(j*42+21*i+40, i*42+80);
				keyBtn[i][j]->SetEffectGrow();
				irKeyboard->Append(keyBtn[i][j]);
			}

			if( j<6 ){
				sprintf(txt, "%s   %s", current_keyboard->keys[i][j*2].ch, current_keyboard->keys[i][j*2+1].ch);
				ac_keyImg[i][j] = new GuiImage(ac_key);
				ac_keyImgOver[i][j] = new GuiImage(ac_keyOver);
				ac_keyTxt[i][j] = new GuiText(txt, 20, (GXColor){0, 0, 0, 0xff});
				ac_keyTxt[i][j]->SetAlignment(ALIGN_CENTRE, ALIGN_BOTTOM);
				ac_keyTxt[i][j]->SetPosition(0, -10);
				ac_keyBtn[i][j] = new GuiButton(ac_key->GetWidth(), ac_key->GetHeight());
				ac_keyBtn[i][j]->SetImage(ac_keyImg[i][j]);
				ac_keyBtn[i][j]->SetImageOver(ac_keyImgOver[i][j]);
				ac_keyBtn[i][j]->SetTrigger(trig1);
				ac_keyBtn[i][j]->SetTrigger(trig2);
				ac_keyBtn[i][j]->SetLabel(ac_keyTxt[i][j]);
				ac_keyBtn[i][j]->SetPosition(j*84+11*i, i*42+80);
				ac_keyBtn[i][j]->SetEffectGrow();
				ac_keyBtn[i][j]->ac_button = 0x20|(i<<3)|j;
				ac_keyBtn[i][j]->parentKeyboard = this;
				acKeyboard->Append(ac_keyBtn[i][j]);
			}
		}
	}
this->Append(irKeyboard);
this->Append(acKeyboard);
}

/**
 * Destructor for the GuiKeyboard class.
 */
GuiKeyboard::~GuiKeyboard()
{
	delete TextLen;
	delete kbText;
	delete keyTextbox;
	delete keyTextboxImg;
	delete keyCapsText;
	delete keyCapsImg;
	delete keyCapsOverImg;
	delete keyCaps;
	delete keyShiftText;
	delete keyShiftImg;
	delete keyShiftOverImg;
	delete keyShift;
	delete keyBackText;
	delete keyBackImg;
	delete keyBackOverImg;
	delete keyBack;
	delete keySpaceImg;
	delete keySpaceOverImg;
	delete keySpace;
	delete key;
	delete keyOver;
	delete ac_key;
	delete ac_keyOver;
	delete keyMedium;
	delete keyMediumOver;
	delete keyLarge;
	delete keyLargeOver;
	delete trigA;
	delete trig1;
	delete trig2;

	for(int i=0; i<4; i++)
	{
		for(int j=0; j<11; j++)
		{
			if(j == 10 && (!i || i == 3)) continue;

			{
				delete keyImg[i][j];
				delete keyImgOver[i][j];
				delete keyTxt[i][j];
				delete keyBtn[i][j];
			}
			if( j<6 ){
				delete ac_keyImg[i][j];
				delete ac_keyImgOver[i][j];
				delete ac_keyTxt[i][j];
				delete ac_keyBtn[i][j];
			}
		}
	}

	delete irKeyboard;
	delete acKeyboard;
}


// overloaded new operator
void *GuiKeyboard::operator new(size_t size)
{
	void *p = gui_malloc(size);

	if (!p)
	{
		bad_alloc ba;
		throw ba;
	}
	return p;
}

// overloaded delete operator
void GuiKeyboard::operator delete(void *p)
{
	gui_free(p);
}

// overloaded new operator for arrays
void *GuiKeyboard::operator new[](size_t size)
{
	void *p = gui_malloc(size);

	if (!p)
	{
		bad_alloc ba;
		throw ba;
	}
	return p;
}

// overloaded delete operator for arrays
void GuiKeyboard::operator delete[](void *p)
{
	gui_free(p);
}

#define ROLL_AMP 8.0f
#define NROLL_AMP 4.3f
#define PITCH_AMP 10.0f
#define NPITCH_AMP 4.3f

void GuiKeyboard::ac_select(double roll, double pitch){
 int i = -1, j = -1;
 if (roll > (2.0f*ROLL_AMP+1.5f*NROLL_AMP) || roll < (-2.0f*ROLL_AMP-1.5f*NROLL_AMP)) return;

	if(roll <= -ROLL_AMP-1.5f*NROLL_AMP){
		i  = 3;
	}else if(roll <= -NROLL_AMP*0.5f && roll > -ROLL_AMP-0.5f*NROLL_AMP){
		i = 2;
	}else if(roll <= 0.5f*NROLL_AMP+ROLL_AMP && roll > 0.5f*NROLL_AMP){
		i = 1;
	}else if(roll >= ROLL_AMP+1.5f*NROLL_AMP){
		i = 0;
	}

	if(pitch <= -2.0f*PITCH_AMP-2.5f*NPITCH_AMP){
		j = 5;
	}else if(pitch <= -PITCH_AMP-1.5f*NPITCH_AMP && pitch > -2.0f*PITCH_AMP-1.5f*NPITCH_AMP){
		j = 4;
	}else if(pitch <= -NPITCH_AMP * 0.5f && pitch > -0.5f*NPITCH_AMP-PITCH_AMP){
		j = 3;
	}else if(pitch <= 0.5f*NPITCH_AMP+PITCH_AMP && pitch > NPITCH_AMP * 0.5f){
		j = 2;
	}else if(pitch <= 2.0f*PITCH_AMP+1.5f*NPITCH_AMP && pitch > PITCH_AMP+1.5f*NPITCH_AMP){
		j = 1;
	}else if(pitch >= 2.0f*PITCH_AMP+2.5f*NPITCH_AMP){
		j = 0;
	}

	if(j == -1 || i == -1) return ;

ac_selected = (0x20|(i<<3)|j);
}

void GuiKeyboard::Update(GuiTrigger * t)
{
	if(_elements.size() == 0 || (state == STATE_DISABLED && parentElement))
		return;
	
	for (u8 i = 0; i < _elements.size(); i++)
	{
		try	{ _elements.at(i)->Update(t); }
		catch (const std::exception& e) { }
	}

	bool update = false;

	int doDelete=0, doSpace=0;

	if(KeyboardMaps){
		if(t->wpad->btns_d & WPAD_BUTTON_MINUS){
			if(--CurrentKbIndex < 0) CurrentKbIndex = KeyboardMapsNum;
			update = true;
		}else if(t->wpad->btns_d & WPAD_BUTTON_PLUS){
			if(++CurrentKbIndex > KeyboardMapsNum) CurrentKbIndex = 0;
			update = true;
		}
	}

	if((t->wpad->btns_d & WPAD_BUTTON_HOME) || (t->wpad->ir.valid && !isIR)){
		acKeyboard->SetVisible(isIR);
		isIR ^= 1;
		irKeyboard->SetVisible(isIR);
	}

	if(t->wpad->btns_h & WPAD_BUTTON_B){
		if(!caps){
			caps = 1; update = true;
		}
	}else if(t->wpad->btns_u & WPAD_BUTTON_B){
		caps = 0; update = true;
	}

	if(acKeyboard->IsVisible()){
		ac_select(t->wpad->orient.roll, t->wpad->orient.pitch);
		if(t->wpad->btns_d & WPAD_BUTTON_A){
			doDelete = 1;
		}else if(t->wpad->accel.z < 500){
			if(cursor_mem_pos && kbtextstr[cursor_mem_pos-1] != ' '){
				doSpace = 1;
			}
		}
	}else{
		if(keySpace->GetState() == STATE_CLICKED)
		{
			doSpace = 1;
			keySpace->SetState(STATE_SELECTED, t->chan);
		}
		else if(keyBack->GetState() == STATE_CLICKED)
		{
			doDelete = 1;
			keyBack->SetState(STATE_SELECTED, t->chan);
		}
		else if(keyShift->GetState() == STATE_CLICKED)
		{
			shift ^= 1;
			keyShift->SetState(STATE_SELECTED, t->chan);
			update = true;
		}
		else if(keyCaps->GetState() == STATE_CLICKED)
		{
			caps ^= 1;
			shift = 0;
			keyCaps->SetState(STATE_SELECTED, t->chan);
			update = true;
		}
	}

	if(doDelete){
		if(char_len > 0)
		{
			if(kbtextstr[--cursor_mem_pos] < 0x80){
				kbtextstr[cursor_mem_pos] = 0;
				mem_len--;
			}else if((kbtextstr[--cursor_mem_pos] >> 5) == 0x06){
				mem_len -= 2;
				memset(&(kbtextstr[cursor_mem_pos]),0,2);
			}else if((kbtextstr[--cursor_mem_pos] >> 4) == 0x0e){
				mem_len -= 3;
				memset(&(kbtextstr[cursor_mem_pos]),0,3);
			}else{
				mem_len -= 4;
				memset(&(kbtextstr[--cursor_mem_pos]),0,4);
			}
			char_len--; cursor_char_pos--;

			sprintf(char_len_str, "%d", char_len);
			TextLen->SetText(char_len_str);
			kbText->SetText(GetDisplayText(kbtextstr, char_len));
		}
	}else if(doSpace){
		if(char_len < kbtextmaxlen)
		{
			kbtextstr[mem_len] = ' ';
			kbText->SetText(GetDisplayText(kbtextstr, char_len));
			char_len++; cursor_char_pos++;
			mem_len++; cursor_mem_pos++;
			
			sprintf(char_len_str, "%d", char_len);
			TextLen->SetText(char_len_str);
		}
	}

	char txt[2][16] = { {0} , {0} };

	startloop:

	for(int i=0; i<4; i++)
	{
		for(int j=0; j<11; j++)
		{
			if(j == 10 && (!i || i == 3)) continue;

			if(update)
			{
				if(CurrentKbIndex == 0){
					current_keyboard = (Keyboard *)(&default_keyboardmap);
				}else{
					current_keyboard = &KeyboardMaps[CurrentKbIndex-1];
				}
				if(shift ^ caps){
					strcpy(txt[0], current_keyboard->keys[i][j].chShift);
					if( j<6 ){
						sprintf(txt[1], "%s   %s", current_keyboard->keys[i][j*2].chShift, current_keyboard->keys[i][j*2+1].chShift);
						ac_keyTxt[i][j]->SetText(txt[1]);
					}
				}else{
					strcpy(txt[0], current_keyboard->keys[i][j].ch);
					if( j<6 ){
						sprintf(txt[1], "%s   %s", current_keyboard->keys[i][j*2].ch, current_keyboard->keys[i][j*2+1].ch);
						ac_keyTxt[i][j]->SetText(txt[1]);
					}
				}

				keyTxt[i][j]->SetText(txt[0]);
			}

			if(irKeyboard->IsVisible()){
				if(keyBtn[i][j]->GetState() == STATE_CLICKED)
				{
					if(char_len < kbtextmaxlen)
					{
					char len;
						if(shift ^ caps)
						{
							len = strlen(current_keyboard->keys[i][j].chShift);
							sprintf(&(kbtextstr[cursor_mem_pos]), "%s", current_keyboard->keys[i][j].chShift);
						}
						else
						{
							len = strlen(current_keyboard->keys[i][j].ch);
							sprintf(&(kbtextstr[cursor_mem_pos]), "%s", current_keyboard->keys[i][j].ch);
						}
						char_len++; cursor_char_pos++;
						mem_len += len; cursor_mem_pos += len;

						sprintf(char_len_str, "%d", char_len);
						TextLen->SetText(char_len_str);
						kbText->SetText(GetDisplayText(kbtextstr, char_len));
					}
					keyBtn[i][j]->SetState(STATE_SELECTED, t->chan);

					if(shift)
					{
						shift ^= 1;
						update = true;
						goto startloop;
					}
				}
			}else{ //AC keyboard
				if( j >= 6) break;

				if(ac_keyBtn[i][j]->GetState() == STATE_CLICKED)
				{
					if(char_len < kbtextmaxlen)
					{
					char len;
					int alt = (t->wpad->btns_d & WPAD_BUTTON_2 ? 1 : 0);
//				sprintf(txt, "%s   %s", current_keyboard->keys[i][j*2].ch, current_keyboard->keys[i][j*2+1].ch);
						if(shift ^ caps)
						{
							len = strlen(current_keyboard->keys[i][j*2+alt].chShift);
							sprintf(&(kbtextstr[cursor_mem_pos]), "%s", current_keyboard->keys[i][j*2+alt].chShift);
						}
						else
						{
							len = strlen(current_keyboard->keys[i][j*2+alt].ch);
							sprintf(&(kbtextstr[cursor_mem_pos]), "%s", current_keyboard->keys[i][j*2+alt].ch);
						}
						char_len++; cursor_char_pos++;
						mem_len += len; cursor_mem_pos += len;

						sprintf(char_len_str, "%d", char_len);
						TextLen->SetText(char_len_str);
						kbText->SetText(GetDisplayText(kbtextstr, char_len));

					}

					ac_keyBtn[i][j]->SetState(STATE_SELECTED, t->chan);

					if(shift)
					{
						shift ^= 1;
						update = true;
						goto startloop;
					}
				}
			}
		}
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
}
