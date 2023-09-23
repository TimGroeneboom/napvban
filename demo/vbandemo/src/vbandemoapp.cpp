#include "vbandemoapp.h"
#include "audio/component/levelmetercomponent.h"

// External Includes
#include <utility/fileutils.h>
#include <nap/logger.h>
#include <inputrouter.h>
#include <rendergnomoncomponent.h>
#include <perspcameracomponent.h>
#include <audio/component/playbackcomponent.h>


RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::VBANDemoApp)
	RTTI_CONSTRUCTOR(nap::Core&)
RTTI_END_CLASS

namespace nap 
{
	bool VBANDemoApp::init(utility::ErrorState& error)
	{
		// Retrieve services
		mRenderService	= getCore().getService<nap::RenderService>();
		mSceneService	= getCore().getService<nap::SceneService>();
		mInputService	= getCore().getService<nap::InputService>();
		mGuiService		= getCore().getService<nap::IMGuiService>();

		// Fetch the resource manager
		mResourceManager = getCore().getResourceManager();

		// Get the render window
		mRenderWindow = mResourceManager->findObject<nap::RenderWindow>("Window");
		if (!error.check(mRenderWindow != nullptr, "unable to find render window with name: %s", "Window"))
			return false;

		// Get the scene that contains our entities and components
		mScene = mResourceManager->findObject<Scene>("Scene");
		if (!error.check(mScene != nullptr, "unable to find scene with name: %s", "Scene"))
			return false;

        // Get VBANReceiver Entity
        mVBANReceiverEntity = mScene->findEntity("VBANReceiverEntity");
        if (!error.check(mVBANReceiverEntity != nullptr, "unable to find entity with name: %s", "VBANReceiverEntity"))
            return false;

        // Get VBANSender Entity
        mVBANSenderEntity = mScene->findEntity("VBANSenderEntity");
        if (!error.check(mVBANSenderEntity != nullptr, "unable to find entity with name: %s", "VBANSenderEntity"))
            return false;

        // Resize the vectors containing the results of the analysis
        mPlotReceiverValues.resize(512, 0);
        mReceiverTickIdx = 0;
        mPlotSenderValues.resize(512, 0);
        mSenderTickIdx = 0;

        capFramerate(true);

		// All done!
		return true;
	}
	
	
	// Update app
	void VBANDemoApp::update(double deltaTime)
	{
		// Use a default input router to forward input events (recursively) to all input components in the default scene
		nap::DefaultInputRouter input_router(true);
		mInputService->processWindowEvents(*mRenderWindow, input_router, { &mScene->getRootEntity() });

        /**
         * Draw the windows displaying info about the VBAN sender and receiver
         */
        const auto& pallete = getCore().getService<IMGuiService>()->getPalette();

        ImGui::Begin("VBAN Receiver");
        ImGui::PushID("VBAN Receiver");

        auto& vban_stream_player_instance = mVBANReceiverEntity->getComponent<audio::VBANStreamPlayerComponentInstance>();
        auto* vban_stream_player_component = vban_stream_player_instance.getComponent<audio::VBANStreamPlayerComponent>();

        ImGui::Text("VBAN Receiver");

        ImGui::Text("Listening for VBAN packets on port:");
        ImGui::SameLine();
        ImGui::TextColored(pallete.mHighlightColor3, "%i", vban_stream_player_component->mVBANPacketReceiver->mServer->mPort);

        ImGui::Text("Listening to stream:");
        ImGui::SameLine();
        ImGui::TextColored(pallete.mHighlightColor3, "%s", vban_stream_player_component->mStreamName.c_str());

        ImGui::Text("Allowed latency in samples:");
        ImGui::SameLine();
        ImGui::TextColored(pallete.mHighlightColor3, "%i", vban_stream_player_component->mMaxBufferSize);

        ImGui::Spacing();
        ImGui::Text("Received Audio (Channel 0)");
        auto receiver_level_meter = mVBANReceiverEntity->findComponent<audio::LevelMeterComponentInstance>();

        // Store new value in array
        mPlotReceiverValues[mReceiverTickIdx] = receiver_level_meter->getLevel();	// save new value so it can be subtracted later
        if (++mReceiverTickIdx == mPlotReceiverValues.size())			// increment current sample index
            mReceiverTickIdx = 0;

        ImGui::PlotHistogram("",
                             mPlotReceiverValues.data(),
                             mPlotReceiverValues.size(),
                             mReceiverTickIdx, nullptr, 0.0f, 0.2f,
                             ImVec2(ImGui::GetColumnWidth(), 128)); // Plot the output values



        ImGui::PopID();
        ImGui::End();

        ImGui::Begin("VBAN Sender");
        ImGui::PushID("VBAN Sender");

        auto& playback_component_instance = mVBANSenderEntity->getComponent<audio::PlaybackComponentInstance>();
        auto& vban_stream_sender_component_instance = mVBANSenderEntity->getComponent<audio::VBANStreamSenderComponentInstance>();
        auto* vban_stream_sender_component = vban_stream_sender_component_instance.getComponent<audio::VBANStreamSenderComponent>();

        ImGui::Text("VBAN Sender");

        ImGui::Text("Sending for VBAN packets to :");
        ImGui::SameLine();
        ImGui::TextColored(pallete.mHighlightColor3, "%s:%i",
                           vban_stream_sender_component->mUdpClient->mEndpoint.c_str(),
                           vban_stream_sender_component->mUdpClient->mPort);

        ImGui::Text("Sending to stream:");
        ImGui::SameLine();
        ImGui::TextColored(pallete.mHighlightColor3, "%s", vban_stream_sender_component->mStreamName.c_str());

        bool play = playback_component_instance.isPlaying();
        if(ImGui::Checkbox("Play", &play))
        {
            if(play)
            {
                playback_component_instance.start(0);
            }else
            {
                playback_component_instance.stop();
            }
        }

        ImGui::Spacing();

        ImGui::Text("Sending Audio (Channel 0)");

        auto sender_level_meter = mVBANSenderEntity->findComponent<audio::LevelMeterComponentInstance>();

        // Store new value in array
        mPlotSenderValues[mSenderTickIdx] = sender_level_meter->getLevel();	// save new value so it can be subtracted later
        if (++mSenderTickIdx == mPlotSenderValues.size())			// increment current sample index
            mSenderTickIdx = 0;

        ImGui::PlotHistogram("",
                             mPlotSenderValues.data(),
                             mPlotSenderValues.size(),
                             mSenderTickIdx, nullptr, 0.0f, 0.2f,
                             ImVec2(ImGui::GetColumnWidth(), 128)); // Plot the output values



        ImGui::PopID();
        ImGui::End();
	}
	
	
	// Render app
	void VBANDemoApp::render()
	{
		// Signal the beginning of a new frame, allowing it to be recorded.
		// The system might wait until all commands that were previously associated with the new frame have been processed on the GPU.
		// Multiple frames are in flight at the same time, but if the graphics load is heavy the system might wait here to ensure resources are available.
		mRenderService->beginFrame();

		// Begin recording the render commands for the main render window
		if (mRenderService->beginRecording(*mRenderWindow))
		{
			// Begin render pass
			mRenderWindow->beginRendering();

			// Render GUI elements
			mGuiService->draw();

			// Stop render pass
			mRenderWindow->endRendering();

			// End recording
			mRenderService->endRecording();
		}

		// Proceed to next frame
		mRenderService->endFrame();
	}
	

	void VBANDemoApp::windowMessageReceived(WindowEventPtr windowEvent)
	{
		mRenderService->addEvent(std::move(windowEvent));
	}
	
	
	void VBANDemoApp::inputMessageReceived(InputEventPtr inputEvent)
	{
		if (inputEvent->get_type().is_derived_from(RTTI_OF(nap::KeyPressEvent)))
		{
			// If we pressed escape, quit the loop
			nap::KeyPressEvent* press_event = static_cast<nap::KeyPressEvent*>(inputEvent.get());
			if (press_event->mKey == nap::EKeyCode::KEY_ESCAPE)
				quit();

			// f is pressed, toggle full-screen
			if (press_event->mKey == nap::EKeyCode::KEY_f)
				mRenderWindow->toggleFullscreen();
		}
		// Add event, so it can be forwarded on update
		mInputService->addEvent(std::move(inputEvent));
	}

	
	int VBANDemoApp::shutdown()
	{
		return 0;
	}

}
