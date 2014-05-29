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

#include "client.h"
#include "xbmc_pvr_dll.h"
#include "AminoPVRData.h"
#include "platform/util/util.h"

using namespace std;
using namespace ADDON;

#ifdef TARGET_WINDOWS
#define snprintf _snprintf
#endif

bool            m_bCreated       = false;
ADDON_STATUS    m_CurStatus      = ADDON_STATUS_UNKNOWN;
AminoPVRData  * m_data           = NULL;
bool            m_bIsPlaying     = false;
AminoPVRChannel m_currentChannel;

/* User adjustable settings are saved here.
 * Default values are defined inside client.h
 * and exported to the other source files.
 */
std::string g_strHostname     = DEFAULT_HOST;             ///< The Host name or IP of the AminoPVR server
int         g_iPort           = DEFAULT_PORT;             ///< The AminoPVR Port (default is 8080)
std::string g_szApiKey        = DEFAULT_API_KEY;          ///< The AminoPVR Api Key (default is empty)
bool        g_UseHttpStreams  = false;
bool        g_SdOnly          = false;
std::string g_strUserPath     = "";
std::string g_strClientPath   = "";

CHelper_libXBMC_addon *XBMC   = NULL;
CHelper_libXBMC_pvr   *PVR    = NULL;
CHelper_libXBMC_gui   *GUI    = NULL;

extern "C" {

void ADDON_ReadSettings(void)
{
  // Read settings
  char *buffer;
  buffer = (char*)malloc(1024);
  buffer[0] = 0; /* Set the end of string */

  /* Read setting "host" from settings.xml */
  if (XBMC->GetSetting("host", buffer))
    g_strHostname = buffer;
  else
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'host' setting, falling back to '%s' as default", DEFAULT_HOST);
    g_strHostname = DEFAULT_HOST;
  }
  buffer[0] = 0;

  /* Read setting "port" from settings.xml */
  if (!XBMC->GetSetting("port", &g_iPort))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'port' setting, falling back to '%i' as default", DEFAULT_PORT);
    g_iPort = DEFAULT_PORT;
  }

  /* Read setting "api_key" from settings.xml */
  if (XBMC->GetSetting("api_key", buffer))
    g_szApiKey = buffer;
  else
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'api_key' setting, falling back to '%s' as default", DEFAULT_API_KEY);
    g_szApiKey = DEFAULT_API_KEY;
  }

  /* Read setting "use_http_streams" from settings.xml */
  if (!XBMC->GetSetting("use_http_streams", &g_UseHttpStreams))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'use_http_streams' setting, falling back to '%i' as default", false);
    g_UseHttpStreams = false;
  }

  /* Read setting "sd_only" from settings.xml */
  if (!XBMC->GetSetting("sd_only", &g_SdOnly))
  {
    /* If setting is unknown fallback to defaults */
    XBMC->Log(LOG_ERROR, "Couldn't get 'sd_only' setting, falling back to '%i' as default", false);
    g_SdOnly = false;
  }
  buffer[0] = 0;
}

ADDON_STATUS ADDON_Create(void* hdl, void* props)
{
  if (!hdl || !props)
    return ADDON_STATUS_UNKNOWN;

  PVR_PROPERTIES* pvrprops = (PVR_PROPERTIES*)props;

  XBMC = new CHelper_libXBMC_addon;
  if (!XBMC->RegisterMe(hdl))
  {
    SAFE_DELETE(XBMC);
    return ADDON_STATUS_PERMANENT_FAILURE;
  }

  PVR = new CHelper_libXBMC_pvr;
  if (!PVR->RegisterMe(hdl))
  {
    SAFE_DELETE(PVR);
    SAFE_DELETE(XBMC);
    return ADDON_STATUS_PERMANENT_FAILURE;
  }

  GUI = new CHelper_libXBMC_gui;
  if (!GUI->RegisterMe(hdl))
  {
    SAFE_DELETE(GUI);
    SAFE_DELETE(PVR);
    SAFE_DELETE(XBMC);
    return ADDON_STATUS_PERMANENT_FAILURE;
  }

  XBMC->Log(LOG_DEBUG, "%s - Creating the PVR demo add-on", __FUNCTION__);

  m_CurStatus     = ADDON_STATUS_UNKNOWN;
  g_strUserPath   = pvrprops->strUserPath;
  g_strClientPath = pvrprops->strClientPath;

  ADDON_ReadSettings();

  m_data      = new AminoPVRData;
  m_CurStatus = ADDON_STATUS_OK;
  m_bCreated  = true;
  return m_CurStatus;
}

