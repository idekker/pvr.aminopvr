#pragma once
/*
 *      Copyright (C) 2011 Pulse-Eight
 *      http://www.pulse-eight.com/
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <platform/util/StdString.h>
#include <libXBMC_addon.h>
#include <libXBMC_pvr.h>
#include <libXBMC_gui.h>

#define DEFAULT_HOST            "127.0.0.1"
#define DEFAULT_PORT            8080
#define DEFAULT_API_KEY         "secret"

extern bool                     m_bCreated;
extern std::string              g_strUserPath;
extern std::string              g_strClientPath;
extern std::string              g_strHostname;      ///< The Host name or IP of the AminoPVR server
extern int                      g_iPort;            ///< The AminoPVR Port (default is 8080)
extern std::string              g_szApiKey;         ///< The AminoPVR Api Key (default is empty)
extern bool                     g_UseHttpStreams;   ///< Use HTTP if TRUE, else use RTSP streams for recordings

extern ADDON::CHelper_libXBMC_addon *XBMC;
extern CHelper_libXBMC_pvr          *PVR;
extern CHelper_libXBMC_gui          *GUI;
