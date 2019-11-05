//************************************************************************
// file name: SsbDemodulator.cc
//************************************************************************
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>

#include "SsbDemodulator.h"

using namespace std;

// These coefficients are used for decimation by 4.
static float stage1DecimatorCoefficients[] =
{
   0.0273576,
   0.0863889,
   0.1643134,
   0.2208413,
   0.2208413,
   0.1643134,
   0.0863889,
   0.0273576
};

// These coefficients are used for decimation by 4.
static float stage2DecimatorCoefficients[] =
{
  -0.0193717,
  -0.0035725,
   0.0346554,
   0.1014222,
   0.1752242,
   0.2239757,
   0.2239757,
   0.1752242,
   0.1014222,
   0.0346554,
  -0.0035725,
  -0.0193717
};

// These coefficients are used for decimation by 2.
static float stage3DecimatorCoefficients[] =
{
  -0.0028484,
   0.0303700,
   0.0217396,
  -0.0308793,
  -0.0721113,
   0.0003860,
   0.1955291,
   0.3725031,
   0.3725031,
   0.1955291,
   0.0003860,
  -0.0721113,
  -0.0308793,
   0.0217396,
   0.0303700,
  -0.0028484
};

// These coefficients provide a delay line function.
static float delayLineCoefficients[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1};

// Hilbert transform coefficients smoothed by a Hamming window.
static float phaseShifterCoefficients[] =
{
  -0.0033953,
   0,
  -0.0058652,
   0,
  -0.0134385,
   0,
  -0.0281423,
   0,
  -0.0534836,
   0,
  -0.0980394,
   0,
  -0.1935638,
   0,
  -0.6302204,
   0,
   0.6302204,
   0,
   0.1935638,
   0,
   0.0980394,
   0,
   0.0534836,
   0,
   0.0281423,
   0,
   0.0134385,
   0,
   0.0058652,
   0,
   0.0033953
};

//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// Coefficients for dc removal IIR filter.
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
static float dcRemovalNumeratorCoefficients[] = {1, -1};
static float dcRemovalDenominatorCoefficients[] = {-0.95};
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

extern void nprintf(FILE *s,const char *formatPtr, ...);

/*****************************************************************************

  Name: SsbDemodulator

  Purpose: The purpose of this function is to serve as the contructor for
  an instance of an SsbDemodulator.

  Calling Sequence: SsbDemodulator()

  Inputs:

    None.

 Outputs:

    None.

*****************************************************************************/
SsbDemodulator::SsbDemodulator(void)
{
  int numberOfStage1DecimatorTaps;
  int numberOfStage2DecimatorTaps;
  int numberOfStage3DecimatorTaps;
  int numberOfDelayLineTaps;
  int numberOfPhaseShifterTaps;
  int numberOfDcRemovalNumeratorTaps;
  int numberOfDcRemovalDenominatorTaps;

  // Default to demodulating lower sideband signals.
  lsbDemodulationMode = true;

  // Set to a nominal gain.
  demodulatorGain = 300;

  numberOfStage1DecimatorTaps =
      sizeof(stage1DecimatorCoefficients) / sizeof(float);

  numberOfStage2DecimatorTaps =
      sizeof(stage2DecimatorCoefficients) / sizeof(float);

  numberOfStage3DecimatorTaps =
      sizeof(stage3DecimatorCoefficients) / sizeof(float);

  numberOfDelayLineTaps =
      sizeof(delayLineCoefficients) / sizeof(float);

  numberOfPhaseShifterTaps =
      sizeof(phaseShifterCoefficients) / sizeof(float);

  numberOfDcRemovalNumeratorTaps =
      sizeof(dcRemovalNumeratorCoefficients) / sizeof(float);

  numberOfDcRemovalDenominatorTaps =
      sizeof(dcRemovalDenominatorCoefficients) / sizeof(float);

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // This pair of decimators reduce the sample rate from 256000S/s to
  // 64000S/s.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Allocate the decimator for the in-phase component.
  stage1IDecimatorPtr = new Decimator(numberOfStage1DecimatorTaps,
                                      stage1DecimatorCoefficients,
                                      4);

  // Allocate the decimator for the quadrature component.
  stage1QDecimatorPtr = new Decimator(numberOfStage1DecimatorTaps,
                                      stage1DecimatorCoefficients,
                                      4);
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // This pair of decimators reduce the sample rate from 64000S/s to
  // 16000S/s.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Allocate the decimator for the in-phase component.
  stage2IDecimatorPtr = new Decimator(numberOfStage2DecimatorTaps,
                                      stage2DecimatorCoefficients,
                                      4);

  // Allocate the decimator for the quadrature component.
  stage2QDecimatorPtr = new Decimator(numberOfStage2DecimatorTaps,
                                      stage2DecimatorCoefficients,
                                      4);
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // This pair of decimators reduce the sample rate from 16000S/s to
  // 8000S/s.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Allocate the decimator for the in-phase component.
  stage3IDecimatorPtr = new Decimator(numberOfStage3DecimatorTaps,
                                      stage3DecimatorCoefficients,
                                      2);

  // Allocate the decimator for the quadrature component.
  stage3QDecimatorPtr = new Decimator(numberOfStage3DecimatorTaps,
                                      stage3DecimatorCoefficients,
                                      2);
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // This pair of filters provide the required processing so that the
  // SSB signal can be demodulated.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Allocate the delay line for the in-phase component.
  delayLinePtr = new FirFilter(numberOfDelayLineTaps,
                               delayLineCoefficients);

  // Allocate the phase shifter for the quadrature component.
  phaseShifterPtr = new FirFilter(numberOfPhaseShifterTaps,
                                  phaseShifterCoefficients);
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Instantiate IIR filter state for dc removal.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  dcRemovalFilterPtr = new IirFilter(numberOfDcRemovalNumeratorTaps,
                                     dcRemovalNumeratorCoefficients,
                                     numberOfDcRemovalDenominatorTaps,
                                     dcRemovalDenominatorCoefficients);
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

  return;

} // SsbDemodulator