ADDON_STATUS ADDON_GetStatus()
{
  return m_CurStatus;
}

void ADDON_Destroy()
{
  delete m_data;
  m_bCreated = false;
  m_CurStatus = ADDON_STATUS_UNKNOWN;

  if (PVR)
  {
    delete(PVR);
    PVR = NULL;
  }

  if (XBMC)
  {
    delete(XBMC);
    XBMC = NULL;
  }

  if (GUI)
  {
    delete(GUI);
    GUI = NULL;
  }
}

bool ADDON_HasSettings()
{
  return true;
}

unsigned int ADDON_GetSettings(ADDON_StructSetting ***sSet)
{
  return 0;
}

ADDON_STATUS ADDON_SetSetting(const char *settingName, const void *settingValue)
{
  string str = settingName;
  if (str == "host")
  {
    string tmp_sHostname;
    XBMC->Log(LOG_INFO, "Changed Setting 'host' from %s to %s", g_strHostname.c_str(), (const char*)settingValue);
    tmp_sHostname = g_strHostname;
    g_strHostname = (const char*)settingValue;
    if (tmp_sHostname != g_strHostname)
      return ADDON_STATUS_NEED_RESTART;
  }
  else if (str == "port")
  {
    XBMC->Log(LOG_INFO, "Changed Setting 'port' from %u to %u", g_iPort, *(int*)settingValue);
    if (g_iPort != *(int*)settingValue)
    {
      g_iPort = *(int*)settingValue;
      return ADDON_STATUS_NEED_RESTART;
    }
  }
  else if (str == "api_key")
  {
    string tmp_sApiKey;
    XBMC->Log(LOG_INFO, "Changed Setting 'api_key' from %s to %s", g_szApiKey.c_str(), (const char*)settingValue);
    tmp_sApiKey = g_szApiKey;
    g_szApiKey = (const char*)settingValue;
    if (tmp_sApiKey != g_szApiKey)
      return ADDON_STATUS_NEED_RESTART;
  }
  else if (str == "use_http_streams")
  {
    XBMC->Log(LOG_INFO, "Changed Setting 'use_http_streams' from %u to %u", g_UseHttpStreams, *(bool*)settingValue);
    if (g_UseHttpStreams != *(bool*)settingValue)
    {
      g_UseHttpStreams = *(bool*)settingValue;
      return ADDON_STATUS_NEED_RESTART;
    }
  }
  else if (str == "sd_only")
  {
    XBMC->Log(LOG_INFO, "Changed Setting 'sd_only' from %u to %u", g_SdOnly, *(bool*)settingValue);
    if (g_SdOnly != *(bool*)settingValue)
    {
      g_SdOnly = *(bool*)settingValue;
      return ADDON_STATUS_NEED_RESTART;
    }
  }
  return ADDON_STATUS_OK;
}

void ADDON_Stop()
{
  ADDON_Destroy();
}

void ADDON_FreeSettings()
{
}

void ADDON_Announce(const char *flag, const char *sender, const char *message, const void *data)
{
  (void) flag; (void) sender; (void) message; (void) data;
}

/***********************************************************
 * PVR Client AddOn specific public library functions
 ***********************************************************/

const char* GetPVRAPIVersion(void)
{
  static const char *strApiVersion = XBMC_PVR_API_VERSION;
  return strApiVersion;
}

