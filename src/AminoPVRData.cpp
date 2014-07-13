/*
 *  This file is part of AminoPVR.
 *  Copyright (C) 2012  Ino Dekker
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "AminoPVRData.h"

using namespace std;
using namespace ADDON;

bool CCurlFile::Get( const CStdString & aUrl, CStdString & aResult )
{
    void * lpFileHandle = XBMC->OpenFile( aUrl.c_str(), 0 );
    if ( lpFileHandle )
    {
        char lpBuffer[1024];
        while ( XBMC->ReadFileString( lpFileHandle, lpBuffer, 1024 ) )
            aResult.append( lpBuffer );
        XBMC->CloseFile( lpFileHandle );
        return true;
    }
    return false;
}

bool CCurlFile::Post( const CStdString & aUrl, const CStdString & aArguments, CStdString & aResult )
{
    void * lpFileHandle = XBMC->OpenFileForWrite( aUrl.c_str(), true );
    if ( lpFileHandle )
    {
        char lpBuffer[1024];
        XBMC->WriteFile( lpFileHandle, aArguments.c_str(), aArguments.length() );
        while ( XBMC->ReadFileString( lpFileHandle, lpBuffer, 1024 ) )
            aResult.append( lpBuffer );
        XBMC->CloseFile( lpFileHandle );
        return true;
    }
    return false;
}

AminoPVRData::AminoPVRData( void )
  : ivCategories()
{
    PVR_ERROR lError;

    ivChannels.clear();
    ivRecordings.clear();
    ivSchedules.clear();
    ivScheduledRecordings.clear();

    lError = GetGeneralConfig();
    if ( lError != PVR_ERROR_NO_ERROR )
    {
        XBMC->Log( LOG_ERROR, "Unable to get General Config" );
    }
}

AminoPVRData::~AminoPVRData(void)
{
    ivChannels.clear();
    ivRecordings.clear();
    ivSchedules.clear();
    ivScheduledRecordings.clear();
}

PVR_ERROR AminoPVRData::GetGeneralConfig()
{
    CStdString lPath;
    lPath.Format( "/api/config/getGeneralConfig" );
    Json::Value lResponse;

    if ( GrabAndParse( ConstructUrl( lPath ), lResponse ) )
    {
        CStdString lTimeslotDelta;
        size_t     lOffset;

        ivRtspPort      = atoi( lResponse["rtsp_server_port"].asString().c_str() );
        lTimeslotDelta  = lResponse["timeslot_delta"].asString();

        ivTimeslotDelta = 0;
        lOffset = lTimeslotDelta.find( 'h', 0 );        // 1h30m30s --> offset = 1
        if ( lOffset != string::npos )
        {
            ivTimeslotDelta += 3600 * atoi( lTimeslotDelta.Left( lOffset ).c_str() );           // 1
            lTimeslotDelta = lTimeslotDelta.Right( lTimeslotDelta.length() - (lOffset + 1) );   // 8 - (1 + 1) = 6
        }
        lOffset = lTimeslotDelta.find( 'm', 0 );        // 30m30s --> offset = 2
        if ( lOffset != string::npos )
        {
            ivTimeslotDelta += 60 * atoi( lTimeslotDelta.Left( lOffset ).c_str() );             // 2
            lTimeslotDelta = lTimeslotDelta.Right( lTimeslotDelta.length() - (lOffset + 1) );   // 6 - (2 + 1) = 3
        }
        lOffset = lTimeslotDelta.find( 's', 0 );        // 30s --> offset = 1
        if ( lOffset != string::npos )
        {
            ivTimeslotDelta += atoi( lTimeslotDelta.Left( lOffset ).c_str() );                  // 2
            lTimeslotDelta = lTimeslotDelta.Right( lTimeslotDelta.length() - (lOffset + 1) );   // 3 - (2 + 1) = 0
        }
    }
    else
    {
        return PVR_ERROR_FAILED;
    }

    return PVR_ERROR_NO_ERROR;
}

int AminoPVRData::GetChannelsAmount( void )
{
    int         lNumChannels = 0;
    Json::Value lResponse;

    if ( GrabAndParse( ConstructUrl( "/api/channels/getNumChannels" ), lResponse ) )
    {
        lNumChannels = lResponse["num_channels"].asInt();
    }

    return lNumChannels;
}

PVR_ERROR AminoPVRData::GetChannels( ADDON_HANDLE aHandle, bool aRadio )
{
    if ( aRadio/* && !g_bRadioEnabled*/ ) return PVR_ERROR_NO_ERROR;

    XBMC->Log( LOG_DEBUG, "%s( radio=%s )", __FUNCTION__, aRadio ? "radio" : "television" );

    Json::Value lResponse;

    if ( GrabAndParse( ConstructUrl( "/api/channels/getChannelList", g_SdOnly ? "includeHd=False" : "" ), lResponse ) )
    {
        int lSize = lResponse.size();

        ivChannels.clear();

        // parse channel list
        for ( int lIndex = 0; lIndex < lSize; ++lIndex )
        {
            AminoPVRChannel lChannel;
            PVR_CHANNEL     lTag;

            CreateChannelEntry( lResponse[lIndex], lChannel );

            memset( &lTag, 0 , sizeof( lTag ) );

            lTag.iUniqueId      = lChannel.Id;
            lTag.iChannelNumber = lChannel.Number;
            lTag.bIsRadio       = lChannel.Radio;
            lTag.bIsHidden      = false;

            strncpy( lTag.strChannelName, lChannel.Name.c_str(),     sizeof( lTag.strChannelName ) );
            strncpy( lTag.strIconPath,    lChannel.LogoPath.c_str(), sizeof( lTag.strIconPath ) );

//            if ( lChannel.Url.compare( 0, 1, "/" ) == 0 )
            {
                strncpy( lTag.strStreamURL, ConstructUrl( lChannel.Url, g_SdOnly ? "includeHd=False" : "" ).c_str(), sizeof( lTag.strStreamURL ) );
            }
//            else
//            {
//                strncpy( lTag.strStreamURL, lChannel.Url.c_str(), sizeof( lTag.strStreamURL ) );
//            }

            if ( !lTag.bIsRadio )
            {
                XBMC->Log( LOG_DEBUG, "Found TV channel: %s, Unique id: %d, Backend channel: %d\n", lChannel.Name.c_str(), lTag.iUniqueId, lTag.iChannelNumber );
            }
            else
            {
                XBMC->Log( LOG_DEBUG, "Found Radio channel: %s, Unique id: %d, Backend channel: %d\n", lChannel.Name.c_str(), lTag.iUniqueId, lTag.iChannelNumber );
            }

            ivChannels.push_back( lChannel );   //Local cache...

            PVR->TransferChannelEntry( aHandle, &lTag );
        }

        return PVR_ERROR_NO_ERROR;
    }

    return PVR_ERROR_SERVER_ERROR;
}

