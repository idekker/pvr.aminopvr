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

#include <map>
#include <vector>
#include <json/json.h>
#include "p8-platform/util/StdString.h"
#include "client.h"
#include "categories.h"

enum RecordingStatus
{
    REC_STATUS_UNKNOWN              = 0,
    REC_STATUS_START_RECORDING      = 1,
    REC_STATUS_RECORDING_STARTED    = 2,
    REC_STATUS_STOP_RECORDING       = 3,
    REC_STATUS_RECORDING_FINISHED   = 4,
    REC_STATUS_RECORDING_UNFINISHED = 5
};

enum ScheduleType
{
    SCHEDULE_TYPE_ONCE                 = 1,
    SCHEDULE_TYPE_TIMESLOT_EVERY_DAY   = 2,
    SCHEDULE_TYPE_TIMESLOT_EVERY_WEEK  = 3,
    SCHEDULE_TYPE_ONCE_EVERY_DAY       = 4,
    SCHEDULE_TYPE_ONCE_EVERY_WEEK      = 5,
    SCHEDULE_TYPE_ANY_TIME             = 6,
    SCHEDULE_TYPE_MANUAL_EVERY_DAY     = 7,
    SCHEDULE_TYPE_MANUAL_EVERY_WEEKDAY = 8,
    SCHEDULE_TYPE_MANUAL_EVERY_WEEKEND = 9,
    SCHEDULE_TYPE_MANUAL_EVERY_WEEK    = 10
};

enum DuplicationType
{
    DUPLICATION_METHOD_NONE        = 0,
    DUPLICATION_METHOD_TITLE       = 1,
    DUPLICATION_METHOD_SUBTITLE    = 2,
    DUPLICATION_METHOD_DESCRIPTION = 4
};

struct AminoPVREpgEntry
{
    CStdString              EpgId;
    int                     Id;
    CStdString              Title;
    time_t                  StartTime;
    time_t                  EndTime;
    CStdString              Subtitle;
    CStdString              Description;
    std::vector<CStdString> Genres;
    std::vector<CStdString> Actors;
    std::vector<CStdString> Directors;
    std::vector<CStdString> Presenters;
    std::vector<CStdString> Ratings;
    CStdString              AspectRatio;
    CStdString              ParentalRating;
};

struct AminoPVRChannel
{
    int                             Id;
    CStdString                      EpgId;
    int                             Number;
    CStdString                      Name;
    CStdString                      Url;
    CStdString                      LogoPath;
    bool                            Radio;
    std::vector<AminoPVREpgEntry>   EpgEntries;
};

struct AminoPVRRecording
{
    int                 Id;
    int                 ScheduleId;
    time_t              StartTime;
    time_t              EndTime;
    CStdString          Title;
    int                 ChannelId;
    CStdString          ChannelName;
    CStdString          Url;
    CStdString          Filename;
    unsigned int        FileSize;
    bool                Scrambled;
    int                 Marker;
    RecordingStatus     Status;
    int                 EpgProgramId;
    AminoPVREpgEntry    EpgEntry;
};

struct AminoPVRSchedule
{
    int                 Id;
    ScheduleType        Type;
    int                 ChannelId;
    time_t              StartTime;
    time_t              EndTime;
    CStdString          Title;
    bool                PreferHd;
    bool                PreferUnscrambled;
    DuplicationType     DupMethod;
    int                 StartEarly;
    int                 EndLate;
    bool                Inactive;
};

class CCurlFile
{
public:
    CCurlFile( void )  {};
    ~CCurlFile( void ) {};

    bool Get( const CStdString & aUrl, CStdString & aResult );
    bool Post( const CStdString & aUrl, const CStdString & aArguments, CStdString & aResult );
    bool Put( const CStdString & aUrl, const CStdString & aArguments, CStdString & aResult );
    bool Delete( const CStdString & aUrl, CStdString & aResult );
};

class AminoPVRData
{
public:
    AminoPVRData( void );
    virtual ~AminoPVRData( void );

    virtual int       GetChannelsAmount( void);
    virtual PVR_ERROR GetChannels      ( ADDON_HANDLE aHandle, bool aRadio );
    virtual bool      GetChannel       ( const PVR_CHANNEL & aChannel, AminoPVRChannel & aMyChannel );

    virtual PVR_ERROR GetEPGForChannel( ADDON_HANDLE aHandle, const PVR_CHANNEL & aChannel, time_t aStart, time_t aEnd );

    virtual int       GetRecordingsAmount           ( void );
    virtual PVR_ERROR GetRecordings                 ( ADDON_HANDLE aHandle );
    virtual PVR_ERROR DeleteRecording               ( const PVR_RECORDING & aRecording );
    virtual PVR_ERROR SetRecordingLastPlayedPosition( const PVR_RECORDING & aRecording, int aLastPlayedPosition );
    virtual int       GetRecordingLastPlayedPosition( const PVR_RECORDING & aRecording );

	virtual PVR_ERROR GetTimerTypes( PVR_TIMER_TYPE types[], int *size );
    virtual int       GetTimersAmount( void );
    virtual PVR_ERROR GetTimers( ADDON_HANDLE handle );
    virtual PVR_ERROR AddTimer( const PVR_TIMER & timer );
    virtual PVR_ERROR DeleteTimer( const PVR_TIMER & timer, bool bForceDelete );
    virtual PVR_ERROR UpdateTimer( const PVR_TIMER & timer );

protected:
private:
    PVR_ERROR  GetGeneralConfig();

    CStdString ConstructUrl( const CStdString aPath, const CStdString aArguments="", bool aUseApiKey=true );
    bool       GrabAndParse( const CStdString aUrl, Json::Value & aResponse, bool aExpectData=true );
    bool       PostAndParse( const CStdString aUrl, Json::Value aArguments, Json::Value & aResponse, bool aExpectData=true );
    bool       PutAndParse( const CStdString aUrl, Json::Value aArguments, Json::Value & aResponse, bool aExpectData = true );
    bool       DeleteAndParse( const CStdString aUrl, Json::Value & aResponse, bool aExpectData = true );
    bool       ParseResponse( CStdString aJsonString, Json::Value & aJson, bool aExpectData=true );

    void       CreateChannelEntry( Json::Value aJson, AminoPVRChannel & aChannel );
    void       CreateRecordingEntry( Json::Value aJson, AminoPVRRecording & aRecording );
    void       CreateEpgEntry( Json::Value aJson, AminoPVREpgEntry & aEpgEntry );
    void       CreateScheduleEntry( Json::Value aJson, AminoPVRSchedule & aSchedule );
    void       CreateScheduleJson( AminoPVRSchedule aSchedule, Json::Value & aJson );

    void       join( const std::vector<CStdString>& v, char c, CStdString& s );

    // Categories
    Categories                      ivCategories;

    std::vector<AminoPVRChannel>    ivChannels;
    std::vector<AminoPVRRecording>  ivRecordings;
    std::vector<AminoPVRRecording>  ivScheduledRecordings;
    std::vector<AminoPVRSchedule>   ivSchedules;

    int                             ivRtspPort;
    int                             ivTimeslotDelta;
};
