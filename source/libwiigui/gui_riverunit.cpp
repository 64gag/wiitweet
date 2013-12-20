/****************************************************************************
 * libwiigui
 *
 * Pedro Aguiar 2012
 *
 * gui_riverunit.cpp
 *
 * GuiRiverUnit class definition
 ***************************************************************************/

#include "gui.h"
#include "gui_riverunit.h"
#include "twitter.h"
#include "fileop.h"
#include "wiitweet.h"
#include "cache.h"
#include "wt_profile.h"

#define RIVERUNITS_WIDTH 500

//strings
const char authmsg_pin_msg[] = "PIN needed";
const char authmsg_done_msg[] = "Authorized account";
const char dofavorite_msg[] = "Favorite";
const char undofavorite_msg[] = "Undo favorite";
const char doretweet_msg[] = "Retweet";
const char undoretweet_msg[] = "Undo retweet";
const char doreply_msg[] = "Reply";
const char dofollow_msg[] = "Follow";
const char undofollow_msg[] = "Unfollow";

//commonly used shared objects...
static GuiImageData *riverunitTop = NULL;
static GuiImageData *riverunitTile = NULL;
static GuiImageData *riverunitBottom = NULL;
static GuiImageData *btnImgData = NULL;
static GuiImageData *btnImgOverData = NULL;
static GuiImageData *FavImgData = NULL;
static GuiImageData *FavImgHovData = NULL;
static GuiImageData *FavImgOnData = NULL;
static GuiImageData *RtImgData = NULL;
static GuiImageData *RtImgHovData = NULL;
static GuiImageData *RtImgOnData = NULL;
static GuiImageData *ReplyImgData = NULL;
static GuiImageData *ReplyImgHovData = NULL;
static GuiImageData *RetweetIconData = NULL;
static GuiImageData *FollowIconData = NULL;
static GuiImageData *UnfollowIconData = NULL;
static GuiTrigger * trigA = NULL;


void GetDisplayTime(char * strtime, time_t created, int type){

struct tm * timeinfo;
timeinfo = localtime ( &created );

	if(type & BRIEFMODE){
		strftime (strtime, 96, "%I:%M%p - %d %b %y", timeinfo);
		strtime[5] |= 0x20; strtime[6] |= 0x20;		
	}else{
		double diff = difftime(time(NULL), created);
		if( diff < 60.0f){
			sprintf(strtime, "%ds", (int)diff);
		}else if( diff < 3600.0f){
			sprintf(strtime, "%dm", (int)(diff/60));
		}else if( diff < 86400.0f){
			sprintf(strtime, "%dh", (int)(diff/3600));		
		}else{
			strftime (strtime, 96, "%d %b", timeinfo);
		}
	}
}

