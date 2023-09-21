/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

// Std includes
#include <atomic>

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

            void setStreamName(const std::string& name) { getNodeManager().enqueueTask([&, name](){ mStreamName = name; }); }
            void setUDPClient(UDPClient* client) { getNodeManager().enqueueTask([&, client]{ mUdpClient = client; }); }

		private:
            void setChannelCount(int channelCount);

			// Inherited from Node
			void process() override;

			std::vector<char> mBuffer;
            int mBufferPosition = 0;
            std::vector<std::vector<audio::SampleValue>*> mInputPullResult;
            std::string mStreamName;
            uint32_t mFrameCount = 0;
            UDPClient* mUdpClient = nullptr;
            int mChannelCount = 0;
		};

	}

}