bool AminoPVRData::GetChannel( const PVR_CHANNEL & aChannel, AminoPVRChannel & aMyChannel )
{
    for ( unsigned int lChannelPtr = 0; lChannelPtr < ivChannels.size(); lChannelPtr++ )
    {
        AminoPVRChannel & lThisChannel = ivChannels.at( lChannelPtr );
        if ( lThisChannel.Id == (int)aChannel.iUniqueId )
        {
            aMyChannel.Id           = lThisChannel.Id;
            aMyChannel.EpgId        = lThisChannel.EpgId;
            aMyChannel.Number       = lThisChannel.Number;
            aMyChannel.Name         = lThisChannel.Name;
            aMyChannel.LogoPath     = lThisChannel.LogoPath;
            aMyChannel.Url          = lThisChannel.Url;
            aMyChannel.Radio        = lThisChannel.Radio;
            aMyChannel.EpgEntries   = lThisChannel.EpgEntries;

            return true;
        }
    }

    return false;
}

PVR_ERROR AminoPVRData::GetEPGForChannel( ADDON_HANDLE aHandle, const PVR_CHANNEL & aChannel, time_t aStart, time_t aEnd )
{
    for ( unsigned int lChannelPtr = 0; lChannelPtr < ivChannels.size(); lChannelPtr++ )
    {
        AminoPVRChannel & lChannel = ivChannels.at( lChannelPtr );
        if ( lChannel.Id != (int)aChannel.iUniqueId )
            continue;

        CStdString lPath;
        lPath.Format( "/api/epg/getEpgForChannel/%i/%i/%i", lChannel.Id, aStart, aEnd );
        Json::Value lResponse;

        if ( GrabAndParse( ConstructUrl( lPath ), lResponse ) )
        {
            int lSize = lResponse.size();

            lChannel.EpgEntries.clear();

            // parse channel list
            for ( int lIndex = 0; lIndex < lSize; ++lIndex )
            {
                AminoPVREpgEntry lEpgEntry;
                EPG_TAG          lTag;

                CreateEpgEntry( lResponse[lIndex], lEpgEntry );

                memset( &lTag, 0, sizeof( EPG_TAG ) );

                lTag.iUniqueBroadcastId = lEpgEntry.Id;
                lTag.strTitle           = lEpgEntry.Title.c_str();
                lTag.startTime          = lEpgEntry.StartTime;
                lTag.endTime            = lEpgEntry.EndTime;
                lTag.strEpisodeName     = lEpgEntry.Subtitle.c_str();
                lTag.iChannelNumber     = lChannel.Id;
                lTag.strPlotOutline     = lEpgEntry.Description.c_str();
                lTag.strPlot            = lEpgEntry.Description.c_str();
                lTag.strIconPath        = "";
                if ( lEpgEntry.Genres.size() > 0 )
                {
                    int lCategory      = ivCategories.Category( lEpgEntry.Genres[0] );
                    lTag.iGenreType    = lCategory & 0xF0;
                    lTag.iGenreSubType = lCategory & 0x0F;
                }
                else
                {
                    lTag.iGenreType    = 0;
                    lTag.iGenreSubType = 0;
                }

                PVR->TransferEpgEntry( aHandle, &lTag );
                lChannel.EpgEntries.push_back( lEpgEntry );
            }

            return PVR_ERROR_NO_ERROR;
        }
    }

    return PVR_ERROR_NO_ERROR;
}

