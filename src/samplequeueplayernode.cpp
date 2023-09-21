/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "samplequeueplayernode.h"

#include <nap/logger.h>

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::audio::SampleQueuePlayerNode)
RTTI_PROPERTY("audioOutput", &nap::audio::SampleQueuePlayerNode::audioOutput, nap::rtti::EPropertyMetaData::Embedded)
RTTI_PROPERTY("maxQueueSize", &nap::audio::SampleQueuePlayerNode::mMaxQueueSize, nap::rtti::EPropertyMetaData::Default)
RTTI_PROPERTY("verbose", &nap::audio::SampleQueuePlayerNode::mMaxQueueSize, nap::rtti::EPropertyMetaData::Default)
RTTI_END_CLASS

namespace nap
{

	namespace audio
	{

		SampleQueuePlayerNode::SampleQueuePlayerNode(NodeManager& manager) : Node(manager)
		{
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
            const int buffer_size = this->getBufferSize();

            // if the amount of samples in the queue exceeds buffer sized used by audio service we can fill the outputbuffer
            if(mQueue.size_approx() >= buffer_size)
            {
                // dequeue the samples from the queue
                std::vector<SampleValue> samples(buffer_size);
                if(mQueue.try_dequeue_bulk(samples.begin(), buffer_size))
                {
                    // fill the output buffer
                    auto& outputBuffer = getOutputBuffer(audioOutput);
                    std::memcpy(outputBuffer.data(), samples.data(), buffer_size * sizeof(SampleValue));
                }
            }else
            {
                // not enough samples in queue, fill with silence
                if(mVerbose)
                    nap::Logger::warn("%s: Not enough samples in queue", std::string(get_type().get_name()).c_str());
                auto& outputBuffer = getOutputBuffer(audioOutput);
                std::fill(outputBuffer.begin(), outputBuffer.end(), 0.0f);
            }
		}
	}
}
