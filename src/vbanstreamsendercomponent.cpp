/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "vbanstreamsendercomponent.h"
#include "udppacket.h"
#include "vbansendernode.h"
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
		mVBANSenderNode->setUDPClient(nullptr);
	}


	bool VBANStreamSenderComponentInstance::init(utility::ErrorState& errorState)
	{
		// acquire audio service and node manager
		auto audioService = getEntityInstance()->getCore()->getService<AudioService>();
		auto& nodeManager = audioService->getNodeManager();

		// acquire resources
		auto* resource = getComponent<VBANStreamSenderComponent>();
		auto& channelRouting = resource->mChannelRouting;

		// configure channel routing
		if (channelRouting.empty())
		{
			for (auto channel = 0; channel < mInput->getChannelCount(); ++channel)
				channelRouting.emplace_back(channel);
		}
		for (auto channel = 0; channel < channelRouting.size(); ++channel)
		{
			if (channelRouting[channel] >= mInput->getChannelCount())
			{
				errorState.fail("%s: Trying to route input channel that is out of bounds.", resource->mID.c_str());
				return false;
			}
		}

		// Create the VBAN sender node
		mVBANSenderNode = nodeManager.makeSafe<VBANSenderNode>(nodeManager);
		mVBANSenderNode->setStreamName(resource->mStreamName);
		mVBANSenderNode->setUDPClient(resource->mUdpClient.get());

		// Connect outputs to VBAN sender node
		for (auto channel = 0; channel < channelRouting.size(); ++channel)
		{
			if (channelRouting[channel] < 0)
				continue;

			mVBANSenderNode->inputs.connect(*mInput->getOutputForChannel(channelRouting[channel]));
		}

		return true;
	}


	void VBANStreamSenderComponentInstance::setStreamName(const std::string& name)
	{
		mVBANSenderNode->setStreamName(name);
	}


}