const char* GetMininumPVRAPIVersion(void)
{
  static const char *strMinApiVersion = XBMC_PVR_MIN_API_VERSION;
  return strMinApiVersion;
}

const char* GetGUIAPIVersion(void)
{
  static const char *strGuiApiVersion = XBMC_GUI_API_VERSION;
  return strGuiApiVersion;
}

const char* GetMininumGUIAPIVersion(void)
{
  static const char *strMinGuiApiVersion = XBMC_GUI_MIN_API_VERSION;
  return strMinGuiApiVersion;
}

PVR_ERROR GetAddonCapabilities(PVR_ADDON_CAPABILITIES* pCapabilities)
{
  pCapabilities->bSupportsEPG                = true;
  pCapabilities->bSupportsTV                 = true;
  pCapabilities->bSupportsRadio              = false;
  pCapabilities->bSupportsChannelGroups      = false;
  pCapabilities->bSupportsRecordings         = true;
  pCapabilities->bSupportsRecordingFolders   = true;
  pCapabilities->bSupportsLastPlayedPosition = true;
  pCapabilities->bSupportsTimers             = true;

  return PVR_ERROR_NO_ERROR;
}

const char *GetBackendName(void)
{
  static const char *strBackendName = "AminoPVR";
  return strBackendName;
}

const char *GetBackendVersion(void)
{
  static CStdString strBackendVersion = "0.1";
  return strBackendVersion.c_str();
}

const char *GetConnectionString(void)
{
  static CStdString strConnectionString = "connected";
  return strConnectionString.c_str();
}

PVR_ERROR GetDriveSpace(long long *iTotal, long long *iUsed)
{
  *iTotal = 1024 * 1024 * 1024;
  *iUsed  = 0;
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd)
{
  if (m_data)
    return m_data->GetEPGForChannel(handle, channel, iStart, iEnd);

  return PVR_ERROR_SERVER_ERROR;
}

int GetChannelsAmount(void)
{
  if (m_data)
    return m_data->GetChannelsAmount();

  return -1;
}

PVR_ERROR GetChannels(ADDON_HANDLE handle, bool bRadio)
{
  if (m_data)
    return m_data->GetChannels(handle, bRadio);

  return PVR_ERROR_SERVER_ERROR;
}

bool OpenLiveStream(const PVR_CHANNEL &channel)
{
  if (m_data)
  {
    CloseLiveStream();

    if (m_data->GetChannel(channel, m_currentChannel))
    {
      m_bIsPlaying = true;
      return true;
    }
  }

  return false;
}

void CloseLiveStream(void)
{
  m_bIsPlaying = false;
}

int GetCurrentClientChannel(void)
{
  return m_currentChannel.Id;
}

bool SwitchChannel(const PVR_CHANNEL &channel)
{
  CloseLiveStream();

  return OpenLiveStream(channel);
}

PVR_ERROR GetStreamProperties(PVR_STREAM_PROPERTIES* pProperties)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}

PVR_ERROR SignalStatus(PVR_SIGNAL_STATUS &signalStatus)
{
  snprintf(signalStatus.strAdapterName, sizeof(signalStatus.strAdapterName), "pvr demo adapter 1");
  snprintf(signalStatus.strAdapterStatus, sizeof(signalStatus.strAdapterStatus), "OK");

  return PVR_ERROR_NO_ERROR;
}

int GetRecordingsAmount(void)
{
  if (m_data)
    return m_data->GetRecordingsAmount();

  return -1;
}

PVR_ERROR GetRecordings(ADDON_HANDLE handle)
{
  if (m_data)
    return m_data->GetRecordings(handle);

  return PVR_ERROR_NOT_IMPLEMENTED;
}

PVR_ERROR DeleteRecording(const PVR_RECORDING &recording)
{
    if (m_data)
      return m_data->DeleteRecording(recording);
    return PVR_ERROR_NOT_IMPLEMENTED;
}