GuiRiverUnit::GuiRiverUnit(int type, void *data, void * cache){

	ruType = type;
	width = RIVERUNITS_WIDTH;
	imgdata = NULL;
	rt_id = "";
	if(riverunitTop == NULL)
	{
		riverunitTop = new GuiImageData(riverunit_top_png);
		riverunitTile = new GuiImageData(riverunit_tile_png);
		riverunitBottom = new GuiImageData(riverunit_bottom_png);
		btnImgData = new GuiImageData(small_button_png);
		btnImgOverData = new GuiImageData(small_button_over_png);
		FavImgData = new GuiImageData(favorite_png);
		FavImgHovData = new GuiImageData(favorite_hover_png);
		FavImgOnData = new GuiImageData(favorite_on_png);
		RtImgData = new GuiImageData(retweet_png);
		RtImgHovData = new GuiImageData(retweet_hover_png);
		RtImgOnData = new GuiImageData(retweet_on_png);
		ReplyImgData = new GuiImageData(reply_png);
		ReplyImgHovData = new GuiImageData(reply_hover_png);
		RetweetIconData = new GuiImageData(small_retweet_png);
		FollowIconData = new GuiImageData(follow_button_png);
		UnfollowIconData = new GuiImageData(unfollow_button_png);
		trigA = new GuiTrigger;
		trigA->SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A, PAD_BUTTON_A);
	}


	struct t_tweet * tweet;
	struct t_user * user;
	switch(type & 0xf){
		case UNIT_PROFILE:
			buttons = new GuiButton[P_BTN_QTY];
			texts = new GuiText[P_TXT_QTY];
			images = new GuiImage[P_IMG_QTY];
			tooltips = new GuiTooltip[P_TTIP_QTY];
			
				height = 80;

			/* < Background window > */
				bg = new GuiWindow(RIVERUNITS_WIDTH, height);
				buttons[P_BTN_DEL].SetSize(btnImgData->GetWidth(), btnImgData->GetHeight());
				images[P_IMG_DELB].SetImage(btnImgData);
				images[P_IMG_DELBO].SetImage(btnImgOverData);
				texts[P_TXT_DELBTN].SetText("Delete");
				texts[P_TXT_DELBTN].SetAlignment(ALIGN_CENTRE, ALIGN_MIDDLE);
				texts[P_TXT_DELBTN].SetPosition(0,0);
				buttons[P_BTN_DEL].SetTrigger(trigA);
				buttons[P_BTN_DEL].SetAlignment(ALIGN_CENTRE, ALIGN_MIDDLE);
				buttons[P_BTN_DEL].SetPosition(0,0);
				buttons[P_BTN_DEL].SetLabel(&texts[P_TXT_DELBTN]);
				buttons[P_BTN_DEL].SetImage(&images[P_IMG_DELB]);
				buttons[P_BTN_DEL].SetImageOver(&images[P_IMG_DELBO]);
				bg->SetVisible(0);
				bg->Append(&buttons[P_BTN_DEL]);
			/* </ Background window > */

			/* < Foreground window > */
				fg = new GuiWindow(RIVERUNITS_WIDTH, height);
				
				if(((struct ProfileData *)data)->img){
					char * buffer = (char *)mem2_malloc(1024*300, MEM2_GUI);
					if(buffer){
						char filename[256];
						sprintf(filename, "%s/profiles/%s_img", appPath, ((struct ProfileData *)data)->name);
						int read = LoadFile(buffer, filename, 0, SILENT);

						if(read){
							imgdata = new GuiImageData((u8 *)buffer, read, GX_TF_RGBA8);
							images[P_IMG_PP].SetImage(imgdata);
							images[P_IMG_PP].SetScale(48,48);
							images[P_IMG_PP].SetAlignment(ALIGN_LEFT, ALIGN_MIDDLE);
							images[P_IMG_PP].SetPosition(11,0);
							fg->Append(&images[P_IMG_PP]);
						}
						mem2_free(buffer, MEM2_GUI);
					}
				}
				buttons[P_BTN_FG].SetSize(RIVERUNITS_WIDTH, height);
				buttons[P_BTN_FG].SetTrigger(trigA);
				buttons[P_BTN_FG].SetAlignment(ALIGN_LEFT, ALIGN_TOP);
				buttons[P_BTN_FG].SetPosition(0,0);

				texts[P_TXT_NAME].SetText(((struct ProfileData *)data)->name);
				texts[P_TXT_NAME].SetAlignment(ALIGN_LEFT, ALIGN_MIDDLE);
				texts[P_TXT_NAME].SetPosition(80,0);
				texts[P_TXT_NAME].SetFontSize(32);
				texts[P_TXT_AUTH].SetText(((struct ProfileData *)data)->oauth == 1 ? authmsg_pin_msg : authmsg_done_msg);
				texts[P_TXT_AUTH].SetFontSize(17);
				texts[P_TXT_AUTH].SetAlignment(ALIGN_RIGHT, ALIGN_BOTTOM);
				texts[P_TXT_AUTH].SetPosition(-5,-3);

				fg->Append(&buttons[P_BTN_FG]);
				fg->Append(&texts[P_TXT_NAME]); fg->Append(&texts[P_TXT_AUTH]);
			/* </ Foreground window > */
		break;
		case UNIT_TWEET:
			buttons = new GuiButton[T_BTN_QTY];
			texts = new GuiText[T_TXT_QTY];
			images = new GuiImage[T_IMG_QTY];
			tooltips = new GuiTooltip[T_TTIP_QTY];

				tweet = (struct t_tweet *)data;
				
				if(tweet->retweetby[0]){
					height = 144;
				}else{
					height = 128;
				}
				
				if(tweet->retweet_id){
					char itoa[64];
					sprintf(itoa, "%llu", tweet->retweet_id);
					rt_id = itoa;
				}
			/* < Background window > */
				bg = new GuiWindow(RIVERUNITS_WIDTH, height);
				
				images[T_IMG_FAV].SetImage(FavImgData);
				images[T_IMG_FAV_HO].SetImage(FavImgHovData);
				images[T_IMG_FAV_ON].SetImage(FavImgOnData);
				images[T_IMG_RT].SetImage(RtImgData);
				images[T_IMG_RT_HO].SetImage(RtImgHovData);
				images[T_IMG_RT_ON].SetImage(RtImgOnData);
				images[T_IMG_REPLY].SetImage(ReplyImgData);
				images[T_IMG_REPLY_HO].SetImage(ReplyImgHovData);
				
				buttons[T_BTN_FAV].SetSize(FavImgData->GetWidth(), FavImgData->GetHeight());
				buttons[T_BTN_FAV].SetTrigger(trigA);
				buttons[T_BTN_FAV].SetAlignment(ALIGN_CENTRE, ALIGN_MIDDLE);
				buttons[T_BTN_FAV].SetPosition(100,-20);
				buttons[T_BTN_FAV].SetTooltip(&tooltips[T_TTIP_FAV]);
				buttons[T_BTN_FAV].SetEffectGrow();
				if(tweet->flags & FAVORITED){
					tooltips[T_TTIP_FAV].SetText(undofavorite_msg);
					buttons[T_BTN_FAV].SetImage(&images[T_IMG_FAV_ON]);
					buttons[T_BTN_FAV].SetImageOver(&images[T_IMG_FAV_ON]);
				}else{
					tooltips[T_TTIP_FAV].SetText(dofavorite_msg);
					buttons[T_BTN_FAV].SetImage(&images[T_IMG_FAV]);
					buttons[T_BTN_FAV].SetImageOver(&images[T_IMG_FAV_HO]);
				}
				
				buttons[T_BTN_RT].SetSize(RtImgData->GetWidth(), RtImgData->GetHeight());
				buttons[T_BTN_RT].SetTrigger(trigA);
				buttons[T_BTN_RT].SetAlignment(ALIGN_CENTRE, ALIGN_MIDDLE);
				buttons[T_BTN_RT].SetPosition(0, -20);
				buttons[T_BTN_RT].SetTooltip(&tooltips[T_TTIP_RT]);
				buttons[T_BTN_RT].SetEffectGrow();
				if(tweet->flags & RETWEETED){
					tooltips[T_TTIP_RT].SetText(undoretweet_msg);
					buttons[T_BTN_RT].SetImage(&images[T_IMG_RT_ON]);
					buttons[T_BTN_RT].SetImageOver(&images[T_IMG_RT_ON]);
				}else{
					tooltips[T_TTIP_RT].SetText(doretweet_msg);
					buttons[T_BTN_RT].SetImage(&images[T_IMG_RT]);
					buttons[T_BTN_RT].SetImageOver(&images[T_IMG_RT_HO]);
				}

				tooltips[T_TTIP_REPLY].SetText(doreply_msg);
				buttons[T_BTN_REPLY].SetSize(ReplyImgData->GetWidth(), ReplyImgData->GetHeight());
				buttons[T_BTN_REPLY].SetTrigger(trigA);
				buttons[T_BTN_REPLY].SetAlignment(ALIGN_CENTRE, ALIGN_MIDDLE);
				buttons[T_BTN_REPLY].SetPosition(-100, -20);
				buttons[T_BTN_REPLY].SetImage(&images[T_IMG_REPLY]);
				buttons[T_BTN_REPLY].SetImageOver(&images[T_IMG_REPLY_HO]);
				buttons[T_BTN_REPLY].SetTooltip(&tooltips[T_TTIP_REPLY]);
				buttons[T_BTN_REPLY].SetEffectGrow();

				texts[T_TXT_TIMELINE].SetText("Timeline");
				texts[T_TXT_TIMELINE].SetAlignment(ALIGN_CENTRE, ALIGN_MIDDLE);
				texts[T_TXT_TIMELINE].SetFontSize(18);
				texts[T_TXT_TIMELINE].SetColor((GXColor){0, 0, 0, 0xff});
				texts[T_TXT_TIMELINE].SetPosition(0, 0);
				images[T_IMG_TIMELINE].SetImage(btnImgData);
				images[T_IMG_TIMELINE_HO].SetImage(btnImgOverData);
				buttons[T_BTN_TIMELINE].SetSize(btnImgData->GetWidth(), btnImgData->GetHeight());
				buttons[T_BTN_TIMELINE].SetAlignment(ALIGN_CENTRE, ALIGN_MIDDLE);
				buttons[T_BTN_TIMELINE].SetPosition(0, 30);
				buttons[T_BTN_TIMELINE].SetLabel(&texts[T_TXT_TIMELINE]);
				buttons[T_BTN_TIMELINE].SetImage(&images[T_IMG_TIMELINE]);
				buttons[T_BTN_TIMELINE].SetImageOver(&images[T_IMG_TIMELINE_HO]);
				buttons[T_BTN_TIMELINE].SetTrigger(trigA);
				buttons[T_BTN_TIMELINE].SetEffectGrow();
								
				bg->SetVisible(0);
				bg->Append(&buttons[T_BTN_RT]); bg->Append(&buttons[T_BTN_REPLY]); bg->Append(&buttons[T_BTN_FAV]);
				bg->Append(&buttons[T_BTN_TIMELINE]);
			/* </ Background window > */

			/* < Foreground window > */
				fg = new GuiWindow(RIVERUNITS_WIDTH, height);
				if(!(type & BRIEFMODE)){
					imgdata = cacheImage(tweet->user.imageurl, (struct cached_imgs *)cache, 0, tweet->user.id);
					if(imgdata){
						images[T_IMG_PP].SetImage(imgdata);
						images[T_IMG_PP].SetScale(48,48);
						images[T_IMG_PP].SetAlignment(ALIGN_LEFT, ALIGN_MIDDLE);
						images[T_IMG_PP].SetPosition(11,0);
						fg->Append(&images[T_IMG_PP]);
					}
				}

				buttons[T_BTN_TWEET].SetSize(RIVERUNITS_WIDTH-70, height);
				buttons[T_BTN_TWEET].SetTrigger(trigA);
				buttons[T_BTN_TWEET].SetAlignment(ALIGN_LEFT, ALIGN_BOTTOM);
				buttons[T_BTN_TWEET].SetPosition(70, 0);

				buttons[T_BTN_USER].SetSize(70, height);
				buttons[T_BTN_USER].SetTrigger(trigA);
				buttons[T_BTN_USER].SetAlignment(ALIGN_LEFT, ALIGN_TOP);
				buttons[T_BTN_USER].SetPosition(0,0);
				
				texts[T_TXT_NAME].SetText(tweet->user.name);
				texts[T_TXT_NAME].SetAlignment(ALIGN_LEFT, ALIGN_TOP);
				texts[T_TXT_NAME].SetFontSize(20);
				texts[T_TXT_NAME].SetColor((GXColor){0, 0, 0, 0xff});
				texts[T_TXT_NAME].SetPosition(70,4);
				texts[T_TXT_SNAME].SetText(tweet->user.screenname);
				texts[T_TXT_SNAME].SetAlignment(ALIGN_LEFT, ALIGN_TOP);
				texts[T_TXT_SNAME].SetFontSize(16);
				texts[T_TXT_SNAME].SetColor((GXColor){0x80, 0x80, 0x80, 0xff});
				if(texts[T_TXT_NAME].GetTextWidth() + texts[T_TXT_SNAME].GetTextWidth() >= 378){
					texts[T_TXT_NAME].SetMaxWidth(374 - texts[T_TXT_SNAME].GetTextWidth());
					texts[T_TXT_NAME].SetScroll(SCROLL_HORIZONTAL);
				}
				texts[T_TXT_SNAME].SetPosition(texts[T_TXT_NAME].GetLeft() + texts[T_TXT_NAME].GetTextWidth() + 4, 6);

				if(tweet->retweetby[0]){
					images[T_IMG_RT_SMALL].SetImage(RetweetIconData);
					texts[T_TXT_RTBY].SetText(tweet->retweetby);
					texts[T_TXT_RTBY].SetFontSize(16);
					texts[T_TXT_RTBY].SetColor((GXColor){0x80, 0x80, 0x80, 0xff});
					if(type & BRIEFMODE){
						texts[T_TXT_RTBY].SetAlignment(ALIGN_CENTRE, ALIGN_BOTTOM);
						texts[T_TXT_RTBY].SetPosition(0, -8);
						images[T_IMG_RT_SMALL].SetAlignment(ALIGN_CENTRE, ALIGN_BOTTOM);
						images[T_IMG_RT_SMALL].SetPosition((texts[T_TXT_RTBY].GetTextWidth()+RetweetIconData->GetWidth()+4)/-2, -4);
					}else{
						images[T_IMG_RT_SMALL].SetAlignment(ALIGN_LEFT, ALIGN_BOTTOM);
						images[T_IMG_RT_SMALL].SetPosition(70, -4);
						texts[T_TXT_RTBY].SetAlignment(ALIGN_LEFT, ALIGN_BOTTOM);
						texts[T_TXT_RTBY].SetPosition(74 + RetweetIconData->GetWidth(), -8);
					}
					fg->Append(&texts[T_TXT_RTBY]); fg->Append(&images[T_IMG_RT_SMALL]);
				}

				texts[T_TXT_TWEET].SetText(tweet->text);
				
				if(type & BRIEFMODE){ 
					texts[T_TXT_TWEET].SetFontSize(20);
					texts[T_TXT_TWEET].SetColor((GXColor){0xff, 0x84, 0x00, 0xff});
					texts[T_TXT_TWEET].SetAlignment(ALIGN_LEFT, ALIGN_TOP);
					texts[T_TXT_TWEET].SetPosition(5,30);
					texts[T_TXT_TWEET].SetWrap(true, RIVERUNITS_WIDTH-10);				
				}else{
					texts[T_TXT_TWEET].SetFontSize(17);
					texts[T_TXT_TWEET].SetColor((GXColor){0xff, 0x84, 0x00, 0xff});
					texts[T_TXT_TWEET].SetAlignment(ALIGN_LEFT, ALIGN_TOP);
					texts[T_TXT_TWEET].SetPosition(70,30);
					texts[T_TXT_TWEET].SetWrap(true, RIVERUNITS_WIDTH-70);
				}
				char strtime[96];
				GetDisplayTime(strtime, tweet->created, type);
				texts[T_TXT_TIME].SetText(strtime);
				texts[T_TXT_TIME].SetAlignment(ALIGN_RIGHT, ALIGN_TOP);
				texts[T_TXT_TIME].SetFontSize(16);
				texts[T_TXT_TIME].SetColor((GXColor){0x80, 0x80, 0x80, 0xff});
				texts[T_TXT_TIME].SetPosition(-2,2);
				
				if(!(type & BRIEFMODE)){ 
					fg->Append(&buttons[T_BTN_USER]); fg->Append(&buttons[T_BTN_TWEET]);
					fg->Append(&texts[T_TXT_NAME]); fg->Append(&texts[T_TXT_SNAME]);
				}
				fg->Append(&texts[T_TXT_TIME]);
				fg->Append(&texts[T_TXT_TWEET]);
			/* </ Foreground window > */
		break;
		case UNIT_USER:
			buttons = new GuiButton[U_BTN_QTY];
			texts = new GuiText[U_TXT_QTY];
			images = new GuiImage[U_IMG_QTY];
			tooltips = new GuiTooltip[U_TTIP_QTY];

				user = (struct t_user *)data;

				texts[U_TXT_DESC].SetText(user->description);
				texts[U_TXT_DESC].SetFontSize(17);
				texts[U_TXT_DESC].SetColor((GXColor){0x50, 0x50, 0x50, 0xff});
				texts[U_TXT_DESC].SetAlignment(ALIGN_LEFT, ALIGN_TOP);
				texts[U_TXT_DESC].SetPosition(11, 70);
				texts[U_TXT_DESC].SetWrap(true, RIVERUNITS_WIDTH-11);
				int approx = (17 + 6) * (1 + (int)(strlen(user->description) / 50));
				height = 80 + approx;
				height += 4 - (height % 4);
				if(height < 120) height = 120;

			/* < Background window > */
				bg = new GuiWindow(RIVERUNITS_WIDTH, height);
				texts[U_TXT_TWEETS].SetText("Tweets: ");
				texts[U_TXT_TWEETS].SetAlignment(ALIGN_LEFT, ALIGN_MIDDLE);
				texts[U_TXT_TWEETS].SetFontSize(22);
				texts[U_TXT_TWEETS].SetColor((GXColor){0, 0, 0, 0xff});
				texts[U_TXT_TWEETS].SetPosition(235, -30);
				
				texts[U_TXT_FOLLOWERS].SetText("Followers: ");
				texts[U_TXT_FOLLOWERS].SetAlignment(ALIGN_LEFT, ALIGN_MIDDLE);
				texts[U_TXT_FOLLOWERS].SetFontSize(22);
				texts[U_TXT_FOLLOWERS].SetColor((GXColor){0, 0, 0, 0xff});
				texts[U_TXT_FOLLOWERS].SetPosition(235, 0);
				
				texts[U_TXT_FOLLOWING].SetText("Following: ");
				texts[U_TXT_FOLLOWING].SetAlignment(ALIGN_LEFT, ALIGN_MIDDLE);
				texts[U_TXT_FOLLOWING].SetFontSize(22);
				texts[U_TXT_FOLLOWING].SetColor((GXColor){0, 0, 0, 0xff});
				texts[U_TXT_FOLLOWING].SetPosition(235, 30);

				char itoa[32];
				sprintf(itoa, "%llu", user->tweets);
				texts[U_TXT_NTWEETS].SetText(itoa);
				texts[U_TXT_NTWEETS].SetAlignment(ALIGN_LEFT, ALIGN_MIDDLE);
				texts[U_TXT_NTWEETS].SetFontSize(22);
				texts[U_TXT_NTWEETS].SetColor((GXColor){0xf2, 0x7c, 0x27, 0xff});
				texts[U_TXT_NTWEETS].SetPosition(237+texts[U_TXT_TWEETS].GetTextWidth(), -30);
				
				sprintf(itoa, "%llu", user->followers);
				texts[U_TXT_NFOLLOWERS].SetText(itoa);
				texts[U_TXT_NFOLLOWERS].SetAlignment(ALIGN_LEFT, ALIGN_MIDDLE);
				texts[U_TXT_NFOLLOWERS].SetFontSize(22);
				texts[U_TXT_NFOLLOWERS].SetColor((GXColor){0xf2, 0x7c, 0x27, 0xff});
				texts[U_TXT_NFOLLOWERS].SetPosition(237+texts[U_TXT_FOLLOWERS].GetTextWidth(), 0);

				sprintf(itoa, "%llu", user->following);
				texts[U_TXT_NFOLLOWING].SetText(itoa);
				texts[U_TXT_NFOLLOWING].SetAlignment(ALIGN_LEFT, ALIGN_MIDDLE);
				texts[U_TXT_NFOLLOWING].SetFontSize(22);
				texts[U_TXT_NFOLLOWING].SetColor((GXColor){0xf2, 0x7c, 0x27, 0xff});
				texts[U_TXT_NFOLLOWING].SetPosition(237+texts[U_TXT_FOLLOWING].GetTextWidth(), 30);

				texts[U_TXT_BFOLLOWERS].SetText("Followers");
				texts[U_TXT_BFOLLOWERS].SetAlignment(ALIGN_CENTRE, ALIGN_MIDDLE);
				texts[U_TXT_BFOLLOWERS].SetFontSize(18);
				texts[U_TXT_BFOLLOWERS].SetColor((GXColor){0, 0, 0, 0xff});
				texts[U_TXT_BFOLLOWERS].SetPosition(0, 0);
				
				texts[U_TXT_BFOLLOWING].SetText("Following");
				texts[U_TXT_BFOLLOWING].SetAlignment(ALIGN_CENTRE, ALIGN_MIDDLE);
				texts[U_TXT_BFOLLOWING].SetFontSize(18);
				texts[U_TXT_BFOLLOWING].SetColor((GXColor){0, 0, 0, 0xff});
				texts[U_TXT_BFOLLOWING].SetPosition(0, 0);

				texts[U_TXT_BFAVOURITES].SetText("Favorites");
				texts[U_TXT_BFAVOURITES].SetAlignment(ALIGN_CENTRE, ALIGN_MIDDLE);
				texts[U_TXT_BFAVOURITES].SetFontSize(18);
				texts[U_TXT_BFAVOURITES].SetColor((GXColor){0, 0, 0, 0xff});
				texts[U_TXT_BFAVOURITES].SetPosition(0, 0);

				images[U_IMG_BFOLLOWING].SetImage(btnImgData);
				images[U_IMG_BFOLLOWINGH].SetImage(btnImgOverData);
				buttons[U_BTN_FOLLOWING].SetSize(btnImgData->GetWidth(), btnImgData->GetHeight());
				buttons[U_BTN_FOLLOWING].SetAlignment(ALIGN_LEFT, ALIGN_MIDDLE);
				buttons[U_BTN_FOLLOWING].SetPosition(80, -35);
				buttons[U_BTN_FOLLOWING].SetLabel(&texts[U_TXT_BFOLLOWING]);
				buttons[U_BTN_FOLLOWING].SetImage(&images[U_IMG_BFOLLOWING]);
				buttons[U_BTN_FOLLOWING].SetImageOver(&images[U_IMG_BFOLLOWINGH]);
				buttons[U_BTN_FOLLOWING].SetTrigger(trigA);

				images[U_IMG_BFOLLOWERS].SetImage(btnImgData);
				images[U_IMG_BFOLLOWERSH].SetImage(btnImgOverData);
				buttons[U_BTN_FOLLOWERS].SetSize(btnImgData->GetWidth(), btnImgData->GetHeight());
				buttons[U_BTN_FOLLOWERS].SetAlignment(ALIGN_LEFT, ALIGN_MIDDLE);
				buttons[U_BTN_FOLLOWERS].SetPosition(80, 0);
				buttons[U_BTN_FOLLOWERS].SetLabel(&texts[U_TXT_BFOLLOWERS]);
				buttons[U_BTN_FOLLOWERS].SetImage(&images[U_IMG_BFOLLOWERS]);
				buttons[U_BTN_FOLLOWERS].SetImageOver(&images[U_IMG_BFOLLOWERSH]);
				buttons[U_BTN_FOLLOWERS].SetTrigger(trigA);

				images[U_IMG_BFAVOURITES].SetImage(btnImgData);
				images[U_IMG_BFAVOURITESH].SetImage(btnImgOverData);
				buttons[U_BTN_FAVOURITES].SetSize(btnImgData->GetWidth(), btnImgData->GetHeight());
				buttons[U_BTN_FAVOURITES].SetAlignment(ALIGN_LEFT, ALIGN_MIDDLE);
				buttons[U_BTN_FAVOURITES].SetPosition(80, 35);
				buttons[U_BTN_FAVOURITES].SetLabel(&texts[U_TXT_BFAVOURITES]);
				buttons[U_BTN_FAVOURITES].SetImage(&images[U_IMG_BFAVOURITES]);
				buttons[U_BTN_FAVOURITES].SetImageOver(&images[U_IMG_BFAVOURITESH]);
				buttons[U_BTN_FAVOURITES].SetTrigger(trigA);
				
				bg->Append(&texts[U_TXT_TWEETS]); bg->Append(&texts[U_TXT_FOLLOWERS]); bg->Append(&texts[U_TXT_FOLLOWING]);
				bg->Append(&texts[U_TXT_NTWEETS]); bg->Append(&texts[U_TXT_NFOLLOWERS]); bg->Append(&texts[U_TXT_NFOLLOWING]);
				bg->Append(&buttons[U_BTN_FOLLOWING]); bg->Append(&buttons[U_BTN_FOLLOWERS]); bg->Append(&buttons[U_BTN_FAVOURITES]); 
				bg->SetVisible(0);
			/* </ Background window > */

			/* < Foreground window > */
				fg = new GuiWindow(RIVERUNITS_WIDTH, height);
			
				imgdata = cacheImage(user->imageurl, (struct cached_imgs *)cache, 0, user->id);
				if(imgdata){
					images[U_IMG_PP].SetImage(imgdata);
					images[U_IMG_PP].SetScale(48,48);
					images[U_IMG_PP].SetAlignment(ALIGN_LEFT, ALIGN_TOP);
					images[U_IMG_PP].SetPosition(11,11);
					fg->Append(&images[U_IMG_PP]);

				}

				images[U_IMG_FOLLOW].SetImage(FollowIconData);
				images[U_IMG_UNFOLLOW].SetImage(UnfollowIconData);
				buttons[U_BTN_FOLLOW].SetSize(FollowIconData->GetWidth(), FollowIconData->GetHeight());
				buttons[U_BTN_FOLLOW].SetAlignment(ALIGN_RIGHT, ALIGN_TOP);
				buttons[U_BTN_FOLLOW].SetPosition(-2, 4);
				if(user->flags & FOLLOWMASK){
					if(user->flags & FOLLOWING){
						tooltips[U_TTIP_FOLLOW].SetText(undofollow_msg);
						buttons[U_BTN_FOLLOW].SetImage(&images[U_IMG_UNFOLLOW]);
					}else{
						tooltips[U_TTIP_FOLLOW].SetText(dofollow_msg);
						buttons[U_BTN_FOLLOW].SetImage(&images[U_IMG_FOLLOW]);
					}
				}else{
					tooltips[U_TTIP_FOLLOW].SetText("Check");
					images[U_IMG_FOLLOW].SetAlpha(0x80);
					buttons[U_BTN_FOLLOW].SetImage(&images[U_IMG_FOLLOW]);
				}
				buttons[U_BTN_FOLLOW].SetTooltip(&tooltips[U_TTIP_FOLLOW]);
				buttons[U_BTN_FOLLOW].SetTrigger(trigA);
				buttons[U_BTN_FOLLOW].SetEffectGrow();

				buttons[U_BTN_FG].SetSize(RIVERUNITS_WIDTH, height);
				buttons[U_BTN_FG].SetTrigger(trigA);
				buttons[U_BTN_FG].SetAlignment(ALIGN_LEFT, ALIGN_TOP);
				buttons[U_BTN_FG].SetPosition(0,0);

				texts[U_TXT_NAME].SetText(user->name);
				texts[U_TXT_NAME].SetAlignment(ALIGN_LEFT, ALIGN_TOP);
				texts[U_TXT_NAME].SetFontSize(20);
				texts[U_TXT_NAME].SetColor((GXColor){0, 0, 0, 0xff});
				texts[U_TXT_NAME].SetPosition(70, 4);
				texts[U_TXT_SNAME].SetText(user->screenname);
				texts[U_TXT_SNAME].SetAlignment(ALIGN_LEFT, ALIGN_TOP);
				texts[U_TXT_SNAME].SetFontSize(16);
				texts[U_TXT_SNAME].SetColor((GXColor){0x80, 0x80, 0x80, 0xff});
				if(texts[U_TXT_NAME].GetTextWidth() + texts[U_TXT_SNAME].GetTextWidth() >= 378){
					texts[U_TXT_NAME].SetMaxWidth(374 - texts[U_TXT_SNAME].GetTextWidth());
					texts[U_TXT_NAME].SetScroll(SCROLL_HORIZONTAL);
				}
				texts[U_TXT_SNAME].SetPosition(texts[U_TXT_NAME].GetLeft() + texts[U_TXT_NAME].GetTextWidth() + 4, 6);

				texts[U_TXT_LOCATION].SetText(user->location);
				texts[U_TXT_LOCATION].SetFontSize(16);
				texts[U_TXT_LOCATION].SetColor((GXColor){0x80, 0x80, 0x80, 0xff});
				texts[U_TXT_LOCATION].SetAlignment(ALIGN_LEFT, ALIGN_TOP);
				texts[U_TXT_LOCATION].SetPosition(70, 26);

				texts[U_TXT_URL].SetText(user->url);
				texts[U_TXT_URL].SetFontSize(16);
				texts[U_TXT_URL].SetColor((GXColor){0x80, 0x80, 0x80, 0xff});
				texts[U_TXT_URL].SetAlignment(ALIGN_LEFT, ALIGN_TOP);
				texts[U_TXT_URL].SetPosition(70, 44);
				texts[U_TXT_URL].SetMaxWidth(425);
				texts[U_TXT_URL].SetScroll(SCROLL_HORIZONTAL);

				//XXX DESC moved to the top

				fg->Append(&texts[U_TXT_NAME]); fg->Append(&texts[U_TXT_SNAME]); fg->Append(&texts[U_TXT_LOCATION]);
				fg->Append(&texts[U_TXT_URL]); fg->Append(&texts[U_TXT_DESC]); fg->Append(&buttons[U_BTN_FG]);
				fg->Append(&buttons[U_BTN_FOLLOW]);
			/* </ Foreground window > */

		break;
	}

	/* < Look window >*/
		look = new GuiWindow(RIVERUNITS_WIDTH, height);
		if(type & BRIEFMODE){
			look->SetVisible(0);
		}
		top = new GuiImage(riverunitTop);
		tile = new GuiImage(riverunitTile);
		bottom = new GuiImage(riverunitBottom);
		top->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
		top->SetPosition(0, 0);
		tile->SetAlignment(ALIGN_LEFT, ALIGN_TOP);
		tile->SetPosition(0, top->GetHeight());
		tile->SetTileVertical((look->GetHeight()-top->GetHeight()*2)/tile->GetHeight());
		bottom->SetAlignment(ALIGN_LEFT, ALIGN_BOTTOM);
		bottom->SetPosition(0, 0);
		look->Append(top); look->Append(tile); look->Append(bottom);
	/* </ Look window > */

	this->Append(look);
	this->Append(bg);
	this->Append(fg);
}

