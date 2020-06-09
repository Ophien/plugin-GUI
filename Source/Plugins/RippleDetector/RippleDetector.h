#ifndef __RIPPLE_DETECTOR_H
#define __RIPPLE_DETECTOR_H

#include <ProcessorHeaders.h>
#include <stdio.h>			 
#include <iostream>			  
#include <string>			  
#include <vector>
#include <time.h>

#include "RippleDetectorEditor.h" 

class RippleDetector : public GenericProcessor
{
  public:
    RippleDetector();
    ~RippleDetector();
    void process(AudioSampleBuffer &rInContinuousBuffer) override;
    bool enable() override;
    void handleEvent(const EventChannel *rInChannelInfo, const MidiMessage &rInEvent, int sampleNum) override;
    AudioProcessorEditor *createEditor() override;
    void createEventChannels() override;
    void updateSettings() override;

    // calibration function
    void calibrate();
    void detectRipples(std::vector<double> &rInRmsBuffer);
    void sendTtlEvent(int rmsIndex, int val);
    double calculateRms(const float *rInBuffer, int index);

    // utilized _channel to check for high frequencies
    int _outputChannel;
    int _channel;
    int _rmsSize;

    // rms statistics
    std::vector<double> _localRms;
    std::vector<double> _calibrationRms;
    double _mean;
    double _standardDeviation;

    // detection _threshold
    double _threshold;
    double _thresholdAmp;

    // store processing time in seconds
    int _currentBuffer;
    int _calibrationBuffers;

    // calibrating flag
    bool _isCalibrating;

    // Event count
    bool _refractoryTime;
    bool _detected;
    bool _detectionEnabled;
    bool _movementDetected;
    unsigned int _rmsSamplesSinceDetection;
    unsigned int _rmsRefractionCount;

    // event handle variables
    bool _isPluginEnabled;
    char *_pMessageString;
    int _messageSize;

    // ttl event _channel
    EventChannel *_pTtlEventChannel;

    // Editor
    RippleDetectorEditor *_pRippleDetectorEditor;

  private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RippleDetector);
};

#endif
