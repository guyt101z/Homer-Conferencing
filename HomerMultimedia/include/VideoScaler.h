/*****************************************************************************
 *
 * Copyright (C) 2012 Thomas Volkert <thomas@homer-conferencing.com>
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
 * Purpose: video scaler
 * Author:  Thomas Volkert
 * Since:   2012-08-09
 */

#ifndef _MULTIMEDIA_VIDEO_SCALER_
#define _MULTIMEDIA_VIDEO_SCALER_

#include <Header_Ffmpeg.h>
#include <HBMutex.h>
#include <HBThread.h>
#include <MediaFifo.h>
#include <RTP.h>

#include <vector>
#include <string>

using namespace Homer::Base;

namespace Homer { namespace Multimedia {

///////////////////////////////////////////////////////////////////////////////

// the following de/activates debugging of sent packets
//#define VS_DEBUG_PACKETS

///////////////////////////////////////////////////////////////////////////////

class VideoScaler:
    public Thread, public MediaFifo
{
public:
    VideoScaler();

    virtual ~VideoScaler();

    void StartScaler(enum CodecID pTargetCodecId, int pSourceResX, int pSourceResY, int pTargetResX, int pTargetResY);
    void StopScaler();

    virtual void WriteFifo(char* pBuffer, int pBufferSize);
    virtual void ReadFifo(char *pBuffer, int &pBufferSize); // memory copy, returns entire memory
    virtual void ClearFifo();

    // avoids memory copy, returns a pointer to memory
    virtual int ReadFifoExclusive(char **pBuffer, int &pBufferSize); // return -1 if internal FIFO isn't available yet
    virtual void ReadFifoExclusiveFinished(int pEntryPointer);

    virtual int GetEntrySize();
    virtual int GetUsage();
    virtual int GetSize();

private:
    virtual void* Run(void* pArgs = NULL); // video scaler main loop

    MediaFifo           *mInputFifo;
    MediaFifo           *mOutputFifo;
    bool                mScalerNeeded;
    int                 mSourceResX;
    int                 mSourceResY;
    int                 mTargetResX;
    int                 mTargetResY;
    int                 mChunkNumber;
    enum CodecID        mTargetCodecId;
    enum PixelFormat    mTargetPixelFormat;
    SwsContext          *mScalerContext;
};

///////////////////////////////////////////////////////////////////////////////

}} // namespace

#endif
