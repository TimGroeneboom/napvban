/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include <utility/errorstate.h>
#include "vban/vban.h"

namespace nap
{
	namespace utility
	{
		/**
		 * Translates given samplerate to VBAN sample rate format, returns true on success
		 * @param srFormat reference to sample rate format
		 * @param sampleRate the samplerate
		 * @param errorState contains any errors
		 * @return true on success
		 */
		bool getVBANSampleRateFormatFromSampleRate(uint8_t& srFormat, int sampleRate, utility::ErrorState& errorState);

		/**
		 * Translates VBAN sample rate format to sample rate, returns true on success
		 * @param sampleRate the samplerate
		 * @param srFormat the format
		 * @param errorState contains any errors
		 * @return true on success
		 */
		bool getSampleRateFromVBANSampleRateFormat(int& sampleRate, uint8_t srFormat, utility::ErrorState& errorState);
	}
}