/*****************************************************************************

  Name: ~SsbDemodulator

  Purpose: The purpose of this function is to serve as the destructor for
  an instance of an SsbDemodulator.

  Calling Sequence: ~SsbDemodulator()

  Inputs:

    None.

  Outputs:

    None.

*****************************************************************************/
SsbDemodulator::~SsbDemodulator(void)
{

  // Release resources.
  delete stage1IDecimatorPtr;
  delete stage1QDecimatorPtr;
  delete stage2IDecimatorPtr;
  delete stage2QDecimatorPtr;
  delete stage3IDecimatorPtr;
  delete stage3QDecimatorPtr;
  delete delayLinePtr;
  delete phaseShifterPtr;
  delete dcRemovalFilterPtr;

  return;

} // ~SsbDemodulator

/*****************************************************************************

  Name: resetDemodulator

  Purpose: The purpose of this function is to reset the demodulator to its
  initial condition.

  Calling Sequence: resetDemodulator()

  Inputs:

    None.

  Outputs:

    None.

*****************************************************************************/
void SsbDemodulator::resetDemodulator(void)
{

  // Reset all filter structures to their initial conditions.
  stage1IDecimatorPtr->resetFilterState();
  stage1QDecimatorPtr->resetFilterState();
  stage2IDecimatorPtr->resetFilterState();
  stage2QDecimatorPtr->resetFilterState();
  stage3IDecimatorPtr->resetFilterState();
  stage3QDecimatorPtr->resetFilterState();
  delayLinePtr->resetFilterState();
  phaseShifterPtr->resetFilterState();
  dcRemovalFilterPtr->resetFilterState();

  return;

} // resetDemodulator

/*****************************************************************************

  Name: setLsbDemodulationMode

  Purpose: The purpose of this function is to set the demodulator mode so
  that lower sideband demodulation is performed.

  Calling Sequence: setLsbDemodulationMode()

  Inputs:

    None.

  Outputs:

    None.

*****************************************************************************/
void SsbDemodulator::setLsbDemodulationMode(void)
{

  // Indicate that lower sideband signals are to be demodulated.
  lsbDemodulationMode = true;

  return;

} // setLsbDemodulationMode

/*****************************************************************************

  Name: setUsbDemodulationMode

  Purpose: The purpose of this function is to set the demodulator mode so
  that upper sideband demodulation is performed.

  Calling Sequence: setUsbDemodulationMode()

  Inputs:

    None.

  Outputs:

    None.

*****************************************************************************/
void SsbDemodulator::setUsbDemodulationMode(void)
{

  // Indicate that upper sideband signals are to be demodulated.
  lsbDemodulationMode = false;

  return;

} // setUsbDemodulationMode

