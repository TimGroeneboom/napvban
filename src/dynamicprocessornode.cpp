/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "dynamicprocessornode.h"
#include "vbanstreamsendercomponent.h"
#include "vban/vban.h"

#include <audio/core/audionodemanager.h>

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::audio::DynamicProcessorNode)
		RTTI_PROPERTY("input", &nap::audio::DynamicProcessorNode::inputs, nap::rtti::EPropertyMetaData::Embedded)
RTTI_END_CLASS


namespace nap
{
	namespace audio
	{
		DynamicProcessorNode::DynamicProcessorNode(NodeManager& nodeManager)
			: Node(nodeManager)
		{
			getNodeManager().registerRootProcess(*this);
            mInputPullResult.reserve(2);
		}


		DynamicProcessorNode::~DynamicProcessorNode()
		{
			getNodeManager().unregisterRootProcess(*this);
		}


		void DynamicProcessorNode::process()
		{
			// get output buffers
			inputs.pull(mInputPullResult);

            int channel = 0;
            for(auto& buffer : mInputPullResult)
            {
                for (auto* processor : mProcessors)
                {
                    processor->processBuffer(*buffer, channel);
                }
                channel++;
			}
		}


		void DynamicProcessorNode::registerProcessor(IProcessor * processor)
		{
			getNodeManager().enqueueTask([this, processor]()
			{
				auto it = std::find_if(mProcessors.begin(), mProcessors.end(),
									   [processor](auto& a) { return a == processor; });

				assert(it == mProcessors.end()); // processor already registered

				if (it == mProcessors.end())
				{
					mProcessors.emplace_back(processor);
				}
			});
		}


		void DynamicProcessorNode::removeProcessor(IProcessor * processor)
		{
			getNodeManager().enqueueTask([this, processor]()
			{
				auto it = std::find_if(mProcessors.begin(), mProcessors.end(), [processor](auto& a)
				{
					return a == processor;
				});

				assert(it != mProcessors.end()); // processor not registered

				if(it != mProcessors.end())
				{
					mProcessors.erase(it);
				}
			});
		}
	}
}
