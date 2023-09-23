#include "vbanutils.h"

namespace nap
{
    bool utility::getVBANSampleRateFormatFromSampleRate(uint8_t& srFormat, int sampleRate, utility::ErrorState& errorState)
    {
        for(int i = 0; i < VBAN_SR_MAXNUMBER; i++)
        {
            if (sampleRate==VBanSRList[i])
            {
                srFormat = i;
                return true;
            }
        }

        errorState.fail("Could not find VBAN sample rate format for samplerate %i", sampleRate);
        return false;
    }


    bool utility::getSampleRateFromVBANSampleRateFormat(int& sampleRate, uint8_t srFormat, utility::ErrorState& errorState)
    {
        if(srFormat >= 0 && srFormat < VBAN_SR_MAXNUMBER)
        {
            sampleRate = VBanSRList[srFormat];
            return true;
        }

        errorState.fail("Could not find samplerate for VBAN sample rate format %i", srFormat);
        return false;
    }
}