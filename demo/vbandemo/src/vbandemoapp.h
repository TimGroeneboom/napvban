#pragma once

// Core includes
#include <nap/resourcemanager.h>
#include <nap/resourceptr.h>

// Module includes
#include <renderservice.h>
#include <imguiservice.h>
#include <sceneservice.h>
#include <inputservice.h>
#include <scene.h>
#include <renderwindow.h>
#include <entity.h>
#include <app.h>
#include <vbanstreamplayercomponent.h>
#include <vbanstreamsendercomponent.h>

namespace nap
{
	using namespace rtti;

    /**
     * VBAN demo demonstrates the use of the VBAN module, which enables a NAP application to send lossless low latency audio
     * over the network using UDP.
     * To play received audio, simple attach a VBANStreamPlayerComponent to a VBANPacketReceiver and hook it up to an AudioOutputComponent
     * To send audio, simple attach a VBANStreamSenderComponent to an AudioComponent to send its output over the network.
     * See VBAN protocol for protocol specifications : https://vb-audio.com/Voicemeeter/VBANProtocol_Specifications.pdf
     */
	class VBANDemoApp : public App
	{
		RTTI_ENABLE(App)
	public:
		/**
		 * Constructor
		 * @param core instance of the NAP core system
		 */
		VBANDemoApp(nap::Core& core) : App(core) { }
		
		/**
		 * Initialize all the services and app specific data structures
		 * @param error contains the error code when initialization fails
		 * @return if initialization succeeded
		*/
		bool init(utility::ErrorState& error) override;
		
		/**
		 * Update is called every frame, before render.
		 * @param deltaTime the time in seconds between calls
		 */
		void update(double deltaTime) override;

		/**
		 * Render is called after update. Use this call to render objects to a specific target
		 */
		void render() override;

		/**
		 * Called when the app receives a window message.
		 * @param windowEvent the window message that occurred
		 */
		void windowMessageReceived(WindowEventPtr windowEvent) override;
		
		/**
		 * Called when the app receives an input message (from a mouse, keyboard etc.)
		 * @param inputEvent the input event that occurred
		 */
		void inputMessageReceived(InputEventPtr inputEvent) override;
		
		/**
		 * Called when the app is shutting down after quit() has been invoked
		 * @return the application exit code, this is returned when the main loop is exited
		 */
		virtual int shutdown() override;

	private:
		ResourceManager*			mResourceManager = nullptr;		///< Manages all the loaded data
		RenderService*				mRenderService = nullptr;		///< Render Service that handles render calls
		SceneService*				mSceneService = nullptr;		///< Manages all the objects in the scene
		InputService*				mInputService = nullptr;		///< Input service for processing input
		IMGuiService*				mGuiService = nullptr;			///< Manages GUI related update / draw calls
		ObjectPtr<RenderWindow>		mRenderWindow;					///< Pointer to the render window	
		ObjectPtr<Scene>			mScene = nullptr;				///< Pointer to the main scene
        ObjectPtr<EntityInstance>	mVBANSenderEntity = nullptr;    ///< Pointer to VBAN sender entity
        ObjectPtr<EntityInstance>	mVBANReceiverEntity = nullptr;  ///< Pointer to VBAN receiver entity

        // used to draw histogram of received audio signal
        std::vector<audio::ControllerValue> mPlotReceiverValues = { };
        uint32 mReceiverTickIdx = 0;

        // used to draw histogram of sent audio signal
        std::vector<audio::ControllerValue> mPlotSenderValues = { };
        uint32 mSenderTickIdx = 0;

		bool mSenderActive = true;
    };
}
