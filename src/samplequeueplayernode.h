/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

// Std includes
#include <atomic>

// Nap includes
#include <audio/utility/safeptr.h>
#include <utility/threading.h>

// Audio includes
#include <audio/core/audionode.h>
#include <audio/core/audionodemanager.h>

namespace nap
{
	namespace audio
	{

		/**
		 * Node that allows the queueing of samples from another thread before they are being send through the output pin.
		 *
		 */
		class NAPAPI SampleQueuePlayerNode : public Node
		{
			RTTI_ENABLE(Node)

		public:
			SampleQueuePlayerNode(NodeManager& manager);

			/**
			 * Connect output to this pin
			 */
			OutputPin audioOutput = {this};

			/**
			 * Queue any amount of samples from another thread to be played back through the outpu pin.
			 * @param samples Pointer to floating point data of sample data
			 * @param numSamples Number of samples to be queued
			 */
			void queueSamples(const float* samples, size_t numSamples);

			/**
			 * Sets the maximum size of the sample queue.
			 * @param value New maximum queue size
			 */
			void setMaxQueueSize(int value) { mMaxQueueSize = value; }

			/**
			 * @param value True if logging is enabled.
			 */
			void setVerbose(bool value) { mVerbose = value; }

		private:
			// Inherited from Node
			void process() override;

			moodycamel::ConcurrentQueue<float> mQueue;  // New samples are queued here from a different thread.
			std::vector<SampleValue> mSamples;
			int mBufferSize;
			std::atomic<int> mMaxQueueSize = { 4096 }; // The amount of samples that the queue is allowed to have
			std::atomic<bool> mVerbose = { false }; // Enable logging
		};

	}
}
