/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <vbansendernode.h>
#include <vbanstreamsendercomponent.h>
#include <vban/vban.h>
#include <vbanutils.h>

#include <audio/core/audionodemanager.h>

#include <nap/logger.h>

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::audio::VBANSenderNode)
		RTTI_PROPERTY("input", &nap::audio::VBANSenderNode::inputs, nap::rtti::EPropertyMetaData::Embedded)
RTTI_END_CLASS


namespace nap
{
	namespace audio
	{

		VBANSenderNode::VBANSenderNode(NodeManager& nodeManager) : Node(nodeManager)
        {
            getNodeManager().registerRootProcess(*this);
            mInputPullResult.reserve(2);
            sampleRateChanged(nodeManager.getSampleRate());
		}


		VBANSenderNode::~VBANSenderNode()
		{
			getNodeManager().unregisterRootProcess(*this);
		}


		void VBANSenderNode::process()
		{
            if (mUDPClient == nullptr)
                return;

            if (mStreamName.empty())
                return;

            assert(mSampleRateFormat >= 0);

			// get output buffers
			inputs.pull(mInputPullResult);
            setChannelCount(mInputPullResult.size());

            for (auto i = 0; i < getBufferSize(); ++i)
            {
                for (auto channel = 0; channel < mChannelCount; ++channel)
                {
                    float sample = (*mInputPullResult[channel])[i];

                    // convert float to short
                    short value = static_cast<short>(sample * 32768.0f);

                    // convert short to two bytes
                    char byte_1 = value;
                    char byte_2 = value >> 8;

                    mBufferChannelOffsets[channel][mBufferWritePosition] = byte_1;
                    mBufferChannelOffsets[channel][mBufferWritePosition + 1] = byte_2;
                }
                mBufferWritePosition = mBufferWritePosition + 2;

                if (mBufferWritePosition == mBufferSizePerChannel)
                {
                    mVBANHeader->nuFrame = mFrameCount;

                    // create udp packet & send
                    UDPPacket packet(mBuffer);
                    mUDPClient->send(packet);

                    // reset udp buffer write position
                    mBufferWritePosition = 0;

                    // advance framecount
                    mFrameCount++;
                }
            }
		}


        void VBANSenderNode::sampleRateChanged(float sampleRate)
        {
            // acquire sample rate format
            utility::ErrorState errorState;
            if (!utility::getVBANSampleRateFormatFromSampleRate(mSampleRateFormat,
                                                                static_cast<int>(sampleRate),
                                                                errorState))
            {
                nap::Logger::error(errorState.toString().c_str());
                mSampleRateFormat = -1;
            }
        }


        void VBANSenderNode::setChannelCount(int channelCount)
        {
            if (mChannelCount != channelCount)
            {
                mChannelCount = channelCount;
                // buffer size for each channel
                mBufferSizePerChannel = VBAN_SAMPLES_MAX_NB;

                // if total buffersize exceeds max data size, resize buffersize to fit max data size
                if (mBufferSizePerChannel * mChannelCount > VBAN_DATA_MAX_SIZE)
                    mBufferSizePerChannel = VBAN_DATA_MAX_SIZE / mChannelCount;

                // compute the buffer size of all channels together
                int totalBufferSize = mBufferSizePerChannel * mChannelCount;

                // resize it to have the correct size
                mBuffer.resize(VBAN_HEADER_SIZE + totalBufferSize);

                // init header
                mVBANHeader = (struct VBanHeader*)(&mBuffer[0]);
                mVBANHeader->vban       = *(int32_t*)("VBAN");
                mVBANHeader->format_nbc = getChannelCount() - 1;
                mVBANHeader->format_SR  = mSampleRateFormat;
                mVBANHeader->format_bit = VBAN_BITFMT_16_INT;
                strncpy(mVBANHeader->streamname, mStreamName.c_str(), VBAN_STREAM_NAME_SIZE-1);
                mVBANHeader->nuFrame    = mFrameCount;
                mVBANHeader->format_nbs = (totalBufferSize / ((mVBANHeader->format_nbc+1) * VBanBitResolutionSize[(mVBANHeader->format_bit & VBAN_BIT_RESOLUTION_MASK)])) - 1;

                mBufferChannelOffsets.resize(mChannelCount);
                for (auto channel = 0; channel < mChannelCount; ++channel)
                    mBufferChannelOffsets[channel] = &mBuffer[VBAN_HEADER_SIZE + channel * mBufferSizePerChannel];
            }
        }


	}
}