int AminoPVRData::GetRecordingsAmount( void )
{
    int         lNumRecordings = 0;
    Json::Value lResponse;
    if ( GrabAndParse( ConstructUrl( "/api/recordings/getNumRecordings" ), lResponse ) )
    {
        lNumRecordings = lResponse["num_recordings"].asInt();
    }

    return lNumRecordings;
}

PVR_ERROR AminoPVRData::GetRecordings( ADDON_HANDLE aHandle )
{
    XBMC->Log( LOG_DEBUG, "%s()", __FUNCTION__ );

    Json::Value lResponse;
    if ( GrabAndParse( ConstructUrl( "/api/recordings/getRecordingList" ), lResponse ) )
    {
        int lSize = lResponse.size();

        ivRecordings.clear();

        for ( int lIndex = 0; lIndex < lSize; ++lIndex )
        {
            AminoPVRRecording lRecording;
            PVR_RECORDING     lTag;

            CreateRecordingEntry( lResponse[lIndex], lRecording );

            memset( &lTag, 0, sizeof( lTag ) );

            lTag.recordingTime  = lRecording.StartTime;
            lTag.iDuration      = lRecording.EndTime - lRecording.StartTime;

            sprintf( lTag.strRecordingId, "%d", lRecording.Id );

            if ( (lRecording.EpgEntry.Title.length()) && (lRecording.EpgEntry.Subtitle.length() > 0) )
            {
                CStdString lTitle;
                lTitle.Format( "%s: %s",    lRecording.EpgEntry.Title.c_str(),          lRecording.EpgEntry.Subtitle.c_str() );
                strncpy( lTag.strTitle,     lTitle.c_str(),                             sizeof( lTag.strTitle ) );
                strncpy( lTag.strDirectory, lRecording.EpgEntry.Title.c_str(),          sizeof( lTag.strDirectory ) );
            }
            else if ( lRecording.EpgEntry.Title.length() > 0 )
            {
                strncpy( lTag.strTitle,     lRecording.EpgEntry.Title.c_str(),          sizeof( lTag.strTitle ) );
                strncpy( lTag.strDirectory, lRecording.EpgEntry.Title.c_str(),          sizeof( lTag.strDirectory ) );
            }
            else
            {
                strncpy( lTag.strTitle,     lRecording.Title.c_str(),                   sizeof( lTag.strTitle ) );
                strncpy( lTag.strDirectory, lRecording.Title.c_str(),                   sizeof( lTag.strDirectory ) );
            }

            strncpy( lTag.strPlot,          lRecording.EpgEntry.Description.c_str(),    sizeof( lTag.strPlot ) );
            strncpy( lTag.strPlotOutline,   lRecording.EpgEntry.Description.c_str(),    sizeof( lTag.strPlotOutline ) );
            strncpy( lTag.strChannelName,   lRecording.ChannelName.c_str(),             sizeof( lTag.strChannelName ) );

            if ( g_UseHttpStreams )
            {
                strncpy( lTag.strStreamURL, ConstructUrl( lRecording.Url ).c_str(),     sizeof( lTag.strStreamURL ) );
            }
            else
            {
                sprintf( lTag.strStreamURL, "rtsp://%s:%d/%s", g_strHostname.c_str(), ivRtspPort, lRecording.Filename.c_str() );
            }

            XBMC->Log( LOG_DEBUG, "Found recording: %s, Unique id: %s, url: %s\n", lRecording.Title.c_str(), lTag.strRecordingId, lTag.strStreamURL );

            ivRecordings.push_back( lRecording );   //Local cache...

            PVR->TransferRecordingEntry( aHandle, &lTag );
        }

        return PVR_ERROR_NO_ERROR;
    }

    return PVR_ERROR_SERVER_ERROR;
}

PVR_ERROR AminoPVRData::DeleteRecording( const PVR_RECORDING & aRecording )
{
    int lRecordingId = -1;
    XBMC->Log( LOG_DEBUG, "%s( %s )", __FUNCTION__, aRecording.strRecordingId );
    if ( sscanf( aRecording.strRecordingId, "%i", &lRecordingId ) == 1 )
    {
        for ( unsigned int lRecordingPtr = 0; lRecordingPtr < ivRecordings.size(); lRecordingPtr++ )
        {
            AminoPVRRecording & lRecording = ivRecordings.at( lRecordingPtr );
            if ( lRecording.Id != lRecordingId )
                continue;

            CStdString lPath;
            lPath.Format( "/api/recordings/deleteRecording/%i", lRecordingId );
            Json::Value lResponse;
            if ( GrabAndParse( ConstructUrl( lPath ), lResponse, false ) )
            {
                ivRecordings.erase( ivRecordings.begin() + lRecordingPtr );
                return PVR_ERROR_NO_ERROR;
            }
        }
    }

    XBMC->Log( LOG_ERROR, "%s: could not find recording.\n", __FUNCTION__ );

    return PVR_ERROR_SERVER_ERROR;
}

