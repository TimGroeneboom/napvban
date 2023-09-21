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

