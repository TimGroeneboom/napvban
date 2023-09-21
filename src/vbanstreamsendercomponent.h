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

		class VbanStreamSenderComponentInstance;


		class NAPAPI VbanStreamSenderComponent : public AudioComponentBase
		{
			RTTI_ENABLE(AudioComponentBase)
			DECLARE_COMPONENT(VbanStreamSenderComponent, VbanStreamSenderComponentInstance)

		public:
			// Properties
			ResourcePtr<UDPClient> mUdpClient = nullptr;
			std::string mStreamName = "localhost";
			nap::ComponentPtr<audio::AudioComponentBase> mInput; ///< property: 'Input' The component whose audio output will be send
			std::vector<int> mChannelRouting;

		public:
		};


		class NAPAPI VbanStreamSenderComponentInstance : public AudioComponentBaseInstance
		{
			RTTI_ENABLE(AudioComponentBaseInstance)
		public:
			VbanStreamSenderComponentInstance(EntityInstance& entity, Component& resource)
				: AudioComponentBaseInstance(entity, resource)
			{
			}

			bool init(utility::ErrorState& errorState) override;

			// Inherited from AudioComponentBaseInstance
			int getChannelCount() const override { return mInput->getChannelCount(); }
			OutputPin* getOutputForChannel(int channel) override { return mInput->getOutputForChannel(channel); }

		private:
			ComponentInstancePtr<audio::AudioComponentBase> mInput	= {this, &VbanStreamSenderComponent::mInput};
			audio::SafeOwner<audio::VBANSenderNode> mDynamicProcessorNode = nullptr;
			VbanStreamSenderComponent* mResource = nullptr;
		};
	}
}
