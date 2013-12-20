#ifndef OBJECTS_H
#define OBJECTS_H

static GuiImageData * pointer;
static GuiTrigger * trigA = NULL;
static GuiTrigger * trigHome = NULL;
static GuiWindow * mainWindow = NULL;
static GuiImage * disabled = NULL;
static GuiImageData * sbtnOutline = NULL;
static GuiImageData * sbtnOutlineOver = NULL;
static GuiImageData * btnOutline = NULL;
static GuiImageData * btnOutlineOver = NULL;
static GuiImageData * mbtnOutline = NULL;
static GuiImageData * dialogBox = NULL;
static GuiImageData * progressbarOutline = NULL;
static GuiImageData * progressbarEmpty = NULL;
static GuiImageData * progressbar = NULL;
static GuiImageData * throbber = NULL;
static GuiImageData * riverunitTop = NULL;
static GuiImageData * riverunitTile = NULL;
static GuiImageData * riverunitBottom = NULL;
static GuiImageData * tbCredits = NULL;
static GuiImageData * tbCreditsOver = NULL;
static GuiImageData * tbHome = NULL;
static GuiImageData * tbHomeOver = NULL;
static GuiImageData * tbMentions = NULL;
static GuiImageData * tbMentionsOver = NULL;
static GuiImageData * tbSettings = NULL;
static GuiImageData * tbSettingsOver = NULL;
static GuiImageData * tbSwitchuser = NULL;
static GuiImageData * tbSwitchuserOver = NULL;
static GuiImageData * tbTweet = NULL;
static GuiImageData * tbTweetOver = NULL;
static GuiImageData * tbSearch = NULL;
static GuiImageData * tbSearchOver = NULL;
static GuiText * disclaimer = NULL;

bool SetupGui(){
	pointer = new GuiImageData(player1_point_png);
	trigA = new GuiTrigger;
	trigA->SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A, PAD_BUTTON_A);
	trigHome = new GuiTrigger;
	trigHome->SetButtonOnlyTrigger(-1, WPAD_BUTTON_HOME | WPAD_CLASSIC_BUTTON_HOME, 0);
	disabled = new GuiImage(screenwidth, screenheight, (GXColor){0, 0, 0, 100});
	sbtnOutline = new GuiImageData(small_button_png);
	sbtnOutlineOver = new GuiImageData(small_button_over_png);
	btnOutline = new GuiImageData(button_png);
	btnOutlineOver = new GuiImageData(button_over_png);
	mbtnOutline = new GuiImageData(keyboard_mediumkey_png);
	dialogBox = new GuiImageData(prompt_png);
	progressbarOutline = new GuiImageData(progressbar_outline_png);
	progressbarEmpty = new GuiImageData(progressbar_empty_png);
	progressbar = new GuiImageData(progressbar_png);
	throbber = new GuiImageData(throbber_png);
	riverunitTop = new GuiImageData(riverunit_top_png);
	riverunitTile = new GuiImageData(riverunit_tile_png);
	riverunitBottom = new GuiImageData(riverunit_bottom_png);
	tbCredits = new GuiImageData(tb_credits_png);
	tbCreditsOver = new GuiImageData(tb_credits_over_png);
	tbHome = new GuiImageData(tb_home_png);
	tbHomeOver = new GuiImageData(tb_home_over_png);
	tbMentions = new GuiImageData(tb_mentions_png);
	tbMentionsOver = new GuiImageData(tb_mentions_over_png);
	tbSettings = new GuiImageData(tb_settings_png);
	tbSettingsOver = new GuiImageData(tb_settings_over_png);
	tbSwitchuser = new GuiImageData(tb_switchuser_png);
	tbSwitchuserOver = new GuiImageData(tb_switchuser_over_png);
	tbTweet = new GuiImageData(tb_tweet_png);
	tbTweetOver = new GuiImageData(tb_tweet_over_png);
	tbSearch = new GuiImageData(tb_search_png);
	tbSearchOver = new GuiImageData(tb_search_over_png);
	disclaimer = new GuiText ("WiiTweet is not related to Twitter in any other way than using its service. It is not related to Nintendo in any way other than this software's main target platform is its Wii. These companies were not involved in the development of WiiTweet.", 16, (GXColor){0, 0, 0, 255});
	disclaimer->SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
	disclaimer->SetPosition(0,0);
	disclaimer->SetWrap(true, 430);
return true;
}
#endif