PVR_ERROR AminoPVRData::SetRecordingLastPlayedPosition( const PVR_RECORDING & aRecording, int aLastPlayedPosition )
{
    int lRecordingId = -1;
    XBMC->Log( LOG_NOTICE, "%s( %s, %d )", __FUNCTION__, aRecording.strRecordingId, aLastPlayedPosition );
    if ( sscanf( aRecording.strRecordingId, "%i", &lRecordingId ) == 1 )
    {
        for ( unsigned int lRecordingPtr = 0; lRecordingPtr < ivRecordings.size(); lRecordingPtr++ )
        {
            AminoPVRRecording & lRecording = ivRecordings.at( lRecordingPtr );
            if ( lRecording.Id != lRecordingId )
                continue;

            lRecording.Marker = aLastPlayedPosition;

            CStdString lPath;
            lPath.Format( "/api/recordings/setRecordingMarker/%i/%i", lRecordingId, aLastPlayedPosition );
            Json::Value lResponse;

            if ( GrabAndParse( ConstructUrl( lPath ), lResponse, false ) )
            {
                return PVR_ERROR_NO_ERROR;
            }
        }
    }

    XBMC->Log( LOG_ERROR, "%s: could not find recording.\n", __FUNCTION__ );

    return PVR_ERROR_SERVER_ERROR;
}

int AminoPVRData::GetRecordingLastPlayedPosition( const PVR_RECORDING & aRecording )
{
    int lRecordingId = -1;
    XBMC->Log( LOG_NOTICE, "%s( %s )", __FUNCTION__, aRecording.strRecordingId );
    if ( sscanf( aRecording.strRecordingId, "%i", &lRecordingId ) == 1 )
    {
        for ( unsigned int lRecordingPtr = 0; lRecordingPtr < ivRecordings.size(); lRecordingPtr++ )
        {
            AminoPVRRecording & lRecording = ivRecordings.at( lRecordingPtr );
            if ( lRecording.Id != lRecordingId )
                continue;

            return lRecording.Marker;
        }
    }

    XBMC->Log( LOG_ERROR, "%s: could not find recording.\n", __FUNCTION__ );

    return PVR_ERROR_SERVER_ERROR;
}

int AminoPVRData::GetTimersAmount( void )
{
    int         lNumRecordings = 0;
    Json::Value lResponse;
    XBMC->Log( LOG_DEBUG, "%s()", __FUNCTION__ );
    if ( GrabAndParse( ConstructUrl( "/api/schedules/getNumScheduledRecordings" ), lResponse ) )
    {
        lNumRecordings = lResponse["num_recordings"].asInt();
    }

    return lNumRecordings;
}

