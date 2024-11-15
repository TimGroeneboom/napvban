/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "samplequeueplayernode.h"
#include "mathutils.h"

#include <nap/logger.h>

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::audio::SampleQueuePlayerNode)
	RTTI_PROPERTY("audioOutput", &nap::audio::SampleQueuePlayerNode::audioOutput, nap::rtti::EPropertyMetaData::Embedded)
RTTI_END_CLASS

namespace nap
{

	namespace audio
	{

		SampleQueuePlayerNode::SampleQueuePlayerNode(NodeManager& manager) : Node(manager)
		{
			mBufferSize = getBufferSize();
			mSamples = std::vector<SampleValue>(mBufferSize);
		}


		void SampleQueuePlayerNode::queueSamples(const float* samples, size_t numSamples)
		{
			// check if queue size is exceeded, if so throw a warning, if not queue the samples
			if(mQueue.size_approx() <= mMaxQueueSize)
			{
				if (!mQueue.enqueue_bulk(samples, numSamples))
				{
					nap::Logger::error("%s: Failed to allocate memory for queue buffer",  std::string(get_type().get_name()).c_str());
				}
			}else
			{
				if(mVerbose)
					nap::Logger::warn("%s: Dropping samples because buffer is getting to big", std::string(get_type().get_name()).c_str());
			}
		}


		void SampleQueuePlayerNode::process()
		{
			// get buffer size
			const int available_samples = mQueue.size_approx();
			int buffer_size_to_copy = available_samples;
			if(available_samples > mBufferSize)
				buffer_size_to_copy = mBufferSize;

			// if the amount of samples in the queue exceeds buffer sized used by audio service we can fill the outputbuffer
			if(buffer_size_to_copy > 0)
			{
				// get output buffer
				auto& outputBuffer = getOutputBuffer(audioOutput);

				// if sample buffer is smaller the buffersize fill beginning with silence
				int silent_samples_num = mBufferSize - available_samples;
				silent_samples_num = math::max(silent_samples_num, 0);
				if(silent_samples_num > 0)
				{
					std::fill(outputBuffer.begin(), outputBuffer.begin() + silent_samples_num, 0.0f);
				}

				// dequeue the samples from the queue and fill the rest with the outputbuffer
				if(mQueue.try_dequeue_bulk(mSamples.begin(), buffer_size_to_copy))
				{
					// fill the output buffer
					std::memcpy(&outputBuffer.data()[silent_samples_num], mSamples.data(), buffer_size_to_copy * sizeof(SampleValue));
				}
			}else
			{
				// no samples in queue, fill with silence
				if(mVerbose)
					nap::Logger::warn("%s: Not enough samples in queue", std::string(get_type().get_name()).c_str());
				auto& outputBuffer = getOutputBuffer(audioOutput);
				std::fill(outputBuffer.begin(), outputBuffer.end(), 0.0f);
			}
		}
	}
}