GuiRiverUnit::~GuiRiverUnit(){
	delete [] buttons;
	delete [] texts;
	delete [] images;
	delete [] tooltips;
	
	if(imgdata) imgdata->Delete();

	delete look;
	delete fg;
	delete bg;
	delete top;
	delete tile;
	delete bottom;
}

// overloaded new operator
void *GuiRiverUnit::operator new(size_t size)
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
void GuiRiverUnit::operator delete(void *p)
{
	gui_free(p);
}

// overloaded new operator for arrays
void *GuiRiverUnit::operator new[](size_t size)
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
void GuiRiverUnit::operator delete[](void *p)
{
	gui_free(p);
}
//These functions are very alike TODO: merge them
bool GuiRiverUnit::DoFollow(class wt_profile * profile, struct t_user *user){
	char string_id[64];
	sprintf(string_id, "%llu", user->id);
	std::string uid(string_id);

	if(user->flags & FOLLOWING){
		if(profile->twitterObj->friendshipDestroy(uid, 1)){
			SetFollow(0, user);
			return true;
		}
	}else{
		if(profile->twitterObj->friendshipCreate(uid, 1)){
			SetFollow(1, user);
			return true;
		}
	}
	
	return false;
}

void GuiRiverUnit::SetFollow(int following, struct t_user * user){
	images[U_IMG_FOLLOW].SetAlpha(0xff);
	if(!following){
		tooltips[U_TTIP_FOLLOW].SetText(dofollow_msg);
		buttons[U_BTN_FOLLOW].SetImage(&images[U_IMG_FOLLOW]);
		user->flags &= ~FOLLOWMASK;
		user->flags |= NFOLLOWING;
	}else{
		tooltips[U_TTIP_FOLLOW].SetText(undofollow_msg);
		buttons[U_BTN_FOLLOW].SetImage(&images[U_IMG_UNFOLLOW]);
		user->flags &= ~FOLLOWMASK;
		user->flags |= FOLLOWING;
	}
}

