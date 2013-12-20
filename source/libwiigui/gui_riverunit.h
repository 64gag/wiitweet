#ifndef GUI_RIVERUNIT_H
#define GUI_RIVERUNIT_H

#include <string>

#define BRIEFMODE 0x10

enum {
	UNIT_PROFILE = 0,
	UNIT_TWEET,
	UNIT_USER
};

//Profile related
	enum {
		P_BTN_FG = 0,
		P_BTN_DEL,
		P_BTN_NIP,
		P_BTN_AUTH,
		P_BTN_QTY
	};

	enum {
		P_TXT_NAME = 0,
		P_TXT_AUTH,
		P_TXT_DELBTN,
		P_TXT_QTY
	};

	enum {
		P_IMG_DELB = 0,
		P_IMG_DELBO,
		P_IMG_PP,
		P_IMG_QTY
	};
	
	enum {
		P_TTIP_STUB,
		P_TTIP_QTY
	};
	
	struct ProfileData { 
		char NIP;
		char oauth;
		char name[64];
		char img;
	};

//Tweet related
	enum {
		T_BTN_RT = 0,
		T_BTN_FAV,
		T_BTN_REPLY,
		T_BTN_TWEET,
		T_BTN_USER,
		T_BTN_TIMELINE,
		T_BTN_QTY
	};

	enum {
		T_TXT_NAME = 0,
		T_TXT_SNAME,
		T_TXT_TWEET,
		T_TXT_RTBY,
		T_TXT_TIME,
		T_TXT_TIMELINE,
		T_TXT_QTY
	};

	enum {
		T_IMG_PP = 0,
		T_IMG_RT_SMALL,
		T_IMG_RT,
		T_IMG_RT_HO,
		T_IMG_RT_ON,
		T_IMG_FAV,
		T_IMG_FAV_HO,
		T_IMG_FAV_ON,
		T_IMG_REPLY,
		T_IMG_REPLY_HO,
		T_IMG_TIMELINE,
		T_IMG_TIMELINE_HO,
		T_IMG_QTY
	};

	enum {
		T_TTIP_FAV = 0,
		T_TTIP_RT,
		T_TTIP_REPLY,
		T_TTIP_QTY
	};

//User related
	enum {
		U_BTN_FOLLOWING = 0,
		U_BTN_FOLLOWERS,
		U_BTN_FAVOURITES,
		U_BTN_FOLLOW,
		U_BTN_FG,
		U_BTN_QTY
	};

	enum {
		U_TXT_NAME = 0,
		U_TXT_SNAME,
		U_TXT_LOCATION,
		U_TXT_URL,
		U_TXT_DESC,
		U_TXT_TWEETS,
		U_TXT_FOLLOWERS,
		U_TXT_FOLLOWING,
		U_TXT_NTWEETS,
		U_TXT_NFOLLOWERS,
		U_TXT_NFOLLOWING,
		U_TXT_FAVOURITES,
		U_TXT_BFOLLOWING,
		U_TXT_BFOLLOWERS,
		U_TXT_BFAVOURITES,
		U_TXT_QTY
	};

	enum {
		U_IMG_PP = 0,
		U_IMG_BFOLLOWERS,
		U_IMG_BFOLLOWING,
		U_IMG_BFAVOURITES,
		U_IMG_BFOLLOWERSH,
		U_IMG_BFOLLOWINGH,
		U_IMG_BFAVOURITESH,
		U_IMG_FOLLOW,
		U_IMG_UNFOLLOW,
		U_IMG_QTY
	};

	enum {
		U_TTIP_FOLLOW = 0,
		U_TTIP_QTY
	};
class GuiRiverUnit : public GuiWindow{
	public:
		GuiRiverUnit(int type, void * data, void * cache = NULL);
		~GuiRiverUnit();
		void *operator new(size_t size);
		void operator delete(void *p);
		void *operator new[](size_t size);
		void operator delete[](void *p);	
		GuiWindow * look, * bg, * fg;
		GuiImage *top, *tile, *bottom;
		GuiButton * buttons;
		GuiText * texts;
		GuiImage * images;
		GuiTooltip * tooltips;

		GuiImageData * imgdata;
		int ruType;

		void Update(GuiTrigger *t);

		std::string rt_id;
		bool DoRetweet(class wt_profile * profile, struct t_tweet * tweet);
		bool DoFavorite(class wt_profile * profile, struct t_tweet * tweet);
		bool DoFollow(class wt_profile * profile, struct t_user * user);
		void SetFollow(int following, struct t_user *user);
};

#endif
