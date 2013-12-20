/****************************************************************************
 * WiiTweet
 *
 * Tantric 2008-2010
 *
 * networkop.cpp
 *
 * Network support routines
 *
 * Slightly modified by Pedro Aguiar
 ****************************************************************************/

#ifndef _NETWORKOP_H_
#define _NETWORKOP_H_

bool UpdateCheck();
bool DownloadUpdate();
void StartNetworkThread();
bool InitializeNetwork(bool silent);

extern bool updateFound;
#endif