bool GuiRiverUnit::DoRetweet(class wt_profile * profile, struct t_tweet * tweet){
	char string_id[64];
	sprintf(string_id, "%llu", tweet->id);
	std::string tweetid(string_id);
	
	if(tweet->flags & RETWEETED){
		if(rt_id.length() && profile->twitterObj->statusDestroyById(rt_id)){
			tooltips[T_TTIP_RT].SetText(doretweet_msg);
			buttons[T_BTN_RT].SetImage(&images[T_IMG_RT]);
			buttons[T_BTN_RT].SetImageOver(&images[T_IMG_RT_HO]);
			tweet->flags &= ~RETWEETED;
			return true;
		}
	}else{
		if(profile->twitterObj->retweet(tweetid)){
			std::string response;
			profile->twitterObj->getLastWebResponse(response);
			size_t pos_start, pos_end;
			pos_start = response.find("<id>") + 4;
			pos_end = response.find("</id>", pos_start);
			rt_id = response.substr(pos_start, pos_end - pos_start);
			tooltips[T_TTIP_RT].SetText(undoretweet_msg);
			buttons[T_BTN_RT].SetImage(&images[T_IMG_RT_ON]);
			buttons[T_BTN_RT].SetImageOver(&images[T_IMG_RT_ON]);

			tweet->flags |= RETWEETED;
			return true;
		}
	}
	
	return false;
}

