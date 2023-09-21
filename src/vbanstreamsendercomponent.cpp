/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "vbanstreamsendercomponent.h"
#include "udppacket.h"
#include "dynamicprocessornode.h"
#include "vbanutils.h"
#include "vban/vban.h"

#include <entity.h>
#include <audio/service/audioservice.h>
#include <audio/node/outputnode.h>

RTTI_BEGIN_CLASS(nap::audio::VBANStreamSenderComponent)
RTTI_PROPERTY("UdpClient", &nap::audio::VBANStreamSenderComponent::mUdpClient, nap::rtti::EPropertyMetaData::Required)
RTTI_PROPERTY("Input", &nap::audio::VBANStreamSenderComponent::mInput, nap::rtti::EPropertyMetaData::Required)
RTTI_PROPERTY("StreamName", &nap::audio::VBANStreamSenderComponent::mStreamName, nap::rtti::EPropertyMetaData::Default)
RTTI_END_CLASS

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::audio::VBANStreamSenderComponentInstance)
		RTTI_CONSTRUCTOR(nap::EntityInstance&, nap::Component &)
RTTI_END_CLASS

using namespace nap::audio;

namespace nap
{
	void VBANStreamSenderComponentInstance::onDestroy()
	{
        mDynamicProcessorNode->removeProcessor(this);
	}


	bool VBANStreamSenderComponentInstance::init(utility::ErrorState& errorState)
	{
        // acquire audio service and node manager
        mAudioService = getEntityInstance()->getCore()->getService<AudioService>();
        auto& nodeManager = mAudioService->getNodeManager();

        // acquire resources
		auto* resource = getComponent<VBANStreamSenderComponent>();
		mResource = resource;
		mChannelRouting = resource->mChannelRouting;
        mStreamName = mResource->mStreamName;
        mUdpClient = mResource->mUdpClient.get();

        // acquire sample rate format
        if(!utility::getVBANSampleRateFormatFromSampleRate(mSampleRateFormat,
                                                           static_cast<int>(nodeManager.getSampleRate()),
                                                           errorState))
            return false;

        // configure channel routing
		if (mChannelRouting.empty())
		{
			for (auto channel = 0; channel < mInput->getChannelCount(); ++channel)
				mChannelRouting.emplace_back(channel);
		}
		for (auto channel = 0; channel < mChannelRouting.size(); ++channel)
        {
            if (mChannelRouting[channel] >= mInput->getChannelCount())
            {
                errorState.fail("%s: Trying to route input channel that is out of bounds.", resource->mID.c_str());
                return false;
            }
        }

        // Create the dynamic processor node
        mDynamicProcessorNode = nodeManager.makeSafe<DynamicProcessorNode>(nodeManager);
        mDynamicProcessorNode->registerProcessor(this);

        // Connect outputs to dynamic processor node
		for (auto channel = 0; channel < mChannelRouting.size(); ++channel)
		{
			if (mChannelRouting[channel] < 0)
				continue;

			mDynamicProcessorNode->inputs.connect(*mInput->getOutputForChannel(mChannelRouting[channel]));
		}

        // create the dynamic buffers
        for(int i = 0; i < mChannelRouting.size(); i++)
        {
            mBuffers.emplace_back(std::vector<char>());
        }

		return true;
	}


	void VBANStreamSenderComponentInstance::processBuffer(const std::vector<nap::audio::SampleValue>& buffer, int channel)
	{
        assert(channel < mBuffers.size()); // channel should always be in range

        int channels = mChannelRouting.size(); // number of channels
        int buffer_size_for_each_channel = VBAN_SAMPLES_MAX_NB / channels; // buffer size for each channel
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
        if(channel == channels-1)
        {
            // compute the amount of buffers we need to send
            int buffer_num = mBuffers[channel].size() / buffer_size_for_each_channel;

            // compute the payload size
            int payload_size = buffer_size_for_each_channel * channels;

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
                hdr->format_nbc = mChannelRouting.size() - 1;
                hdr->format_SR  = mSampleRateFormat;
                hdr->format_bit = VBAN_BITFMT_16_INT;
                strncpy(hdr->streamname, mStreamName.c_str(), VBAN_STREAM_NAME_SIZE-1);
                hdr->nuFrame    = mFrameCount;
                hdr->format_nbs = (payload_size / ((hdr->format_nbc+1) * VBanBitResolutionSize[(hdr->format_bit & VBAN_BIT_RESOLUTION_MASK)])) - 1;

                // copy bytes for each channel
                for(int c = 0 ; c < mChannelRouting.size(); c++)
                {
                    std::memcpy(&buffer[VBAN_HEADER_SIZE + c * buffer_size_for_each_channel],
                                &mBuffers[c][i * buffer_size_for_each_channel],
                                buffer_size_for_each_channel);
                }

                // create udp packet & send
                UDPPacket packet(std::move(buffer));
                mUdpClient->send(packet);

                // advance framecount
                mFrameCount++;
            }

            // see if we have buffers to clear
            if(buffer_num > 0)
            {
                for(int j = 0 ; j < mChannelRouting.size(); j++)
                {
                    mBuffers[j].erase(mBuffers[j].begin(), mBuffers[j].begin() + buffer_num * buffer_size_for_each_channel);
                }
            }
        }
	}
}