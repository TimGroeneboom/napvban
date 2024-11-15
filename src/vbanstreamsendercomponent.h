/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "udpclient.h"
#include "vbansendernode.h"

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
		class NAPAPI VBANStreamSenderComponentInstance : public AudioComponentBaseInstance
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
			 * Sets the name of the stream being sent
			 * @param name New name of the stream
			 */
			void setStreamName(const std::string& name);

		private:
			ComponentInstancePtr<audio::AudioComponentBase> mInput	= {this, &VBANStreamSenderComponent::mInput};
			audio::SafeOwner<audio::VBANSenderNode> mVBANSenderNode = nullptr;
		};
	}
}
