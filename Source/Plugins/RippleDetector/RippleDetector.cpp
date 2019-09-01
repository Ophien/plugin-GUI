#include "RippleDetector.h"

#define DEFAULT_RMS_SIZE 10
#define DEFAULT_THRESHOLD_AMP 1
#define DEFAULT__REFRACTORY_COUNT 100
#define DEFAULT_CALIBRATION_BUFFERS_COUNT 500

RippleDetector::RippleDetector()
    : GenericProcessor("RippleDetector")
{
    _calibrationBuffers = DEFAULT_CALIBRATION_BUFFERS_COUNT;
    _currentBuffer = 0;
    _rmsSamplesSinceDetection = 0;
    _mean = 0.0;
    _standardDeviation = 0.0;
    _threshold = 0.0;
    _thresholdAmp = 1.0f;
    _isPluginEnabled = true;
    _detectionEnabled = true;
    _isCalibrating = true;

    setProcessorType(PROCESSOR_TYPE_FILTER);
    createEventChannels();
}

RippleDetector::~RippleDetector()
{
}

bool RippleDetector::enable()
{
    return true;
}

void RippleDetector::createEventChannels()
{
    const DataChannel *in = getDataChannel(0);
    _pTtlEventChannel = new EventChannel(EventChannel::TTL, 8, 1, (in) ? in->getSampleRate() : CoreServices::getGlobalSampleRate(), this);
    MetaDataDescriptor md(MetaDataDescriptor::CHAR, 34, "High frequency detection type", "Description of the frequency", "channelInfo.extra");
    MetaDataValue mv(md);
    _pTtlEventChannel->addMetaData(md, mv);

    if (in)
    {
        md = MetaDataDescriptor(MetaDataDescriptor::UINT16,
                                3,
                                "Detection module",
                                "Index at its source, Source processor ID and Sub Processor index of the channel that triggers this event",
                                "source.channel.identifier.full");
        mv = MetaDataValue(md);
        uint16 source_info[3];
        source_info[0] = in->getSourceIndex();
        source_info[1] = in->getSourceNodeID();
        source_info[2] = in->getSubProcessorIdx();
        mv.setValue(static_cast<const uint16 *>(source_info));
        _pTtlEventChannel->addMetaData(md, mv);
    }

    eventChannelArray.add(_pTtlEventChannel);
}

void RippleDetector::updateSettings()
{
    const DataChannel *in = getDataChannel(0);
    _pTtlEventChannel = new EventChannel(EventChannel::TTL, 8, 1, (in) ? in->getSampleRate() : CoreServices::getGlobalSampleRate(), this);
    MetaDataDescriptor md(MetaDataDescriptor::CHAR, 34, "High frequency detection type", "Description of the frequency", "channelInfo.extra");
    MetaDataValue mv(md);
    _pTtlEventChannel->addMetaData(md, mv);

    if (in)
    {
        md = MetaDataDescriptor(MetaDataDescriptor::UINT16,
                                3,
                                "Detection module",
                                "Index at its source, Source processor ID and Sub Processor index of the channel that triggers this event",
                                "source.channel.identifier.full");
        mv = MetaDataValue(md);
        uint16 source_info[3];
        source_info[0] = in->getSourceIndex();
        source_info[1] = in->getSourceNodeID();
        source_info[2] = in->getSubProcessorIdx();
        mv.setValue(static_cast<const uint16 *>(source_info));
        _pTtlEventChannel->addMetaData(md, mv);
    }

    eventChannelArray.add(_pTtlEventChannel);
}

AudioProcessorEditor *RippleDetector::createEditor()
{
    _pRippleDetectorEditor = new RippleDetectorEditor(this, true);
    editor = _pRippleDetectorEditor;
    return editor;
}

void RippleDetector::sendTtlEvent(int rmsIndex, int val)
{
    // send event only when the animal is not moving
    if (!_isPluginEnabled)
        return;

    // timestamp for this sample
    uint64 time_stamp = getTimestamp(_channel) + rmsIndex;

    uint8 ttlData;
    uint8 output_event_channel = _outputChannel;
    ttlData = val << _outputChannel;
    TTLEventPtr ttl = TTLEvent::createTTLEvent(_pTtlEventChannel, time_stamp, &ttlData, sizeof(uint8), output_event_channel);
    addEvent(_pTtlEventChannel, ttl, rmsIndex);
}

void RippleDetector::detectRipples(std::vector<double> &rInRmsBuffer)
{
    for (unsigned int rms_sample = 0; rms_sample < rInRmsBuffer.size(); rms_sample++)
    {
        double sample = rInRmsBuffer[rms_sample];

        if (_detectionEnabled && sample > _threshold)
        {
            sendTtlEvent(rms_sample, 1);
            _detected = true;
            _detectionEnabled = false;
        }

        // enable detection again
        if (_detected && sample < _threshold)
        { 
            // start resting with refractory count
            _refractoryTime = true;
            _rmsSamplesSinceDetection = 0;

            // disable this state and start refractory count
            _detected = false;

            // mark event in the lfp viewer
            sendTtlEvent(rms_sample, 0);
        }

        // check if refractory count to enable detection again
        if (_rmsSamplesSinceDetection > _rmsRefractionCount)
        {
            _refractoryTime = false;
            _detectionEnabled = true;
        }

        // count rms samples since last detection
        if (_refractoryTime)
        {
            _rmsSamplesSinceDetection += 1;
        }
    }
}

