/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <vbansendernode.h>
#include <vbanstreamsendercomponent.h>
#include <vban/vban.h>
#include <vbanutils.h>

#include <audio/core/audionodemanager.h>

#include <nap/logger.h>

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::audio::VBANSenderNode)
		RTTI_PROPERTY("input", &nap::audio::VBANSenderNode::inputs, nap::rtti::EPropertyMetaData::Embedded)
RTTI_END_CLASS


namespace nap
{
	namespace audio
	{

		VBANSenderNode::VBANSenderNode(NodeManager& nodeManager) : Node(nodeManager)
		{
			getNodeManager().registerRootProcess(*this);
			mInputPullResult.reserve(2);
			sampleRateChanged(nodeManager.getSampleRate());
		}


		VBANSenderNode::~VBANSenderNode()
		{
			getNodeManager().unregisterRootProcess(*this);
		}


		void VBANSenderNode::process()
		{
			if (mUDPClient == nullptr)
				return;

			if (mStreamName.empty())
				return;

			assert(mSampleRateFormat >= 0);

			// get output buffers
			inputs.pull(mInputPullResult);
			setChannelCount(mInputPullResult.size());

			for (auto i = 0; i < getBufferSize(); ++i)
			{
				for (auto channel = 0; channel < mChannelCount; ++channel)
				{
					float sample = (*mInputPullResult[channel])[i];

					// convert float to short
					short value = static_cast<short>(sample * 32768.0f);

					// convert short to two bytes
					char byte_1 = value;
					char byte_2 = value >> 8;

					// copy sample bytes to packet
					mPacketBuffer[mPacketWritePosition] = byte_1;
					mPacketBuffer[mPacketWritePosition + 1] = byte_2;
					mPacketWritePosition += 2;
				}

				assert(mPacketWritePosition <= mPacketSize);
				if (mPacketWritePosition == mPacketSize)
				{
					// set the frame counter in the VBAN header
					mPacketHeader->nuFrame = mFrameCounter;

					// create udp packet & send
					UDPPacket packet(mPacketBuffer); // We deep copy the packet because we will reuse the buffer
					mUDPClient->send(packet);

					// reset udp buffer write position
					mPacketWritePosition = VBAN_HEADER_SIZE;

					// advance framecount
					mFrameCounter++;
				}
			}
		}


		void VBANSenderNode::sampleRateChanged(float sampleRate)
		{
			// acquire sample rate format
			utility::ErrorState errorState;
			if (!utility::getVBANSampleRateFormatFromSampleRate(mSampleRateFormat,
																static_cast<int>(sampleRate),
																errorState))
			{
				nap::Logger::error(errorState.toString().c_str());
				mSampleRateFormat = -1;
			}
		}


		void VBANSenderNode::setChannelCount(int channelCount)
		{
			// sanity check the amount of channels
			if(channelCount > 254)
			{
				nap::Logger::warn("Channel count %i not allowed, clamping to 254", channelCount);
				mChannelCount = 254;
			}

			if (mChannelCount != channelCount)
			{
				mChannelCount = channelCount;

				// buffer size for each channel
				mPacketChannelSize = VBAN_SAMPLES_MAX_NB * 2;

				// if total buffersize exceeds max data size, resize packet channel size to fit max data size
				if (mPacketChannelSize * mChannelCount > VBAN_DATA_MAX_SIZE)
					mPacketChannelSize = (VBAN_DATA_MAX_SIZE / (mChannelCount * 2)) * 2;

				// compute the buffer size of all channels together
				int total_buffer_size = mPacketChannelSize * mChannelCount;

				// resize the packet data to have the correct size
				mPacketBuffer.resize(VBAN_HEADER_SIZE + total_buffer_size);

				// set write position
				mPacketWritePosition = VBAN_HEADER_SIZE;

				// set packet size
				mPacketSize = mPacketChannelSize * mChannelCount + VBAN_HEADER_SIZE;

				// initialize VBAN header
				mPacketHeader = (struct VBanHeader*)(&mPacketBuffer[0]);
				mPacketHeader->vban       = *(int32_t*)("VBAN");
				mPacketHeader->format_nbc = mChannelCount - 1;
				mPacketHeader->format_SR  = mSampleRateFormat;
				mPacketHeader->format_bit = VBAN_BITFMT_16_INT;
				strncpy(mPacketHeader->streamname, mStreamName.c_str(), VBAN_STREAM_NAME_SIZE - 1);
				mPacketHeader->nuFrame    = mFrameCounter;
				mPacketHeader->format_nbs = (mPacketChannelSize / 2) - 1;
			}
		}
	}
}
