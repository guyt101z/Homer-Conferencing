/*****************************************************************************
 *
 * Copyright (C) 2009 Thomas Volkert <thomas@homer-conferencing.com>
 *
 * This software is free software.
 * Your are allowed to redistribute it and/or modify it under the terms of
 * the GNU General Public License version 2 as published by the Free Software
 * Foundation.
 *
 * This source is published in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License version 2 for more details.
 *
 * You should have received a copy of the GNU General Public License version 2
 * along with this program. Otherwise, you can write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111, USA.
 * Alternatively, you find an online version of the license text under
 * http://www.gnu.org/licenses/gpl-2.0.html.
 *
 *****************************************************************************/

/*
 * Purpose: Implementation for session description protocol
 * Author:  Thomas Volkert
 * Since:   2009-04-14
 */

#include <string>
#include <sstream>

#include <HBSocket.h>
#include <SDP.h>
#include <RTP.h>
#include <Logger.h>

namespace Homer { namespace Conference {

using namespace std;
using namespace Homer::Base;
using namespace Homer::Multimedia;

///////////////////////////////////////////////////////////////////////////////

SDP::SDP()
{
    mVideoCodecsSupport = 0;
    mAudioCodecsSupport = 0;
    mVideoTransportType = MEDIA_TRANSPORT_RTP_UDP;
    mAudioTransportType = MEDIA_TRANSPORT_RTP_UDP;
}

SDP::~SDP()
{
}

///////////////////////////////////////////////////////////////////////////////

void SDP::SetVideoCodecsSupport(int pSelectedCodecs)
{
    mVideoCodecsSupport = pSelectedCodecs;
}

int SDP::GetVideoCodecsSupport()
{
    return mVideoCodecsSupport;
}

void SDP::SetAudioCodecsSupport(int pSelectedCodecs)
{
    mAudioCodecsSupport = pSelectedCodecs;
}

int SDP::GetAudioCodecsSupport()
{
    return mAudioCodecsSupport;
}

enum MediaTransportType SDP::CreateMediaTransportType(TransportType pSocketType, bool pRtp)
{
    switch(pSocketType)
    {
        case SOCKET_UDP:
            return pRtp ? MEDIA_TRANSPORT_RTP_UDP : MEDIA_TRANSPORT_UDP;
        case SOCKET_TCP:
            return pRtp ? MEDIA_TRANSPORT_RTP_TCP : MEDIA_TRANSPORT_TCP;
        case SOCKET_UDP_LITE:
            return pRtp ? MEDIA_TRANSPORT_RTP_UDP_LITE : MEDIA_TRANSPORT_UDP_LITE;
        default:
            break;
    }
    return MEDIA_TRANSPORT_RTP_UDP;
}

enum TransportType SDP::GetSocketTypeFromMediaTransportType(enum MediaTransportType pType)
{
    switch(pType)
    {
        case MEDIA_TRANSPORT_RTP_UDP:
        case MEDIA_TRANSPORT_UDP:
            return SOCKET_UDP;
        case MEDIA_TRANSPORT_RTP_TCP:
        case MEDIA_TRANSPORT_TCP:
            return SOCKET_TCP;
        case MEDIA_TRANSPORT_RTP_UDP_LITE:
        case MEDIA_TRANSPORT_UDP_LITE:
            return SOCKET_UDP_LITE;
        default:
            break;
    }
    return SOCKET_UDP;
}

void SDP::SetVideoTransportType(enum MediaTransportType pType)
{
    LOG(LOG_VERBOSE, "Setting video transport type to: %s", GetMediaTransportStr(pType).c_str());
    mVideoTransportType = pType;
}

void SDP::SetAudioTransportType(enum MediaTransportType pType)
{
    LOG(LOG_VERBOSE, "Setting audio transport type to: %s", GetMediaTransportStr(pType).c_str());
    mAudioTransportType = pType;
}

enum MediaTransportType SDP::GetVideoTransportType()
{
    return mVideoTransportType;
}

enum MediaTransportType SDP::GetAudioTransportType()
{
    return mAudioTransportType;
}

string SDP::GetMediaTransportStr(enum MediaTransportType pType)
{
    string tResult = "";

    //HINT: see http://www.iana.org/assignments/sdp-parameters
    //HINT: see libsofia-sip-ua/sdp/sdp_parse.c
    switch(pType)
    {
        case MEDIA_TRANSPORT_RTP_UDP:
            tResult = "RTP/AVP";
            break;
        case MEDIA_TRANSPORT_RTP_TCP:
            LOG(LOG_ERROR, "Combining RTP and TCP is not supported by sofia-sip, falling back to udp");
            tResult = "tcp"; //ignore and fallback to tcp, correct value would be "RTP/AVP-TCP"
            break;
        case MEDIA_TRANSPORT_RTP_UDP_LITE:
            LOG(LOG_ERROR, "UDP-Lite not supported by sofia-sip, falling back to udp");
            tResult = "udp"; //ignore because this is not supported by sofia-sip, correct value would be "RTP/AVP-FAST"
            break;
        case MEDIA_TRANSPORT_UDP:
            tResult = "udp";
            break;
        case MEDIA_TRANSPORT_TCP:
            tResult = "tcp";
            break;
        case MEDIA_TRANSPORT_UDP_LITE:
            LOG(LOG_ERROR, "UDP-Lite not supported by sofia-sip, falling back to udp");
            tResult = "udp"; //ignore because this is not supported by sofia-sip, correct value would be "udp-lite"
            break;
        default:
            tResult = "unknown";
            break;
    }

    return tResult;
}

/*************************************************
 *  Video codec to name mapping:
 *  ============================
 *        CODEC_H261					h261
 *        CODEC_H263					h263
 *        CODEC_MPEG1VIDEO				mpeg1video
 *        CODEC_MPEG2VIDEO				mpeg2video
 *        CODEC_H263P					h263+
 *        CODEC_H264					h264
 *        CODEC_MPEG4					mpeg4
 *        CODEC_THEORA					theora
 *        CODEC_VP8						vp8
 *
 *
 *  Audio codec to name mapping:
 *  ============================
 *        CODEC_G711ULAW				ulaw
 *        CODEC_GSM						gsm
 *        CODEC_G711ALAW				alaw
 *        CODEC_G722ADPCM				g722
 *        CODEC_PCMS16					pcms16
 *        CODEC_MP3						mp3
 *        CODEC_AAC						aac
 *        CODEC_AMR_NB					amr
 *
 ****************************************************/
string SDP::CreateSdpData(int pAudioPort, int pVideoPort)
{
    string tResult = "";
    unsigned int tSupportedAudioCodecs = GetAudioCodecsSupport();
    unsigned int tSupportedVideoCodecs = GetVideoCodecsSupport();

    LOG(LOG_VERBOSE, "Create SDP packet for audio port: %d and video port: %d", pAudioPort, pVideoPort);
    LOG(LOG_VERBOSE, "Supported audio codecs: %d and video codecs: %d", tSupportedAudioCodecs, tSupportedVideoCodecs);

    // calculate the new audio sdp string
    if (tSupportedAudioCodecs)
    {
        // rest is filled by SIP library
        tResult += "m=audio " + toString(pAudioPort) + " " + GetMediaTransportStr(mAudioTransportType);

        if (tSupportedAudioCodecs & CODEC_G711ULAW)
            tResult += " " + toString(RTP::CodecToPayloadId("ulaw"));
        if (tSupportedAudioCodecs & CODEC_GSM)
            tResult += " " + toString(RTP::CodecToPayloadId("gsm"));
        if (tSupportedAudioCodecs & CODEC_G711ALAW)
            tResult += " " + toString(RTP::CodecToPayloadId("alaw"));
        if (tSupportedAudioCodecs & CODEC_G722ADPCM)
            tResult += " " + toString(RTP::CodecToPayloadId("g722"));
        if (tSupportedAudioCodecs & CODEC_PCMS16)
            tResult += " " + toString(RTP::CodecToPayloadId("pcms16"));
        if (tSupportedAudioCodecs & CODEC_MP3)
            tResult += " " + toString(RTP::CodecToPayloadId("mp3"));
        if (tSupportedAudioCodecs & CODEC_AAC)
            tResult += " " + toString(RTP::CodecToPayloadId("aac"));
        if (tSupportedAudioCodecs & CODEC_AMR_NB)
            tResult += " " + toString(RTP::CodecToPayloadId("amr"));

        tResult += "\r\n";

        if (tSupportedAudioCodecs & CODEC_G711ULAW)
            tResult += "a=rtpmap:" + toString(RTP::CodecToPayloadId("ulaw")) + " PCMU/8000/1\r\n";
        if (tSupportedAudioCodecs & CODEC_GSM)
            tResult += "a=rtpmap:" + toString(RTP::CodecToPayloadId("gsm")) + " GSM/8000/1\r\n";
        if (tSupportedAudioCodecs & CODEC_G711ALAW)
            tResult += "a=rtpmap:" + toString(RTP::CodecToPayloadId("alaw")) + " PCMA/8000/1\r\n";
        if (tSupportedAudioCodecs & CODEC_G722ADPCM)
            tResult += "a=rtpmap:" + toString(RTP::CodecToPayloadId("g722")) + " G722/8000/1\r\n";
        if (tSupportedAudioCodecs & CODEC_PCMS16)
            tResult += "a=rtpmap:" + toString(RTP::CodecToPayloadId("pcms16le")) + " L16/44100/2\r\n";
        if (tSupportedAudioCodecs & CODEC_MP3)
            tResult += "a=rtpmap:" + toString(RTP::CodecToPayloadId("mp3")) + " MPA/8000/1\r\n";
        if (tSupportedAudioCodecs & CODEC_AAC)
            tResult += "a=rtpmap:" + toString(RTP::CodecToPayloadId("aac")) + " AAC/8000/1\r\n";
        if (tSupportedAudioCodecs & CODEC_AMR_NB)
            tResult += "a=rtpmap:" + toString(RTP::CodecToPayloadId("amr")) + " AMR/8000/1\r\n";
    }

    // calculate the new video sdp string
    if (tSupportedVideoCodecs)
    {
        // rest is filled by SIP library
        tResult += "m=video " + toString(pVideoPort) + " "  + GetMediaTransportStr(mVideoTransportType);

        if (tSupportedVideoCodecs & CODEC_H261)
            tResult += " " + toString(RTP::CodecToPayloadId("h261"));
        if (tSupportedVideoCodecs & CODEC_H263)
            tResult += " " + toString(RTP::CodecToPayloadId("h263"));
        if (tSupportedVideoCodecs & CODEC_MPEG1VIDEO)
            tResult += " " + toString(RTP::CodecToPayloadId("mpeg1video"));
        if (tSupportedVideoCodecs & CODEC_MPEG2VIDEO)
            tResult += " " + toString(RTP::CodecToPayloadId("mpeg2video"));
        if (tSupportedVideoCodecs & CODEC_H263P)
            tResult += " " + toString(RTP::CodecToPayloadId("h263+"));
        if (tSupportedVideoCodecs & CODEC_H264)
            tResult += " " + toString(RTP::CodecToPayloadId("h264"));
        if (tSupportedVideoCodecs & CODEC_MPEG4)
            tResult += " " + toString(RTP::CodecToPayloadId("mpeg4"));
        if (tSupportedVideoCodecs & CODEC_THEORA)
            tResult += " " + toString(RTP::CodecToPayloadId("theora"));
        if (tSupportedVideoCodecs & CODEC_VP8)
            tResult += " " + toString(RTP::CodecToPayloadId("vp8"));

        tResult += "\r\n";

        if (tSupportedVideoCodecs & CODEC_H261)
            tResult += "a=rtpmap:" + toString(RTP::CodecToPayloadId("h261")) + " h261/90000\r\n";
        if (tSupportedVideoCodecs & CODEC_H263)
            tResult += "a=rtpmap:" + toString(RTP::CodecToPayloadId("h263")) + " h263/90000\r\n";
        if (tSupportedVideoCodecs & CODEC_MPEG1VIDEO)
            tResult += "a=rtpmap:" + toString(RTP::CodecToPayloadId("mpeg1video")) + " MPV/90000\r\n";
        if (tSupportedVideoCodecs & CODEC_MPEG2VIDEO)
            tResult += "a=rtpmap:" + toString(RTP::CodecToPayloadId("mpeg2video")) + " MPV/90000\r\n";
        if (tSupportedVideoCodecs & CODEC_H263P)
            tResult += "a=rtpmap:" + toString(RTP::CodecToPayloadId("h263+")) + " h263-1998/90000\r\n";
        if (tSupportedVideoCodecs & CODEC_H264)
            tResult += "a=rtpmap:" + toString(RTP::CodecToPayloadId("h264")) + " h264/90000\r\n";
        if (tSupportedVideoCodecs & CODEC_MPEG4)
            tResult += "a=rtpmap:" + toString(RTP::CodecToPayloadId("mpeg4")) + " MP4V-ES/90000\r\n";
        if (tSupportedVideoCodecs & CODEC_THEORA)
            tResult += "a=rtpmap:" + toString(RTP::CodecToPayloadId("theora")) + " theora/90000\r\n";
        if (tSupportedVideoCodecs & CODEC_VP8)
            tResult += "a=rtpmap:" + toString(RTP::CodecToPayloadId("vp8")) + " VP8/90000\r\n";
    }

    //LOG(LOG_VERBOSE, "..result: %s", tResult.c_str());

    return tResult;
}

///////////////////////////////////////////////////////////////////////////////

}} //namespace