void RippleDetector::calibrate()
{
    if (_currentBuffer > _calibrationBuffers)
    {
        printf("Finished calibration...\n");

        // set flag to false to end the calibration period
        _isCalibrating = false;

        // calculate statistics
        _mean = _mean / (double)_calibrationRms.size();

        // calculate standard deviation
        for (unsigned int rms_sample = 0; rms_sample < _calibrationRms.size(); rms_sample++)
        {
            _standardDeviation += pow(_calibrationRms[rms_sample] - _mean, 2.0);
        }
        _standardDeviation = sqrt(_standardDeviation / ((double)_calibrationRms.size() - 1));

        // define _threshold
        _threshold = _mean + _thresholdAmp * _standardDeviation;

        // printf calculated statistics
        printf("Mean: %f\n"
               "Deviation: %f\n"
               "Threshold amplifier %f\n"
               "Calculated Threshold: %f\n",
               _mean, _standardDeviation, _thresholdAmp, _threshold);
    }
}

double RippleDetector::calculateRms(const float *rInBuffer, int index)
{
    double sum = 0.0;

    for (int cnt = index; cnt < _rmsSize; cnt++)
    {
        sum += pow(rInBuffer[cnt], 2.0);
    }

    double rms = sqrt(sum / _rmsSize);

    return rms;
}

void RippleDetector::process(AudioSampleBuffer &rInBuffer)
{
    // update parameters according to UI
    _outputChannel = _pRippleDetectorEditor->_pluginUi._outChannel - 1;
    _channel = _pRippleDetectorEditor->_pluginUi._channel - 1;
    _thresholdAmp = _pRippleDetectorEditor->_pluginUi._thresholdAmp;
    _rmsRefractionCount = _pRippleDetectorEditor->_pluginUi._rmsRefractionCount;
    _rmsSize = _pRippleDetectorEditor->_pluginUi._rmsSamplesCount;

    if (_pRippleDetectorEditor->_pluginUi._calibrate == true)
    {
        printf("recalibrating...\n");
        _pRippleDetectorEditor->_pluginUi._calibrate = false;
        _isCalibrating = true;
        _currentBuffer = 0;

        // reset calibration RMS array
        _calibrationRms.clear();
    }

    // check if parameters are valid
    if (_rmsRefractionCount <= DEFAULT__REFRACTORY_COUNT)
        _rmsRefractionCount = DEFAULT__REFRACTORY_COUNT;

    if (_rmsSize <= DEFAULT_RMS_SIZE)
        _rmsSize = DEFAULT_RMS_SIZE;

    if (_thresholdAmp <= DEFAULT_THRESHOLD_AMP)
        _thresholdAmp = DEFAULT_THRESHOLD_AMP;

    // Get accelerometer raw data
    const float *rSamples = rInBuffer.getReadPointer(_channel);

    // guarante that the RMS buffer will be clean
    _localRms.clear();

    // Generate RMS buffer
    for (int rms_index = 0; rms_index < rInBuffer.getNumSamples(); rms_index += _rmsSize)
    {
        if (rms_index + _rmsSize > rInBuffer.getNumSamples())
        {
            break;
        }

        // RMS calculation
        double rms = calculateRms(rSamples, rms_index);

        // Calculate average between RMSs to determine baseline _threshold
        if (_isCalibrating)
        {
            // Add variables to be used as a calibration basis
            _calibrationRms.push_back(rms);
            _mean += rms;
        }
        else
        {
            // Set buffer value
            _localRms.push_back(rms);
        }
    }

    // check if plugin needs to stop calibration and calculate its statistics for _threshold estimation
    if (_isCalibrating)
    {
        // printf which calibration buffer is being used
        printf("Ripple Detection Calibration buffer sample: %d out of %d\n", _currentBuffer, _calibrationBuffers);

        calibrate();
    }
    else
    {
        detectRipples(_localRms);
    }

    // count how many buffers have processed
    _currentBuffer++;

    // Check if any event ocurred and tells this plugin to call the handleEventsFunction()
    checkForEvents();
}

void RippleDetector::handleEvent(const EventChannel *rInEventInfo, const MidiMessage &rInEvent, int samplePosition)
{
    // handle plugin control events
    _isPluginEnabled = true;

    if (Event::getEventType(rInEvent) == EventChannel::TEXT)
    {
        TextEventPtr text_event = TextEvent::deserializeFromMessage(rInEvent, rInEventInfo);

        std::string msg = std::string((char *)text_event->getRawDataPointer());

        printf("sample: %Ld message: %s\n", text_event->getTimestamp(), msg.c_str());

        if (strcmp(msg.c_str(), "movement_detected") == 0)
        {
            _isPluginEnabled = false;
        }

        if (strcmp(msg.c_str(), "detected_sharpwave") == 0)
        {
            printf("Captured sharpwave from the ripple detection plugin\n");

            // if detected sharpwave and ripple do logic such as emitting ttl to hardware
            if (_detected)
            {
                //TODO::implement hardware logic

                printf("Detected ripple and sharp wave simultaneusly\n");
            }
        }
    }
}