PVR_ERROR SetRecordingLastPlayedPosition(const PVR_RECORDING &recording, int lastplayedposition)
{
    if (m_data)
      return m_data->SetRecordingLastPlayedPosition(recording, lastplayedposition);
    return PVR_ERROR_NOT_IMPLEMENTED;
}

int GetRecordingLastPlayedPosition(const PVR_RECORDING &recording)
{
    if (m_data)
      return m_data->GetRecordingLastPlayedPosition(recording);
    return -1;
}

int GetTimersAmount(void)
{
    if (m_data)
        return m_data->GetTimersAmount();
    return -1;
}

PVR_ERROR GetTimers( ADDON_HANDLE handle )
{
    if (m_data)
        return m_data->GetTimers( handle );
    return PVR_ERROR_SERVER_ERROR;
}

PVR_ERROR AddTimer( const PVR_TIMER & timer )
{
    if (m_data)
        return m_data->AddTimer( timer );
    return PVR_ERROR_SERVER_ERROR;
}

PVR_ERROR DeleteTimer( const PVR_TIMER & timer, bool bForceDelete )
{
    if (m_data)
        return m_data->DeleteTimer( timer, bForceDelete );
    return PVR_ERROR_SERVER_ERROR;
}

PVR_ERROR UpdateTimer( const PVR_TIMER & timer )
{
    if (m_data)
        return m_data->UpdateTimer( timer );
    return PVR_ERROR_SERVER_ERROR;
}

/** UNUSED API FUNCTIONS */
PVR_ERROR DialogChannelScan(void) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR CallMenuHook(const PVR_MENUHOOK &menuhook, const PVR_MENUHOOK_DATA &item) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DeleteChannel(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR RenameChannel(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR MoveChannel(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DialogChannelSettings(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DialogAddChannel(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
int GetChannelGroupsAmount(void) { return -1; }
PVR_ERROR GetChannelGroups(ADDON_HANDLE handle, bool bRadio) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group) { return PVR_ERROR_NOT_IMPLEMENTED; }
bool OpenRecordedStream(const PVR_RECORDING &recording) { return false; }
void CloseRecordedStream(void) {}
int ReadRecordedStream(unsigned char *pBuffer, unsigned int iBufferSize) { return 0; }
long long SeekRecordedStream(long long iPosition, int iWhence /* = SEEK_SET */) { return 0; }
long long PositionRecordedStream(void) { return -1; }
long long LengthRecordedStream(void) { return 0; }
void DemuxReset(void) {}
void DemuxFlush(void) {}
int ReadLiveStream(unsigned char *pBuffer, unsigned int iBufferSize) { return 0; }
long long SeekLiveStream(long long iPosition, int iWhence /* = SEEK_SET */) { return -1; }
long long PositionLiveStream(void) { return -1; }
long long LengthLiveStream(void) { return -1; }
const char * GetLiveStreamURL(const PVR_CHANNEL &channel) { return ""; }
PVR_ERROR GetRecordingEdl(const PVR_RECORDING&, PVR_EDL_ENTRY[], int*) { return PVR_ERROR_NOT_IMPLEMENTED; };
PVR_ERROR RenameRecording(const PVR_RECORDING &recording) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR SetRecordingPlayCount(const PVR_RECORDING &recording, int count) { return PVR_ERROR_NOT_IMPLEMENTED; }
void DemuxAbort(void) {}
DemuxPacket* DemuxRead(void) { return NULL; }
unsigned int GetChannelSwitchDelay(void) { return 0; }
void PauseStream(bool bPaused) {}
bool CanPauseStream(void) { return false; }
bool CanSeekStream(void) { return false; }
bool SeekTime(int,bool,double*) { return false; }
void SetSpeed(int) {};
time_t GetPlayingTime() { return 0; }
time_t GetBufferTimeStart() { return 0; }
time_t GetBufferTimeEnd() { return 0; }
}