/*****************************************************************************

  Name: setDemodulatorGain

  Purpose: The purpose of this function is to set the gain of the
  demodulator.  This gain is used to convert the phase differences of the
  signal values into numbers that are later converted into PCM values.

  Calling Sequence: setDemodulatorGain(gain)

  Inputs:

    gain - The demodulator gain.

  Outputs:

    None.

*****************************************************************************/
void SsbDemodulator::setDemodulatorGain(float gain)
{

  this->demodulatorGain = gain;

  return;

} // setDemodulatorGain

/*****************************************************************************

  Name: acceptIqData

  Purpose: The purpose of this function is to perform the high level
  processing that is associated with the demodulation of a signal.

  Calling Sequence: acceptIqData(bufferPtr,bufferLength)

  Inputs:

    bufferPtr - A pointer to IQ data.

    bufferLength - The number of bytes contained in the buffer that is
    in the buffer.

  Outputs:

    None.

*****************************************************************************/
void SsbDemodulator::acceptIqData(int8_t *bufferPtr,uint32_t bufferLength)
{
  uint32_t sampleCount;

  // Decimate the signal to the demodulator sample rate.
  sampleCount = reduceSampleRate(bufferPtr,bufferLength);

  // Demodulate the decimated signal.
  sampleCount = demodulateSignal(sampleCount);

  // Create the PCM data stream.
  sampleCount = createPcmData(sampleCount);

  // Send the PCM data to the information sink.
  sendPcmData(sampleCount);

  return;

} // acceptIqData

/*****************************************************************************

  Name: reduceSampleRate

  Purpose: The purpose of this function is to accept interleaved IQ data
  and decimate the data by a factor of 32.  The decimated data will be stored
  in the iData[] and qData[] arrays.

  Calling Sequence: reduceSampleRate(bufferPtr,bufferLength)

  Inputs:

    bufferPtr - A pointer to IQ data.

    bufferLength - The number of bytes contained in the buffer.

  Outputs:

    sampleCount - The number of samples stored in the iData[] and qData[]
    arrays.

*****************************************************************************/
uint32_t SsbDemodulator::reduceSampleRate(int8_t *bufferPtr,uint32_t bufferLength)
{
  uint32_t i;
  uint32_t outputBufferIndex;
  bool sampleAvailable;
  float sample;

  // Set to reference the beginning of the in-phase output buffer.
  outputBufferIndex = 0;

  // Decimate the in-phase samples.
  for (i = 0; i < bufferLength; i += 2)
  {
    sampleAvailable = stage1IDecimatorPtr->decimate((float)bufferPtr[i],
                                                    &sample);

    if (sampleAvailable)
    {
      sampleAvailable = stage2IDecimatorPtr->decimate(sample,&sample);

      if (sampleAvailable)
      {
        sampleAvailable = stage3IDecimatorPtr->decimate(sample,&sample);

        if (sampleAvailable)
        {
          // Store the decimated sample.
          iData[outputBufferIndex] = sample;

          // Reference the next storage location.
          outputBufferIndex++;
        } // if
      } // if
    } // if
  } // for

  // Set to reference the beginning of the quadrature output buffer.
  outputBufferIndex = 0;

  // Decimate the quadrature samples.
  for (i = 1; i < (bufferLength + 1); i += 2)
  {
    sampleAvailable = stage1QDecimatorPtr->decimate((float)bufferPtr[i],
                                                    &sample);

    if (sampleAvailable)
    {
      sampleAvailable = stage2QDecimatorPtr->decimate(sample,&sample);

      if (sampleAvailable)
      {
        sampleAvailable = stage3QDecimatorPtr->decimate(sample,&sample);

        if (sampleAvailable)
        {
          // Store the decimated sample.
          qData[outputBufferIndex] = sample;

          // Reference the next storage location.
          outputBufferIndex++;
        } // if
      } // if
    } // if
  } // for

  return (outputBufferIndex);

} // reduceSampleRate

