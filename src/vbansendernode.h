/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

// Std includes
#include <atomic>

#include <vban/vban.h>

#include <udpclient.h>

// Audio includes
#include <audio/core/audionode.h>
#include <audio/utility/dirtyflag.h>
#include <audio/utility/safeptr.h>
#include <audio/core/process.h>
#include <audio/core/audionodemanager.h>

namespace nap
{

	namespace audio
	{

		/**
		 * Node that allows registering one or more non-node processor objects that incoming audio will be passed to.
		 */
		class NAPAPI VBANSenderNode : public Node
		{
			RTTI_ENABLE(Node)

		public:
			VBANSenderNode(NodeManager& nodeManager);

			virtual ~VBANSenderNode();

			/**
			 * Connect incoming audio to be precessed by the processors here.
			 */
			MultiInputPin inputs = {this};

			void setUDPClient(UDPClient* client) { getNodeManager().enqueueTask([&, client](){ mUDPClient = client; }); }
			void setStreamName(const std::string& name) { getNodeManager().enqueueTask([&, name](){ mStreamName = name; }); }

		private:
			void setChannelCount(int channelCount);
			int getChannelCount() const { return mChannelCount; }
			void processBuffer(const SampleBuffer& buffer, int channel);

			// Inherited from Node
			void process() override;
			void sampleRateChanged(float) override;

			std::vector<std::vector<audio::SampleValue>*> mInputPullResult;
			int mChannelCount = 0;
			int mPacketChannelSize = 0;
			int mPacketWritePosition = 0;
			std::vector<nap::uint8> mPacketBuffer;
			VBanHeader* mPacketHeader = nullptr;

			size_t mPacketSize = 0;
			uint32_t mFrameCounter = 0;
			uint8_t mSampleRateFormat = 0;
			std::string mStreamName;
			UDPClient* mUDPClient = nullptr;
		};

	}

}
