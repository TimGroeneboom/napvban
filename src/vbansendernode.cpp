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
			// get output buffers
			inputs.pull(mInputPullResult);
            setChannelCount(mInputPullResult.size());

            int channel = 0;
            for (auto& buffer : mInputPullResult)
            {
                processBuffer(*buffer, channel);
                channel++;
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
            if (mBuffers.size() != channelCount)
            {
                mBuffers.resize(channelCount);
            }
        }


        void VBANSenderNode::processBuffer(const SampleBuffer& buffer, int channel)
        {
            if (mUDPClient == nullptr)
                return;

            if (mStreamName.empty())
                return;

            if (mSampleRateFormat < 0)
                return;

            assert(channel < mBuffers.size()); // channel should always be in range

            int buffer_size_for_each_channel = VBAN_SAMPLES_MAX_NB / getChannelCount(); // buffer size for each channel
            for(float sampleValue : buffer)
            {
                // convert float to short
                short value = static_cast<short>(sampleValue * 32768.0f);

                // convert short to two bytes
                char byte_1 = value;
                char byte_2 = value >> 8;

                // put them in the correct buffer
                mBuffers[channel].emplace_back(byte_1);
                mBuffers[channel].emplace_back(byte_2);
            }

            // on push of last channel, create the packet
            if(channel == getChannelCount() - 1)
            {
                // compute the amount of buffers we need to send
                int buffer_num = mBuffers[channel].size() / buffer_size_for_each_channel;

                // compute the payload size
                int payload_size = buffer_size_for_each_channel * getChannelCount();

                // continue creating the necessary packets
                for(int i = 0 ; i < buffer_num; i++)
                {
                    // create the buffer
                    std::vector<nap::uint8> buffer;

                    // resize it to have the correct size
                    buffer.resize(VBAN_HEADER_SIZE + payload_size);

                    // init header
                    auto* const hdr = (struct VBanHeader*)(&buffer[0]);
                    hdr->vban       = *(int32_t*)("VBAN");
                    hdr->format_nbc = getChannelCount() - 1;
                    hdr->format_SR  = mSampleRateFormat;
                    hdr->format_bit = VBAN_BITFMT_16_INT;
                    strncpy(hdr->streamname, mStreamName.c_str(), VBAN_STREAM_NAME_SIZE-1);
                    hdr->nuFrame    = mFrameCount;
                    hdr->format_nbs = (payload_size / ((hdr->format_nbc+1) * VBanBitResolutionSize[(hdr->format_bit & VBAN_BIT_RESOLUTION_MASK)])) - 1;

                    // copy bytes for each channel
                    for (int c = 0 ; c <getChannelCount(); c++)
                    {
                        std::memcpy(&buffer[VBAN_HEADER_SIZE + c * buffer_size_for_each_channel],
                                    &mBuffers[c][i * buffer_size_for_each_channel],
                                    buffer_size_for_each_channel);
                    }

                    // create udp packet & send
                    UDPPacket packet(std::move(buffer));
                    mUDPClient->send(packet);

                    // advance framecount
                    mFrameCount++;
                }

                // see if we have buffers to clear
                if (buffer_num > 0)
                {
                    for(int j = 0 ; j < getChannelCount(); j++)
                    {
                        mBuffers[j].erase(mBuffers[j].begin(), mBuffers[j].begin() + buffer_num * buffer_size_for_each_channel);
                    }
                }
            }
        }


	}
}