bool GuiRiverUnit::DoFavorite(class wt_profile * profile,struct t_tweet * tweet){
	char string_id[64];
	sprintf(string_id, "%llu", tweet->id);
	std::string tweetid(string_id);

	if(tweet->flags & FAVORITED){
		if(profile->twitterObj->favoriteDestroy(tweetid)){
			tooltips[T_TTIP_FAV].SetText(dofavorite_msg);
			buttons[T_BTN_FAV].SetImage(&images[T_IMG_FAV]);
			buttons[T_BTN_FAV].SetImageOver(&images[T_IMG_FAV_HO]);
			tweet->flags &= ~FAVORITED;
			return true;
		}
	}else{
		if(profile->twitterObj->favoriteCreate(tweetid)){
			tooltips[T_TTIP_FAV].SetText(undofavorite_msg);
			buttons[T_BTN_FAV].SetImage(&images[T_IMG_FAV_ON]);
			buttons[T_BTN_FAV].SetImageOver(&images[T_IMG_FAV_ON]);
			tweet->flags |= FAVORITED;
			return true;
		}
	}
	
	return false;
}
void GuiRiverUnit::Update(GuiTrigger * t)
{
	if(_elements.size() == 0 || (state == STATE_DISABLED && parentElement))
		return;

	if(t->wpad->ir.valid && this->IsInside(t->wpad->ir.x, t->wpad->ir.y) && t->wpad->btns_h & WPAD_BUTTON_B){
		_elements.at(1)->SetVisible(1); //Background
		_elements.at(2)->SetVisible(0); //Foreground
	}else{
		_elements.at(1)->SetVisible(0);
		_elements.at(2)->SetVisible(1);
	}

	u32 elemSize = _elements.size();

	for(u32 i = 0; i < elemSize; ++i)
	{
		try	{
				_elements.at(i)->Update(t);
			}
		catch (const std::exception& e) { }
	}

	if(updateCB)
		updateCB(this);
}