/*****************************************************************************

  Name: demodulateSignal

  Purpose: The purpose of this function is to demodulate an AM signal that
  is stored in the iData[] and qData[] arrays.
  The signal processing that is performed is now described.  First, the
  in-phase component is delayed by the number of samples required so that
  it compensates for the processing of the quadrature component.  Next, the
  quadrature component is processed by a Hilbert transformer so the analytic
  signal is appropriately processed.  Depending upon which sideband is
  desired, the processed Q component is either added or subtracted from the
  processed I component of t he signal.  For lower sideband demodulation,
  the processed Q component is added from the processed I component of
  the signal.  For upper sideband demodulation, the processed Q component is
  subtracted from the I component of the signal.  Finally, the dc offset is
  removed from the signal.  The demodulated value is then multiplied by the
  demodulator gain with the result that there exists a scaled demodulated
  signal.

  Calling Sequence: sampleCount = demodulateSignal(bufferLength)

  Inputs:

    bufferLength - The number of entries contained iData[] and qData buffers.

  Outputs:

    sampleCount - The number of samples stored in the demodulatedData[]
    array.

*****************************************************************************/
uint32_t SsbDemodulator::demodulateSignal(uint32_t bufferLength)
{
  uint32_t i;
  float iDelayed, qPhaseShifted;
  float outputValue;

  for (i = 0; i < bufferLength; i++)
  {
    // Delay in-phase component.
    iDelayed = delayLinePtr->filterData(iData[i]);

    // Phase shift quadrature component.
    qPhaseShifted = phaseShifterPtr->filterData(qData[i]);

    if (lsbDemodulationMode)
    {
      // Demodulate as lower sideband.
      outputValue = iDelayed + qPhaseShifted;
    } // if
    else
    {
      // Demodulate as upper sideband.
      outputValue = iDelayed - qPhaseShifted;
    } // else

    // Remove dc from the signal.
    outputValue = dcRemovalFilterPtr->filterData(outputValue);

    // Store the demodulated data.
    demodulatedData[i] = demodulatorGain * outputValue;

  } // for

  return (bufferLength);

} // demodulateSignal

/*****************************************************************************

  Name: createPcmData

  Purpose: The purpose of this function is to convert the signal, contained
  in demodulatedData[], into 16-bit little endian PCM data.

  Calling Sequence: sampleCount = createPcmData(bufferLength)

  Inputs:

    bufferLength - The number of entries contained in the demodulatedData[]
    buffer.

  Outputs:

    sampleCount - The number of samples stored in the pcmData[] array.

*****************************************************************************/
uint32_t SsbDemodulator::createPcmData(uint32_t bufferLength)
{
  uint32_t i;

  for (i = 0; i < bufferLength; i++)
  {
    // Store PCM sample with dc offset removed.
    pcmData[i] = (int16_t)demodulatedData[i];
  } // for

  return (bufferLength);

} // createPcmData

/*****************************************************************************

  Name: sendPcmData

  Purpose: The purpose of this function is to send the PCM data to the
  appropriate information sink.

  Calling Sequence: sendPcmData(bufferLength)

  Inputs:

    bufferLength - The number of entries contained in the pcmData[] buffer.

  Outputs:

    None.

*****************************************************************************/
void SsbDemodulator::sendPcmData(uint32_t bufferLength)
{
  uint32_t i;

  // Send the PCM samples to stdout for now.
  fwrite(pcmData,2,bufferLength,stdout);

  return;

} // sendPcmData

/**************************************************************************

  Name: displayInternalInformation

  Purpose: The purpose of this function is to display internal information
  in the AM demodulator.

  Calling Sequence: displayInternalInformation()

  Inputs:

    None.

  Outputs:

    None.

**************************************************************************/
void SsbDemodulator::displayInternalInformation(void)
{

  nprintf(stderr,"\n--------------------------------------------\n");
  nprintf(stderr,"SSB Demodulator Internal Information\n");
  nprintf(stderr,"--------------------------------------------\n");

  nprintf(stderr,"Demodulation Mode        : ");

  if (lsbDemodulationMode)
  {
    nprintf(stderr,"LSB\n");
  } // if
  else
  {
    nprintf(stderr,"USB\n");
  } // else

  nprintf(stderr,"Demodulator Gain         : %f\n",demodulatorGain);

  return;

} // displayInternalInformation


