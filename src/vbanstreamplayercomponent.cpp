/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "vbanstreamplayercomponent.h"
#include "udpclient.h"

// Nap includes
#include <entity.h>
#include <nap/core.h>

// Audio includes
#include <audio/service/audioservice.h>

// RTTI
RTTI_BEGIN_CLASS(nap::audio::VbanStreamPlayerComponent)
		RTTI_PROPERTY("VBANPacketReceiver", &nap::audio::VbanStreamPlayerComponent::mVBANPacketReceiver, nap::rtti::EPropertyMetaData::Required)
		RTTI_PROPERTY("ChannelRouting", &nap::audio::VbanStreamPlayerComponent::mChannelRouting,nap::rtti::EPropertyMetaData::Default)
		RTTI_PROPERTY("MaxBufferSize", &nap::audio::VbanStreamPlayerComponent::mMaxBufferSize, nap::rtti::EPropertyMetaData::Default)
		RTTI_PROPERTY("StreamName", &nap::audio::VbanStreamPlayerComponent::mStreamName, nap::rtti::EPropertyMetaData::Default)
RTTI_END_CLASS

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::audio::VbanStreamPlayerComponentInstance)
		RTTI_CONSTRUCTOR(nap::EntityInstance &, nap::Component &)
RTTI_END_CLASS

namespace nap
{

	namespace audio
	{
		void VbanStreamPlayerComponentInstance::onDestroy()
		{
            mVbanListener->removeStreamListener(this);
		}

		bool VbanStreamPlayerComponentInstance::init(utility::ErrorState& errorState)
		{
			mResource = getComponent<VbanStreamPlayerComponent>();
			mVbanListener = mResource->mVBANPacketReceiver.get();
			mStreamName = mResource->mStreamName;

			mAudioService = getEntityInstance()->getCore()->getService<AudioService>();
			mNodeManager = &mAudioService->getNodeManager();

			mChannelRouting = mResource->mChannelRouting;

			for (auto channel = 0; channel < mChannelRouting.size(); ++channel)
			{
				auto bufferPlayer = mNodeManager->makeSafe<SampleQueuePlayerNode>(*mNodeManager);
                bufferPlayer->mMaxQueueSize = mResource->mMaxBufferSize;
				mBufferPlayers.emplace_back(std::move(bufferPlayer));
			}

            mVbanListener->registerStreamListener(this);

			return true;
		}


		void VbanStreamPlayerComponentInstance::update(double deltaTime)
		{
		}


		void VbanStreamPlayerComponentInstance::pushBuffers(std::vector<std::vector<float>>& buffers)
		{
			if(buffers.size() == getChannelCount())
			{
				for(int i = 0; i < mBufferPlayers.size(); i++)
				{
					mBufferPlayers[i]->queueSamples(&buffers[i][0], buffers[i].size());
				}
			}else
			{
				nap::Logger::warn("error received %i buffers but expected %i", buffers.size(), getChannelCount());
			}
		}
	}
}
