/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

// Nap includes
#include <nap/resourceptr.h>
#include <audio/utility/safeptr.h>

// Audio includes
#include <audio/component/audiocomponentbase.h>
#include <audio/resource/audiobufferresource.h>
#include <audio/node/bufferplayernode.h>
#include <audio/node/multiplynode.h>
#include <audio/node/controlnode.h>
#include <audio/node/filternode.h>

// Vban includes
#include "samplequeueplayernode.h"
#include "vbanpacketreceiver.h"

namespace nap
{
	namespace audio
	{
		// Forward declares
		class AudioService;
		class VbanStreamPlayerComponentInstance;


		class NAPAPI VbanStreamPlayerComponent : public AudioComponentBase
		{
			RTTI_ENABLE(AudioComponentBase)
			DECLARE_COMPONENT(VbanStreamPlayerComponent, VbanStreamPlayerComponentInstance)

		public:
			VbanStreamPlayerComponent() : AudioComponentBase() { }

			// Properties
			ResourcePtr<VBANPacketReceiver> mVBANPacketReceiver = nullptr;
			std::vector<int> mChannelRouting = { };

			int mMaxBufferSize = 4096;
			std::string mStreamName = "localhost";

			/**
			 * Returns if the playback consists of 2 audio channels
			 */
			bool isStereo() const { return mChannelRouting.size() == 2; }

		public:
		};


		class NAPAPI VbanStreamPlayerComponentInstance : public AudioComponentBaseInstance, public IVBANStreamListener
		{
			RTTI_ENABLE(AudioComponentBaseInstance)

		public:
			VbanStreamPlayerComponentInstance(EntityInstance& entity, Component& resource) : AudioComponentBaseInstance(entity, resource) { }

			void onDestroy() override;

			// Inherited from ComponentInstance
			bool init(utility::ErrorState& errorState) override;
			void update(double deltaTime) override;

			// Inherited from AudioComponentBaseInstance
			int getChannelCount() const override { return mBufferPlayers.size(); }
			OutputPin* getOutputForChannel(int channel) override { return &mBufferPlayers[channel]->audioOutput; }

            // Inherited from IVbanStreamAudioReceiver
			void pushBuffers(std::vector<std::vector<float>>& buffers) override;

			const std::string& getStreamName() override { return mStreamName; }

		private:
			std::vector<SafeOwner<SampleQueuePlayerNode>> mBufferPlayers;

			std::vector<int> mChannelRouting;

			std::string mStreamName;

			VbanStreamPlayerComponent* mResource = nullptr; // The component's resource
			NodeManager* mNodeManager = nullptr; // The audio node manager this component's audio nodes are managed by
			AudioService* mAudioService = nullptr;
			VBANPacketReceiver* mVbanListener = nullptr;

			std::mutex mBufferMutex;
		};

	}

}
