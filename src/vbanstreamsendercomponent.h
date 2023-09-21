/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "udpclient.h"
#include "dynamicprocessornode.h"

// Nap includes
#include <nap/resourceptr.h>
#include <audio/utility/safeptr.h>
#include <udpclient.h>

// Audio includes
#include <audio/component/audiocomponentbase.h>
#include <audio/resource/audiobufferresource.h>
#include <audio/node/inputnode.h>


namespace nap
{
	namespace audio
	{
        // Forward declares
		class VBANStreamSenderComponentInstance;

        /**
         * The VBANStreamSenderComponent takes the input of an audio component and translates that into a VBAN stream
         * that will be send via the UDP Client
         */
		class NAPAPI VBANStreamSenderComponent : public AudioComponentBase
		{
			RTTI_ENABLE(AudioComponentBase)
			DECLARE_COMPONENT(VBANStreamSenderComponent, VBANStreamSenderComponentInstance)
		public:
			// Properties
			ResourcePtr<UDPClient> mUdpClient = nullptr; ///< property: 'UDPClient' The udpclient that sends the VBAN packets
			std::string mStreamName			  = "localhost"; ///< property: 'StreamName' The streamname of the VBAN stream
			nap::ComponentPtr<audio::AudioComponentBase> mInput; ///< property: 'Input' The component whose audio output will be send
			std::vector<int> mChannelRouting; ///< property: 'ChannelRouting' The component whose audio output will be send
		};

        /**
         * VBANStreamSenderComponentInstance
         * The instance of the VBANStreamSenderComponent, implements the DynamicProcessorNode::IProcessor interface
         */
		class NAPAPI VBANStreamSenderComponentInstance : public AudioComponentBaseInstance, public DynamicProcessorNode::IProcessor
		{
			RTTI_ENABLE(AudioComponentBaseInstance)
		public:
            /**
             * Constructor
             * @param entity entity
             * @param resource resource
             */
			VBANStreamSenderComponentInstance(EntityInstance& entity, Component& resource)
				: AudioComponentBaseInstance(entity, resource)
			{
			}

            /**
             * Initializes the instance, returns false on failure
             * @param errorState contains any error messages
             * @return false on failure
             */
            bool init(utility::ErrorState& errorState) override;

            /**
             * Called before deconstruction
             * Removes listener from VBANPacketReceiver
             */
            void onDestroy() override;

            /**
             * Processes the buffer for designated channel. Converts the buffer into vban packets
             * @param buffer the buffer
             * @param channel the channel
             */
            void processBuffer(const std::vector<float>& buffer, int channel) override;

            /**
             * Returns amount of channels
             * @return amount of channels
             */
			int getChannelCount() const override { return mInput->getChannelCount(); }

            /**
             * Returns output pin for given channel, no bound checking, assert on out of bound
             * @param channel the channel
             * @return OutputPin for channel
             */
			OutputPin* getOutputForChannel(int channel) override { return mInput->getOutputForChannel(channel); }

            /**
             * Sets streamname this VBANStreamSender accepts
             * @param streamName this VBANStreamSender accepts
             */
            void setStreamName(const std::string& streamName){ mStreamName = streamName; }

            /**
             * Returns streamname this VBANStreamPlayer accepts
             * @return streamname this VBANStreamPlayer accepts
             */
            const std::string& getStreamName() { return mStreamName; }
		private:
			ComponentInstancePtr<audio::AudioComponentBase> mInput	= {this, &VBANStreamSenderComponent::mInput};
			audio::SafeOwner<audio::DynamicProcessorNode> mDynamicProcessorNode = nullptr;

			VBANStreamSenderComponent* mResource = nullptr;
			audio::AudioService* mAudioService	 = nullptr;
			UDPClient* mUdpClient				 = nullptr;
			std::vector<int> mChannelRouting;
			std::vector<audio::SafeOwner<audio::Node>> mOutputs;
			std::string mStreamName;
			uint32_t mFrameCount = 0;
            std::vector<std::vector<char>> mBuffers;
            uint8_t mSampleRateFormat = 0;
		};
	}
}