PVR_ERROR AminoPVRData::GetTimers( ADDON_HANDLE aHandle )
{
    XBMC->Log( LOG_DEBUG, "%s()", __FUNCTION__ );

    Json::Value lResponse;
    if ( GrabAndParse( ConstructUrl( "/api/schedules/getScheduleList" ), lResponse ) )
    {
        int lSize = lResponse.size();

        ivSchedules.clear();

        for ( int lIndex = 0; lIndex < lSize; ++lIndex )
        {
            AminoPVRSchedule lSchedule;

            CreateScheduleEntry( lResponse[lIndex], lSchedule );

            XBMC->Log( LOG_DEBUG, "Found schedule: %s, Unique id: %d\n", lSchedule.Title.c_str(), lSchedule.Id );

            ivSchedules.push_back( lSchedule );   //Local cache...
        }

        if ( GrabAndParse( ConstructUrl( "/api/schedules/getScheduledRecordingList" ), lResponse ) )
        {
            int lSize = lResponse.size();

            ivScheduledRecordings.clear();

            for ( int lIndex = 0; lIndex < lSize; ++lIndex )
            {
                AminoPVRRecording lRecording;
                PVR_TIMER         lTag;

                CreateRecordingEntry( lResponse[lIndex], lRecording );

    //            PVR_TIMER_STATE_NEW          = 0, /*!< @brief a new, unsaved timer */
    //PVR_TIMER_STATE_SCHEDULED    = 1, /*!< @brief the timer is scheduled for recording */
    //PVR_TIMER_STATE_RECORDING    = 2, /*!< @brief the timer is currently recordings */
    //PVR_TIMER_STATE_COMPLETED    = 3, /*!< @brief the recording completed successfully */
    //PVR_TIMER_STATE_ABORTED      = 4, /*!< @brief recording started, but was aborted */
    //PVR_TIMER_STATE_CANCELLED    = 5, /*!< @brief the timer was scheduled, but was canceled */
    //PVR_TIMER_STATE_CONFLICT_OK  = 6, /*!< @brief the scheduled timer conflicts with another one, but will be recorded */
    //PVR_TIMER_STATE_CONFLICT_NOK = 7, /*!< @brief the scheduled timer conflicts with another one and won't be recorded */
    //PVR_TIMER_STATE_ERROR        = 8  /*!< @brief the timer is scheduled, but can't be recorded for some reason */

                for ( unsigned int lSchedulePtr = 0; lSchedulePtr < ivSchedules.size(); lSchedulePtr++ )
                {
                    AminoPVRSchedule & lSchedule = ivSchedules.at( lSchedulePtr );
                    if ( lSchedule.Id != lRecording.ScheduleId )
                        continue;
                
                    memset( &lTag, 0, sizeof( lTag ) );

                    lTag.iClientIndex       = lRecording.Id;                // This is timerId, not actual recordingId
                    lTag.iClientChannelUid  = lRecording.ChannelId;
                    lTag.startTime          = lRecording.StartTime;
                    lTag.endTime            = lRecording.EndTime;

                    switch ( lRecording.Status )
                    {
                        case REC_STATUS_UNKNOWN:
                            lTag.state = PVR_TIMER_STATE_SCHEDULED;
                            break;
                        case REC_STATUS_START_RECORDING:
                        case REC_STATUS_RECORDING_STARTED:
                        case REC_STATUS_STOP_RECORDING:
                            lTag.state = PVR_TIMER_STATE_RECORDING;
                            break;
                        case REC_STATUS_RECORDING_FINISHED:
                            lTag.state = PVR_TIMER_STATE_COMPLETED;
                            break;
                        case REC_STATUS_RECORDING_UNFINISHED:
                            lTag.state = PVR_TIMER_STATE_ABORTED;
                            break;
                        default:
                            lTag.state = PVR_TIMER_STATE_ERROR;
                            break;
                    }

                    if ( (lRecording.EpgEntry.Title.length() > 0) && (lRecording.EpgEntry.Subtitle.length() > 0) )
                    {
                        CStdString lTitle;
                        lTitle.Format( "%s: %s",    lRecording.EpgEntry.Title.c_str(),          lRecording.EpgEntry.Subtitle.c_str() );
                        strncpy( lTag.strTitle,     lTitle.c_str(),                             sizeof( lTag.strTitle ) );
                        strncpy( lTag.strDirectory, lRecording.EpgEntry.Title.c_str(),          sizeof( lTag.strDirectory ) );
                    }
                    else if ( lRecording.EpgEntry.Title.length() > 0 )
                    {
                        strncpy( lTag.strTitle,     lRecording.EpgEntry.Title.c_str(),          sizeof( lTag.strTitle ) );
                        strncpy( lTag.strDirectory, lRecording.EpgEntry.Title.c_str(),          sizeof( lTag.strDirectory ) );
                    }
                    else
                    {
                        strncpy( lTag.strTitle,     lRecording.Title.c_str(),                   sizeof( lTag.strTitle ) );
                        strncpy( lTag.strDirectory, lRecording.Title.c_str(),                   sizeof( lTag.strDirectory ) );
                    }

                    strncpy( lTag.strSummary,       lRecording.EpgEntry.Description.c_str(),    sizeof( lTag.strSummary ) );

                    lTag.iPriority      = 0;
                    lTag.iLifetime      = -1;

                    switch ( lSchedule.Type )
                    {
                        case SCHEDULE_TYPE_TIMESLOT_EVERY_DAY:
                        case SCHEDULE_TYPE_ONCE_EVERY_DAY:
                        case SCHEDULE_TYPE_ANY_TIME:
                        case SCHEDULE_TYPE_MANUAL_EVERY_DAY:
                            lTag.bIsRepeating   = true;
                            lTag.iWeekdays      = 0x7F; // 0111 1111
                            break;
                        case SCHEDULE_TYPE_TIMESLOT_EVERY_WEEK:
                        case SCHEDULE_TYPE_ONCE_EVERY_WEEK:
                        case SCHEDULE_TYPE_MANUAL_EVERY_WEEK:
                            {
                                struct tm lTimeInfo = *localtime( &lSchedule.StartTime );
                                int       lWeekDay  = lTimeInfo.tm_wday;    // days since Sunday [0-6]

                                // bit 0 = monday, need to convert weekday value to bitnumber:
                                if ( lWeekDay == 0 )
                                    lWeekDay = 6;   // sunday 0100 0000
                                else
                                    lWeekDay--;

                                lTag.bIsRepeating   = true;
                                lTag.iWeekdays      = 1 << lWeekDay;
                            }
                            break;
                        case SCHEDULE_TYPE_MANUAL_EVERY_WEEKDAY:
                            lTag.bIsRepeating   = true;
                            lTag.iWeekdays      = 31; // 0001 1111
                            break;
                        case SCHEDULE_TYPE_MANUAL_EVERY_WEEKEND:
                            lTag.bIsRepeating   = true;
                            lTag.iWeekdays      = 96; // 0110 0000
                            break;
                        case SCHEDULE_TYPE_ONCE:
                        default:
                            lTag.bIsRepeating   = false;
                            break;
                    }

                    lTag.firstDay       = lSchedule.StartTime;
                    lTag.iEpgUid        = lRecording.EpgProgramId;
                    lTag.iMarginStart   = lSchedule.StartEarly;
                    lTag.iMarginEnd     = lSchedule.EndLate;
                    lTag.iGenreType     = 0;
                    lTag.iGenreSubType  = 0;

                    XBMC->Log( LOG_DEBUG, "Found scheduled recording: %s, Unique id: %d\n", lRecording.Title.c_str(), lTag.iClientIndex );

                    ivScheduledRecordings.push_back( lRecording );   //Local cache...

                    PVR->TransferTimerEntry( aHandle, &lTag );

                    break;
                }
            }

            return PVR_ERROR_NO_ERROR;
        }
    }

    return PVR_ERROR_SERVER_ERROR;
}

