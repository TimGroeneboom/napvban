/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "vbanstreamsendercomponent.h"
#include "udppacket.h"
#include "vbansendernode.h"

#include "vban/vban.h"

#include <entity.h>
#include <audio/service/audioservice.h>
#include <audio/node/outputnode.h>

RTTI_BEGIN_CLASS(nap::audio::VbanStreamSenderComponent)
RTTI_PROPERTY("UdpClient", &nap::audio::VbanStreamSenderComponent::mUdpClient, nap::rtti::EPropertyMetaData::Required)
RTTI_PROPERTY("Input", &nap::audio::VbanStreamSenderComponent::mInput, nap::rtti::EPropertyMetaData::Required)
RTTI_PROPERTY("StreamName", &nap::audio::VbanStreamSenderComponent::mStreamName, nap::rtti::EPropertyMetaData::Default)
RTTI_END_CLASS

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::audio::VbanStreamSenderComponentInstance)
		RTTI_CONSTRUCTOR(nap::EntityInstance&, nap::Component &)
RTTI_END_CLASS

using namespace nap::audio;

namespace nap
{

	bool VbanStreamSenderComponentInstance::init(utility::ErrorState& errorState)
	{
		VbanStreamSenderComponent* resource = getComponent<VbanStreamSenderComponent>();
		mResource = resource;

		auto audioService = getEntityInstance()->getCore()->getService<AudioService>();
		auto& nodeManager = audioService->getNodeManager();

		auto& channelRouting = resource->mChannelRouting;
		if (channelRouting.empty())
		{
			for (auto channel = 0; channel < mInput->getChannelCount(); ++channel)
                channelRouting.emplace_back(channel);
		}

		for (auto channel = 0; channel < channelRouting.size(); ++channel)
			if (channelRouting[channel] >= mInput->getChannelCount())
			{
				errorState.fail("%s: Trying to rout input channel that is out of bounds.", resource->mID.c_str());
				return false;
			}

        mDynamicProcessorNode = nodeManager.makeSafe<VBANSenderNode>(nodeManager);

		for (auto channel = 0; channel < channelRouting.size(); ++channel)
		{
			if (channelRouting[channel] < 0)
				continue;

			mDynamicProcessorNode->inputs.connect(*mInput->getOutputForChannel(channelRouting[channel]));
		}

        mDynamicProcessorNode->setStreamName(mResource->mStreamName);
        mDynamicProcessorNode->setUDPClient(mResource->mUdpClient.get());

		return true;
	}


}