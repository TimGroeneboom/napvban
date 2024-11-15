/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */


#include "vbanudpserver.h"

// Nap includes
#include <nap/logger.h>

// ASIO Includes
#include <asio/ip/udp.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>
#include <asio/io_service.hpp>

#include <thread>
#include <vban/vban.h>

RTTI_BEGIN_CLASS(nap::VBANUDPServer)
	RTTI_PROPERTY("Port",			        &nap::VBANUDPServer::mPort,			                nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("IP Address",		        &nap::VBANUDPServer::mIPAddress,	                nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("Multicast Groups",		&nap::VBANUDPServer::mMulticastGroups,	            nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("ReceiveBufferSize", &nap::VBANUDPServer::mReceiveBufferSize, nap::rtti::EPropertyMetaData::Default)
RTTI_END_CLASS

using namespace asio::ip;

namespace nap
{

	class VBANUDPServer::Impl
	{
	public:
		explicit Impl() {}

		// ASIO
		asio::io_context 			mIOContext;
		asio::ip::udp::endpoint 	mRemoteEndpoint;
		asio::ip::udp::socket       mSocket{ mIOContext };
	};

	VBANUDPServer::VBANUDPServer() = default;
	VBANUDPServer::~VBANUDPServer() = default;


	bool nap::VBANUDPServer::start(nap::utility::ErrorState &errorState)
	{
		// when asio error occurs, init_success indicates whether initialization should fail or succeed
		bool init_success = false;

		mImpl = std::make_unique<Impl>();

		// try to open socket
		asio::error_code errorCode;
		mImpl->mSocket.open(udp::v4(), errorCode);
		if (handleAsioError(errorCode, errorState, init_success))
			return init_success;

		// try to create ip address
		// when address property is left empty, bind to any local address
		asio::ip::address address;
		if (mIPAddress.empty())
		{
			address = asio::ip::address_v4::any();
		}
		else {
			address = asio::ip::make_address(mIPAddress,errorCode);
			if (handleAsioError(errorCode, errorState, init_success))
				return init_success;
		}

		nap::Logger::info(*this, "Listening at port %i", mPort);
		mImpl->mSocket.bind(udp::endpoint(address,mPort), errorCode);
		mImpl->mSocket.set_option(asio::ip::udp::socket::receive_buffer_size(mReceiveBufferSize));
		if (handleAsioError(errorCode, errorState, init_success))
			return init_success;

		for (const auto& multicast_group : mMulticastGroups)
		{
			auto multicast_address = make_address(multicast_group, errorCode);
			if (handleAsioError(errorCode, errorState, init_success))
				return init_success;

			mImpl->mSocket.set_option(multicast::join_group(multicast_address), errorCode);
			if (handleAsioError(errorCode, errorState, init_success))
				return init_success;
		}

		mRunning.store(true);
		mThread = std::make_unique<std::thread>([&](){
			while (mRunning.load())
				process();
		});

		return true;
	}


	void nap::VBANUDPServer::stop()
	{
		mRunning.store(false);
		mThread->join();

		asio::error_code asio_error_code;
		mImpl->mSocket.close(asio_error_code);

		if (asio_error_code)
		{
			nap::Logger::error(*this, asio_error_code.message());
		}

		// explicitly delete socket
		mImpl = nullptr;
	}


	void nap::VBANUDPServer::process()
	{
		auto available = mImpl->mSocket.available();
		if (available < VBAN_DATA_MAX_SIZE)
			return;

		asio::error_code asio_error_code;
		mBuffer.resize(VBAN_DATA_MAX_SIZE);

		uint len = mImpl->mSocket.receive(asio::buffer(mBuffer));
		if (len > 0)
		{
			assert(len <= VBAN_DATA_MAX_SIZE);
			std::lock_guard<std::mutex> lock(mMutex);
			const UDPPacket packet(std::move(mBuffer));

			packetReceived.trigger(packet);
		}

		if (asio_error_code)
			nap::Logger::error(*this, asio_error_code.message());
	}


	void nap::VBANUDPServer::registerListenerSlot(Slot<const nap::UDPPacket &>& slot)
	{
		std::lock_guard<std::mutex> lock(mMutex);
		packetReceived.connect(slot);
	}


	void nap::VBANUDPServer::removeListenerSlot(nap::Slot<const nap::UDPPacket &> &slot)
	{
		std::lock_guard<std::mutex> lock(mMutex);
		packetReceived.disconnect(slot);
	}


	bool nap::VBANUDPServer::handleAsioError(const std::error_code& errorCode, utility::ErrorState& errorState, bool& success)
	{
		if (errorCode)
		{
			success = false;
			errorState.fail(errorCode.message());
			return true;
		}

		return false;
	}


}