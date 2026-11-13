#pragma once

#include <deque>
#include "PluginProcessor.h"

class WorshipVoiceFXEditor : public juce::AudioProcessorEditor
{
public:
    explicit WorshipVoiceFXEditor (WorshipVoiceFXProcessor&);
    ~WorshipVoiceFXEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    struct Section
    {
        juce::String title;
        std::vector<Knob*> knobs;
        juce::Rectangle<int> area;
    };

    struct Def { const char* id; const char* name; };

    void addSection (const char* title, std::initializer_list<Def> defs);

    WorshipVoiceFXProcessor& proc;
    juce::LookAndFeel_V4 laf;

    std::deque<Knob> knobs;       // deque = endereços estáveis
    std::vector<Section> sections;

    juce::ComboBox presetBox, syncBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> syncAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WorshipVoiceFXEditor)
};
