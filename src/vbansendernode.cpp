/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "vbansendernode.h"
#include "vbanstreamsendercomponent.h"
#include "vban/vban.h"

#include <audio/core/audionodemanager.h>

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::audio::VBANSenderNode)
		RTTI_PROPERTY("input", &nap::audio::VBANSenderNode::inputs, nap::rtti::EPropertyMetaData::Embedded)
RTTI_END_CLASS


namespace nap
{

	namespace audio
	{

		VBANSenderNode::VBANSenderNode(NodeManager& nodeManager)
			: Node(nodeManager)
		{
			getNodeManager().registerRootProcess(*this);
            mInputPullResult.reserve(2);
		}


		VBANSenderNode::~VBANSenderNode()
		{
			getNodeManager().unregisterRootProcess(*this);
		}


        void VBANSenderNode::setChannelCount(int channelCount)
        {
            mChannelCount = channelCount;
            int num_samples = mChannelCount * VBanBitResolutionSize[(VBAN_BITFMT_16_INT & VBAN_BIT_RESOLUTION_MASK)];
            int payload_size = VBAN_DATA_MAX_SIZE / num_samples;
            payload_size = (int) ( payload_size / num_samples ) * num_samples;
            if (payload_size > VBAN_SAMPLES_MAX_NB)
                payload_size = VBAN_SAMPLES_MAX_NB;
            mBuffer.resize(payload_size);
        }


		void VBANSenderNode::process()
		{
            if (mUdpClient == nullptr)
                return;
            if (mStreamName.empty())
                return;

			// get output buffers
			inputs.pull(mInputPullResult);
            if (mInputPullResult.size() != mChannelCount)
                setChannelCount(mInputPullResult.size());

			int buffer_size = mInputPullResult[0]->size();
			int sample_count = 0;
			while (sample_count < buffer_size)
			{
				for (auto& buffer : mInputPullResult)
				{
					// convert float to short
					short value = static_cast<short>(buffer->data()[sample_count] * 32768.0f);

					// convert short to two bytes
					char byte_1 = value;
					char byte_2 = value >> 8;

                    mBuffer[mBufferPosition++] = byte_1;
                    mBuffer[mBufferPosition++] = byte_2;

					// process buffer when we reach max size of vban packet
					if (mBufferPosition == mBuffer.size())
					{
                        std::vector<nap::uint8> buffer;
                        buffer.resize(VBAN_HEADER_SIZE + VBAN_SAMPLES_MAX_NB);

                        // init header
                        auto* const hdr = (struct VBanHeader*)(&buffer[0]);
                        hdr->vban       = *(int32_t*)("VBAN");
                        hdr->format_nbc = mChannelCount - 1;
                        hdr->format_SR  = 16; // 44100
                        hdr->format_bit = VBAN_BITFMT_16_INT;
                        strncpy(hdr->streamname, mStreamName.c_str(), VBAN_STREAM_NAME_SIZE-1);
                        hdr->nuFrame    = mFrameCount;
                        hdr->format_nbs = (mBuffer.size() / ((hdr->format_nbc+1) * VBanBitResolutionSize[(hdr->format_bit & VBAN_BIT_RESOLUTION_MASK)])) - 1;

                        // copy bytes
                        std::memcpy(&buffer[VBAN_HEADER_SIZE], &mBuffer[0], VBAN_SAMPLES_MAX_NB);

                        // create udp packet & send
                        UDPPacket packet(std::move(buffer));
                        mUdpClient->send(packet);

                        mFrameCount++;
                        mBufferPosition = 0;
					}
				}
				sample_count++;
			}
		}

	}
}