PVR_ERROR AminoPVRData::AddTimer( const PVR_TIMER & aTimer )
{
    XBMC->Log( LOG_DEBUG, "%s( %d )", __FUNCTION__, aTimer.iClientIndex );

    AminoPVRSchedule lSchedule;

    lSchedule.Id            = -1;
    lSchedule.Type          = SCHEDULE_TYPE_ONCE;

    // TODO: If no program exists from startime to endtime, then it is a manual recording
    // TODO: Check value of iWeekdays and bIsRepeating
    if ( aTimer.bIsRepeating )
    {
        switch ( aTimer.iWeekdays )
        {
            case 0x40:  // Sunday
            case 0x20:  // Saturday
            case 0x10:  // Friday
            case 0x08:  // Thursday
            case 0x04:  // Wednesday
            case 0x02:  // Tuesday
            case 0x01:  // Monday
                lSchedule.Type = SCHEDULE_TYPE_ONCE_EVERY_WEEK;
                break;
            case 0x1F:  // Weekdays
                lSchedule.Type = SCHEDULE_TYPE_MANUAL_EVERY_WEEKDAY;
                break;
            case 0x60:  // Weekends
                lSchedule.Type = SCHEDULE_TYPE_MANUAL_EVERY_WEEKEND;
                break;
            case 0x7F:
                lSchedule.Type = SCHEDULE_TYPE_ONCE_EVERY_DAY;
                break;
            default:
                lSchedule.Type = SCHEDULE_TYPE_ANY_TIME;
                break;
        }
    }

    lSchedule.Title.Format( "%s", aTimer.strTitle );
    lSchedule.ChannelId         = aTimer.iClientChannelUid;
    lSchedule.StartTime         = aTimer.startTime;
    lSchedule.EndTime           = aTimer.endTime;
    lSchedule.StartEarly        = aTimer.iMarginStart;
    lSchedule.EndLate           = aTimer.iMarginEnd;
    lSchedule.PreferHd          = true;
    lSchedule.PreferUnscrambled = true;
    lSchedule.DupMethod         = (DuplicationType)(DUPLICATION_METHOD_TITLE | DUPLICATION_METHOD_SUBTITLE | DUPLICATION_METHOD_DESCRIPTION);
    lSchedule.Inactive          = false;

    Json::Value         lJson;
    Json::FastWriter    lWriter;
    Json::Value         lResponse;
    Json::Value         lArguments;

    CreateScheduleJson( lSchedule, lJson );

    lArguments["schedule"] = lWriter.write( lJson );

    if ( PostAndParse( ConstructUrl( "/api/schedules/addSchedule" ), lArguments, lResponse, false ) )
    {
        return PVR_ERROR_NO_ERROR;
    }

    return PVR_ERROR_SERVER_ERROR;
}

PVR_ERROR AminoPVRData::DeleteTimer( const PVR_TIMER & aTimer, bool aForceDelete )
{
    int lScheduleId = -1;
    XBMC->Log( LOG_DEBUG, "%s( %d, %d )", __FUNCTION__, aTimer.iClientIndex, aForceDelete );
    for ( unsigned int lSchedulePtr = 0; lSchedulePtr < ivSchedules.size(); lSchedulePtr++ )
    {
        AminoPVRSchedule & lSchedule = ivSchedules.at( lSchedulePtr );
        if ( lSchedule.Id != lScheduleId )
            continue;

        CStdString lPath;
        lPath.Format( "/api/schedules/deleteSchedules/%i", lScheduleId );
        Json::Value lResponse;
        if ( GrabAndParse( ConstructUrl( lPath ), lResponse, false ) )
        {
            ivSchedules.erase( ivSchedules.begin() + lSchedulePtr );
            return PVR_ERROR_NO_ERROR;
        }
    }

    XBMC->Log( LOG_ERROR, "%s: could not find recording.\n", __FUNCTION__ );

    return PVR_ERROR_SERVER_ERROR;
}

