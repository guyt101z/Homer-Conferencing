/*****************************************************************************
 *
 * Copyright (C) 2011 Thomas Volkert <thomas@homer-conferencing.com>
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
 * Purpose: ffmpeg based memory video source
 * Author:  Thomas Volkert
 * Since:   2011-05-05
 */

#ifndef _MULTIMEDIA_MEDIA_SOURCE_MEM_
#define _MULTIMEDIA_MEDIA_SOURCE_MEM_

#include <Header_Ffmpeg.h>
#include <MediaFifo.h>
#include <MediaSource.h>
#include <RTP.h>
#include <VideoScaler.h>

#include <HBThread.h>
#include <HBCondition.h>
#include <HBSystem.h>

#include <string>

using namespace Homer::Base;

namespace Homer { namespace Multimedia {

///////////////////////////////////////////////////////////////////////////////

// the following de/activates debugging of received packets
//#define MSMEM_DEBUG_PACKET_RECEIVER
//#define MSMEM_DEBUG_AUDIO_FRAME_RECEIVER
//#define MSMEM_DEBUG_VIDEO_FRAME_RECEIVER
//#define MSMEM_DEBUG_PACKETS

// de/activate output of RTCP sender reports
//#define MSMEM_DEBUG_SENDER_REPORTS

//#define MSMEM_DEBUG_SEEKING
#define MSMEM_DEBUG_CALIBRATION
//#define MSMEM_DEBUG_TIMING
//#define MSMEM_DEBUG_DECODER_STATE
//#define MSMEM_DEBUG_PRE_BUFFERING


//#define MSMEM_DEBUG_AV_SYNC

///////////////////////////////////////////////////////////////////////////////

// size of one single fragment of a frame packet
#define MEDIA_SOURCE_MEM_FRAGMENT_BUFFER_SIZE                8*1024 // 8 KB (for jumbo packets!)

#define MEDIA_SOURCE_MEM_FRAGMENT_INPUT_QUEUE_SIZE_LIMIT 	 ((System::GetTargetMachineType() != "x86") ? 2048 : 256) // � 8 KB: 256 buffers for 32 bit targets with limit of 4 GB ram, 2*1024 buffers for 64 bit targets

///////////////////////////////////////////////////////////////////////////////

struct MediaInputQueueEntry
{
	char	*Data;
	int		Size;
};

///////////////////////////////////////////////////////////////////////////////

class MediaSourceMem :
    public MediaSource, public RTP, public Thread
{
public:
    /// The default constructor
    MediaSourceMem(string pName = "SOURCE:", bool pRtpActivated = true);
    /// The destructor.
    virtual ~MediaSourceMem();

    /* grabbing control */
    virtual void StopGrabbing();
    virtual int GetChunkDropCounter();
    virtual int GetFragmentBufferCounter();
    virtual int GetFragmentBufferSize();

    virtual int CalculateFrameBufferSize(); // calculates a good value for frame queue
    virtual int GetFrameBufferCounter(); // returns the currently used number of entries in the frame queue
    virtual int GetFrameBufferSize(); // returns current frame queue size

    /* frame stats */
    virtual bool SupportsDecoderFrameStatistics();

    /* A/V sync. */
    virtual int64_t GetSynchronizationTimestamp(); // in us
    virtual bool TimeShift(int64_t pOffset); // in us

    /* video grabbing control */
    virtual GrabResolutions GetSupportedVideoGrabResolutions();

    /* fps */
    virtual void SetFrameRate(float pFps);

    /* recording */
    virtual bool SupportsRecording();

    /* relaying */
    virtual bool SupportsRelaying();

    /* multi stream input interface */
    virtual bool HasInputStreamChanged();

    virtual bool SetInputStreamPreferences(std::string pStreamCodec, bool pDoReset = false);

    virtual bool OpenVideoGrabDevice(int pResX = 352, int pResY = 288, float pFps = 29.97);
    virtual bool OpenAudioGrabDevice(int pSampleRate = 44100, int pChannels = 2);
    virtual bool CloseGrabDevice();
    virtual int GrabChunk(void* pChunkBuffer, int& pChunkSize, bool pDropChunk = false);

    // send input to the media source
    void WriteFragment(char *pBuffer, int pBufferSize);

protected:
    /* internal video resolution switch */
    virtual void DoSetVideoGrabResolution(int pResX = 352, int pResY = 288);

    static int GetNextPacket(void *pOpaque, uint8_t *pBuffer, int pBufferSize);
    void ReadFragment(char *pData, int &pDataSize);

    virtual bool InputIsPicture();

    /* decoder thread */
    virtual void StartDecoder();
    virtual void StopDecoder();
    virtual void* Run(void* pArgs = NULL); // decoder main loop
    VideoScaler *CreateVideoScaler();
    void DestroyVideoScaler(VideoScaler *pScaler);

    /* buffering */
    void UpdateBufferTime();

    /* FIFO helpers */
    void WriteFrameOutputBuffer(char* pBuffer, int pBufferSize, int64_t pPts);
    void ReadFrameOutputBuffer(char *pBuffer, int &pBufferSize, int64_t &pPts);
    bool DecoderFifoFull();

    /* RTP based frame numbering */
    virtual uint64_t CalculateFrameNumberFromRTP();

    /* real-time playback */
    virtual void CalibrateRTGrabbing();
    virtual void WaitForRTGrabbing();

    unsigned long       mFragmentNumber;
    char                *mStreamPacketBuffer;
    char                *mFragmentBuffer;
    int					mResXLastGrabbedFrame, mResYLastGrabbedFrame;
    bool                mRtpActivated;
    bool                mOpenInputStream;
    int                 mWrappingHeaderSize;
    int                 mPacketStatAdditionalFragmentSize; // used to adapt packet statistic to additional fragment header, which is used for TCP transmission
    /* grabber */
    double              mGrabberCurrentFrameIndex; // we have to determine this manually during grabbing because cur_dts and everything else in AVStream is buggy for some video/audio files
    double              mFrameBufferLastWrittenFrameIndex; // we use this for calibrating RT grabbing
    bool                mGrabberProvidesRTGrabbing;
    /* decoder thread */
    bool                mDecoderUsesPTSFromInputPackets;
    int                 mDecoderTargetResX;
    int                 mDecoderTargetResY;
    bool                mDecoderNeeded; // also used to signal that the decoder thread has finished the init. process
    int64_t             mDecoderLastReadPts; // check for interleaved packets, non-monotonous PTS values
    Condition           mDecoderNeedWorkCondition;
    Mutex               mDecoderNeedWorkConditionMutex;
    MediaFifo           *mDecoderFragmentFifo;
    Mutex				mDecoderFragmentFifoDestructionMutex;
    MediaFifo           *mDecoderFifo; // for frames
    MediaFifo           *mDecoderMetaDataFifo; // for meta data about frames
    /* decoder thread seeking */
    double              mDecoderTargetFrameIndex;
    bool                mDecoderWaitForNextKeyFramePackets; // after seeking we wait for next key frame packets -> either i-frames or p-frames
    bool                mDecoderRecalibrateRTGrabbingAfterSeeking;
    bool                mDecoderFlushBuffersAfterSeeking;
    bool                mDecoderWaitForNextKeyFrame; // after seeking we wait for next i -frames
    /* picture grabbing */
    bool                mDecoderSinglePictureGrabbed;
    int                 mDecoderSinglePictureResX;
    int                 mDecoderSinglePictureResY;
    uint8_t             *mDecoderSinglePictureData[AV_NUM_DATA_POINTERS];
    int                 mDecoderSinglePictureLineSize[AV_NUM_DATA_POINTERS];
};

///////////////////////////////////////////////////////////////////////////////

}} // namespace

#endif
