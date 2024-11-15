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
		class VBANStreamPlayerComponentInstance;

		/**
		 * VBANStreamPlayerComponent hooks up to a VBANPacketReceiver and translates incoming VBAN packets
		 * to audio buffers handled by a bufferplayer for each channel.
		 * The VBAN packets must be configured to have the same amount of channels as channels created in channel routing
		 */
		class NAPAPI VBANStreamPlayerComponent : public AudioComponentBase
		{
			RTTI_ENABLE(AudioComponentBase)
			DECLARE_COMPONENT(VBANStreamPlayerComponent, VBANStreamPlayerComponentInstance)

		public:
			/**
			 * Constructor
			 */
			VBANStreamPlayerComponent() : AudioComponentBase() { }

			/**
			 * Returns if the playback consists of 2 audio channels
			 */
			bool isStereo() const { return mChannelRouting.size() == 2; }

			// Properties
			ResourcePtr<VBANPacketReceiver> mVBANPacketReceiver = nullptr; ///< Property: "VBANPacketReceiver" the packet receiver
			std::vector<int> mChannelRouting = { }; ///< Property: "ChannelRouting" the channel routing, must be equal to excpected channels from stream
			int mMaxBufferSize = 4096; ///< Property: "MaxBufferSize" the max buffer size in samples. Keep this as low as possible to ensure the lowest possible latency
			std::string mStreamName; ///< Property: "StreamName" the VBAN stream to listen to
		public:
		};


		/**
		 * VBANStreamPlayerComponentInstance
		 * Instance of VBANStreamPlayerComponent. Implements IVBANStreamListener interface
		 */
		class NAPAPI VBANStreamPlayerComponentInstance : public AudioComponentBaseInstance, public IVBANStreamListener
		{
			RTTI_ENABLE(AudioComponentBaseInstance)

		public:
			/**
			 * Constructor
			 * @param entity entity
			 * @param resource resource
			 */
			VBANStreamPlayerComponentInstance(EntityInstance& entity, Component& resource)
				: AudioComponentBaseInstance(entity, resource) { }

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
			int getChannelCount() const override { return mBufferPlayers.size(); }
			int getPortNumber() const  {return mVbanReceiver->mServer->mPort;}

			/**
			 * Returns output pin for given channel, no bound checking, assert on out of bound
			 * @param channel the channel
			 * @return OutputPin for channel
			 */
			OutputPin* getOutputForChannel(int channel) override { assert(channel < mBufferPlayers.size()); return &mBufferPlayers[channel]->audioOutput; }

			/**
			 * Pushes the buffers to the buffer players
			 * @param buffers the buffers to push
			 */
			void pushBuffers(const std::vector<std::vector<float>>& buffers) override;

			/**
			 * Sets streamname this VBANStreamPlayer accepts
			 * @param streamName this VBANStreamPlayer accepts
			 */
			void setStreamName(const std::string& streamName){ mStreamName = streamName; }

			/**
			 * Returns streamname this VBANStreamPlayer accepts
			 * @return streamname this VBANStreamPlayer accepts
			 */
			const std::string& getStreamName() override { return mStreamName; }

			/**
			 * Returns sample rate used by listener
			 * @return sample rate used by listener
			 */
			int getSampleRate() const override{ return mSampleRate; }

		private:
			std::vector<SafeOwner<SampleQueuePlayerNode>> mBufferPlayers;
			std::vector<int> mChannelRouting;
			std::string mStreamName;

			VBANStreamPlayerComponent* mResource = nullptr; // The component's resource
			NodeManager* mNodeManager = nullptr; // The audio node manager this component's audio nodes are managed by
			AudioService* mAudioService = nullptr; // audio server
			VBANPacketReceiver* mVbanReceiver = nullptr; // the vban packet receiver
			int mSampleRate = 0; // sample rate
		};
	}
}
