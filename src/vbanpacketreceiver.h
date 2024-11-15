/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

// Internal includes
#include <nap/resource.h>
#include <udpserver.h>
#include <udppacket.h>
#include <utility/threading.h>

#include "vbanudpserver.h"

namespace nap
{

	/**
	 * Derive from this class to handle an incoming VBAN audio stream.
	 */
	class NAPAPI IVBANStreamListener
	{
	public:
		virtual ~IVBANStreamListener() = default;

		/**
		 * Has to be overridden to handle incoming audio data for the stream
		 * @param buffers multichannel audio buffer containing audio for each channel in the stream
		 */
		virtual void pushBuffers(const std::vector<std::vector<float>>& buffers) = 0;

		/**
		 * @return Has to return the name of the VBAN audio stream that this receiver will handle.
		 */
		virtual const std::string& getStreamName() = 0;

		/**
		 * Returns sample rate used by listener
		 * @return sample rate used by listener
		 */
		virtual int getSampleRate() const = 0;
	};


	/**
	 * Resource that listens to incoming VBAN UDP packets on an UDPServer object.
	 * The VBANPacketReceiver parses the packets and dispatches them to different IVBANStreamAudioReceiver objects for each stream.
	 */
	class NAPAPI VBANPacketReceiver final : public Resource
	{
		RTTI_ENABLE(Resource)

	public:
		// Inherited from Resource
		virtual bool init(utility::ErrorState& errorState);

		/**
		 * Register a new receiver for a certain stream
		 * @param listener IVBANStreamListener object that handles incoming VBAN packets for a VBAN stream
		 */
		void registerStreamListener(IVBANStreamListener* listener);

		/**
		 * Unregister an existing receiver
		 * @param listener IVBANStreamListener object that handles incoming VBAN packets for a VBAN stream
		 */
		void removeStreamListener(IVBANStreamListener* listener);

	public:
		ResourcePtr<VBANUDPServer> mServer = nullptr; ///< Property: 'Server' Pointer to the VBAN UDP server receiving the packets

	protected:
		Slot<const UDPPacket&> mPacketReceivedSlot = { this, &VBANPacketReceiver::packetReceived };
		void packetReceived(const UDPPacket& packet);

	private:
		bool checkPacket(utility::ErrorState& errorState, nap::uint8 const* buffer, size_t size);
		bool checkPcmPacket(utility::ErrorState& errorState, nap::uint8 const* buffer, size_t size);

	private:
		std::vector<IVBANStreamListener*> mReceivers;
		TaskQueue mTaskQueue;
		std::vector<std::vector<float>> mBuffers; // Here as to not reallocate them for every received packet
	};


}