PVR_ERROR AminoPVRData::UpdateTimer( const PVR_TIMER & aTimer )
{
    XBMC->Log( LOG_DEBUG, "%s( %d )", __FUNCTION__, aTimer.iClientIndex );
    return PVR_ERROR_SERVER_ERROR;
}

CStdString AminoPVRData::ConstructUrl( const CStdString aPath, const CStdString aArguments, bool aUseApiKey )
{
    CStdString lUrl;

    lUrl.Format( "http://%s:%i%s", g_strHostname.c_str(), g_iPort, aPath.c_str() );

    if ( aUseApiKey )
    {
        lUrl.AppendFormat( "?apiKey=%s", g_szApiKey.c_str() );
    }
    if ( aArguments != "" )
    {
        if ( aUseApiKey )
        {
            lUrl.AppendFormat( "&%s", aArguments.c_str() );
        }
        else
        {
            lUrl.AppendFormat( "?%s", aArguments.c_str() );
        }
    }


    return lUrl;
}

bool AminoPVRData::GrabAndParse( const CStdString aUrl, Json::Value & aResponse, bool aExpectData )
{
    bool       lResult = false;
    CStdString lJson;
    CCurlFile  lHttp;
    if ( !lHttp.Get( aUrl, lJson ) )
    {
        XBMC->Log( LOG_ERROR, "%s: Could not open connection to AminoPVR backend: aUrl=%s\n", __FUNCTION__, aUrl.c_str() );
    }
    else
    {
        if ( ParseResponse( lJson, aResponse, aExpectData ) )
        {
            lResult = true;
        }
        else
        {
            XBMC->Log( LOG_ERROR, "%s: failed: aUrl=%s, lJson=%s\n", __FUNCTION__, aUrl.c_str(), lJson.c_str() );
        }
    }

    return lResult;
}

bool AminoPVRData::PostAndParse( const CStdString aUrl, Json::Value aArguments, Json::Value & aResponse, bool aExpectData )
{
    bool                lResult = false;
    Json::FastWriter    lWriter;
    CStdString          lJson;
    CCurlFile           lHttp;

    if ( !lHttp.Post( aUrl, CStdString( lWriter.write( aArguments ) ), lJson ) )
    {
        XBMC->Log( LOG_ERROR, "%s: Could not open connection to AminoPVR backend: aUrl=%s, aArguments=%s\n", __FUNCTION__, aUrl.c_str(), aArguments.toStyledString().c_str() );
    }
    else
    {
        if ( ParseResponse( lJson, aResponse, aExpectData ) )
        {
            lResult = true;
        }
        else
        {
            XBMC->Log( LOG_ERROR, "%s: failed: aUrl=%s, lJson=%s\n", __FUNCTION__, aUrl.c_str(), lJson.c_str() );
        }
    }

    return lResult;
}

bool AminoPVRData::ParseResponse( CStdString aJsonString, Json::Value & aJson, bool aExpectData )
{
    Json::Reader lReader;
    Json::Value  lResponse;
    bool         lParsingSuccessful = false;

    lParsingSuccessful = lReader.parse( aJsonString, lResponse );
    if ( lParsingSuccessful )
    {
        if ( lResponse["status"].asString().compare( "success" ) != 0 )
        {
            XBMC->Log( LOG_ERROR, "%s: No successful response status: %s\n", __FUNCTION__, lResponse["status"].asString().c_str() );
            lParsingSuccessful = false;
        }
    }
    else
    {
        XBMC->Log( LOG_ERROR, "%s: Failed to parse '%s'\nError message: %s\n", __FUNCTION__, aJsonString.c_str(), lReader.getFormatedErrorMessages().c_str() );
    }

    if ( aExpectData && lParsingSuccessful )
    {
        if ( !lResponse.isMember( "data" ) )
        {
            XBMC->Log( LOG_ERROR, "%s: Data expected, but not available.\n", __FUNCTION__ );
            lParsingSuccessful = false;
        }
        else
        {
            aJson = lResponse["data"];
        }
    }

    return lParsingSuccessful;
}

