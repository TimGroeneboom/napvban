/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

// Std includes
#include <atomic>

// Audio includes
#include <audio/core/audionode.h>
#include <audio/utility/dirtyflag.h>
#include <audio/utility/safeptr.h>
#include <audio/core/process.h>

namespace nap
{

	namespace audio
	{

        /**
         * Node that allows registering one or more non-node processor objects that incoming audio will be passed to.
         */
		class NAPAPI DynamicProcessorNode : public Node
		{
			RTTI_ENABLE(Node)

        public:
            /**
             * Subclass this class in order to create a processor for incoming audio
             */
            class NAPAPI IProcessor
            {
            public:
                virtual ~IProcessor() = default;
                /**
                 * Override this method to implement processing behaviour
                 * @param buffer Incoming samples
                 */
                virtual void processBuffer(const std::vector<float>& buffer, int channel) = 0;
            };

        public:
			DynamicProcessorNode(NodeManager& nodeManager);

			virtual ~DynamicProcessorNode();

            /**
             * Connect incoming audio to be precessed by the processors here.
             */
			MultiInputPin inputs = {this};

			// Inherited from Node
			void process() override;

            /**
             * Register a processor object
             */
			void registerProcessor(IProcessor* processor);

            /**
             * Unregister a processor object
             */
			void removeProcessor(IProcessor* processor);

		private:
			std::vector<IProcessor*> mProcessors;
            std::vector<std::vector<audio::SampleValue>*> mInputPullResult;
		};

	}

}
