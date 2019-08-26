#ifndef __RIPPLE_DETECTOR_EDITOR_H
#define __RIPPLE_DETECTOR_EDITOR_H

#include <EditorHeaders.h> 

class RippleDetectorUi : public Component, public ButtonListener, public SliderListener, public ComboBoxListener
{
  public:
    RippleDetectorUi();
    ~RippleDetectorUi();
    void updateChannel(int channelCount);
    void updateSettings();
    void resized() override;
    void paint(Graphics &rInGraphics) override;
    void buttonClicked(Button *pInButtonClicked) override;
    void sliderValueChanged(Slider *pInSliderChanged) override;
    void comboBoxChanged(ComboBox *pInComboBoxChanged) override;

    LookAndFeel_V2 *_defaultLookAndFeel;
    ComboBox *_channelSelection;
    ComboBox *_outChannelSelection;
    Slider *_thresholdAmplitude;
    Slider *_refractoryRmsSamples;
    Slider *_rmsSamples;
    TextButton *_calibrateButton;

    // labels
    Label *_inputChannelLabel;
    Label *_outChannelLabel;
    Label *_thresholdAmpLabel;
    Label *_refractoryRmsSamplesLabel;
    Label *_rmsSamplesLabel;

    // facade
    int _channelCount;
    int _channel;
    int _outChannel;
    bool _calibrate;
    unsigned int _thresholdAmp;
    unsigned int _rmsRefractionCount;
    unsigned int _rmsSamplesCount;

  private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RippleDetectorUi);
};

class RippleDetectorEditor : public GenericEditor
{
  public:
    RippleDetectorEditor();
    virtual ~RippleDetectorEditor();
    RippleDetectorEditor(GenericProcessor *rInParentNode, bool useDefaultParameterEditors);
    void updateSettings() override;
    void resized() override;
    RippleDetectorUi _pluginUi;

  private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RippleDetectorEditor);
};

#endif