void AminoPVRData::CreateEpgEntry( Json::Value aJson, AminoPVREpgEntry & aEpgEntry )
{
    aEpgEntry.EpgId             = aJson["epg_id"].asString();
    aEpgEntry.Id                = aJson["id"].asInt();
    aEpgEntry.Title             = aJson["title"].asString();
    aEpgEntry.StartTime         = aJson["start_time"].asInt();
    aEpgEntry.EndTime           = aJson["end_time"].asInt();
    aEpgEntry.Subtitle          = aJson["subtitle"].asString();
    aEpgEntry.Description       = aJson["description"].asString();
    aEpgEntry.Genres.clear();
    aEpgEntry.Actors.clear();
    aEpgEntry.Directors.clear();
    aEpgEntry.Presenters.clear();
    aEpgEntry.AspectRatio       = aJson["aspect_ratio"].asString();
    aEpgEntry.ParentalRating    = aJson["parental_rating"].asString();

    // TODO: process all genres
    Json::Value lGenres = aJson["genre"];
    if ( lGenres.size() > 0 )
    {
        aEpgEntry.Genres.push_back( lGenres[(Json::Value::UInt)0].asString() );
    }
    Json::Value lActors = aJson["actors"];
    for ( int i = 0; i < (int)lActors.size(); i++ )
    {
        aEpgEntry.Actors.push_back( lActors[i].asString() );
    }
    Json::Value lDirectors = aJson["directors"];
    for ( int i = 0; i < (int)lDirectors.size(); i++ )
    {
        aEpgEntry.Directors.push_back( lDirectors[i].asString() );
    }
    Json::Value lPresenters = aJson["presenters"];
    for ( int i = 0; i < (int)lPresenters.size(); i++ )
    {
        aEpgEntry.Presenters.push_back( lPresenters[i].asString() );
    }
    Json::Value lRatings = aJson["ratings"];
    for ( int i = 0; i < (int)lRatings.size(); i++ )
    {
        aEpgEntry.Ratings.push_back( lRatings[i].asString() );
    }
}

void AminoPVRData::CreateChannelEntry( Json::Value aJson, AminoPVRChannel & aChannel )
{
    aChannel.Id         = aJson["id"].asInt();
    aChannel.EpgId      = aJson["epg_id"].asString();
    aChannel.Number     = aJson["number"].asInt();
    aChannel.Name       = aJson["name"].asString();
    aChannel.Url        = aJson["url"].asString();
    aChannel.LogoPath   = ConstructUrl( aJson["logo_path"].asString() );
    aChannel.Radio      = false;
    aChannel.EpgEntries.clear();
}

void AminoPVRData::CreateRecordingEntry( Json::Value aJson, AminoPVRRecording & aRecording )
{
    aRecording.Id           = aJson["id"].asInt();
    aRecording.ScheduleId   = aJson["schedule_id"].asInt();
    aRecording.StartTime    = aJson["start_time"].asUInt();
    aRecording.EndTime      = aJson["end_time"].asUInt();
    aRecording.Title        = aJson["title"].asString();
    aRecording.ChannelId    = aJson["channel_id"].asInt();
    aRecording.ChannelName  = aJson["channel_name"].asString();
    aRecording.Url          = aJson["url"].asString();
    aRecording.Filename     = aJson["filename"].asString();
    aRecording.FileSize     = aJson["file_size"].asUInt();
    aRecording.Scrambled    = aJson["scrambled"].asBool();
    aRecording.Marker       = aJson["marker"].asInt();
    aRecording.Status       = (RecordingStatus)aJson["status"].asInt();
    aRecording.EpgProgramId = aJson["epg_program_id"].asInt();

    if ( aRecording.EpgProgramId != -1 )
    {
        CreateEpgEntry( aJson["epg_program"], aRecording.EpgEntry );
    }
    else
    {
        aRecording.EpgEntry.Title = aJson["title"].asString();
    }
}

void AminoPVRData::CreateScheduleEntry( Json::Value aJson, AminoPVRSchedule & aSchedule )
{
    aSchedule.Id                   = aJson["id"].asInt();
    aSchedule.Type                 = (ScheduleType)aJson["type"].asInt();
    aSchedule.ChannelId            = aJson["channel_id"].asInt();
    aSchedule.StartTime            = aJson["start_time"].asInt();
    aSchedule.EndTime              = aJson["end_time"].asInt();
    aSchedule.Title                = aJson["title"].asString();
    aSchedule.PreferHd             = aJson["prefer_hd"].asBool();
    aSchedule.PreferUnscrambled    = aJson["prefer_unscrambled"].asBool();
    aSchedule.DupMethod            = (DuplicationType)aJson["dup_method"].asInt();
    aSchedule.StartEarly           = aJson["start_early"].asInt();
    aSchedule.EndLate              = aJson["end_late"].asInt();
    aSchedule.Inactive             = aJson["inactive"].asBool();
}

void AminoPVRData::CreateScheduleJson( AminoPVRSchedule aSchedule, Json::Value & aJson )
{
    aJson["id"]                 = Json::Value( aSchedule.Id );
    aJson["type"]               = Json::Value( aSchedule.Type );
    aJson["channel_id"]         = Json::Value( aSchedule.ChannelId );
    aJson["start_time"]         = Json::Value( (int)aSchedule.StartTime );
    aJson["end_time"]           = Json::Value( (int)aSchedule.EndTime );
    aJson["title"]              = Json::Value( aSchedule.Title.c_str() );
    aJson["prefer_hd"]          = Json::Value( aSchedule.PreferHd );
    aJson["prefer_unscrambled"] = Json::Value( aSchedule.PreferUnscrambled );
    aJson["dup_method"]         = Json::Value( aSchedule.DupMethod );
    aJson["start_early"]        = Json::Value( aSchedule.StartEarly );
    aJson["end_late"]           = Json::Value( aSchedule.EndLate );
    aJson["inactive"]           = Json::Value( aSchedule.Inactive );
}
