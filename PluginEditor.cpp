#include "PluginEditor.h"

namespace
{
    const juce::Colour gold { 0xffd9b45a };
}

WorshipVoiceFXEditor::WorshipVoiceFXEditor (WorshipVoiceFXProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    laf.setColour (juce::Slider::rotarySliderFillColourId, gold);
    laf.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff343a48));
    laf.setColour (juce::Slider::thumbColourId, juce::Colour (0xfff2e2b0));
    laf.setColour (juce::Slider::textBoxTextColourId, juce::Colours::white.withAlpha (0.9f));
    laf.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    laf.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    laf.setColour (juce::Label::textColourId, juce::Colour (0xffb8bfcc));
    laf.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff232938));
    laf.setColour (juce::ComboBox::textColourId, juce::Colours::white);
    laf.setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff3c4458));
    laf.setColour (juce::ComboBox::arrowColourId, gold);
    laf.setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff1b2030));
    laf.setColour (juce::PopupMenu::textColourId, juce::Colours::white);
    laf.setColour (juce::PopupMenu::highlightedBackgroundColourId, gold.darker (0.5f));
    setLookAndFeel (&laf);

    addSection ("ENTRADA",  { { "inGain", "Entrada" }, { "lowCut", "Low Cut" },
                              { "deEss", "De-Esser" }, { "sat", "Saturação" } });
    addSection ("DINÂMICA", { { "compThresh", "Threshold" }, { "compRatio", "Ratio" },
                              { "compMakeup", "Makeup" } });
    addSection ("TOM",      { { "warmth", "Calor" }, { "presence", "Presença" },
                              { "air", "Ar" }, { "chorus", "Coro" } });
    addSection ("REVERB",   { { "revSize", "Tamanho" }, { "revDamp", "Damping" }, { "revPre", "Pré-delay" },
                              { "revTone", "Tom" }, { "revMix", "Mix" } });
    addSection ("DELAY",    { { "dlyTime", "Tempo" }, { "dlyFb", "Feedback" },
                              { "dlyTone", "Tom" }, { "dlyMix", "Mix" } });
    addSection ("SAÍDA",    { { "mix", "Mix Global" }, { "outGain", "Saída" } });

    // Sincronia do delay com o BPM da DAW
    if (auto* cp = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter ("dlySync")))
        syncBox.addItemList (cp->choices, 1);
    addAndMakeVisible (syncBox);
    syncAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        proc.apvts, "dlySync", syncBox);

    // Presets
    for (int i = 0; i < proc.getNumPrograms(); ++i)
        presetBox.addItem (proc.getProgramName (i), i + 1);
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
    presetBox.onChange = [this] { proc.setCurrentProgram (presetBox.getSelectedId() - 1); };
    addAndMakeVisible (presetBox);

    setSize (1000, 560);
}

WorshipVoiceFXEditor::~WorshipVoiceFXEditor()
{
    setLookAndFeel (nullptr);
}

void WorshipVoiceFXEditor::addSection (const char* title, std::initializer_list<Def> defs)
{
    Section s;
    s.title = juce::String::fromUTF8 (title);

    for (auto& d : defs)
    {
        auto& k = knobs.emplace_back();
        k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 72, 16);
        k.label.setText (juce::String::fromUTF8 (d.name), juce::dontSendNotification);
        k.label.setJustificationType (juce::Justification::centred);
        k.label.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (k.slider);
        addAndMakeVisible (k.label);
        k.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            proc.apvts, d.id, k.slider);
        s.knobs.push_back (&k);
    }
    sections.push_back (std::move (s));
}

void WorshipVoiceFXEditor::paint (juce::Graphics& g)
{
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff12141a), 0.0f, 0.0f,
                                             juce::Colour (0xff1d2230), 0.0f, (float) getHeight(), false));
    g.fillAll();

    g.setColour (gold);
    g.setFont (24.0f);
    g.drawText ("WORSHIP VOICE FX", 18, 10, 420, 30, juce::Justification::centredLeft);
    g.setColour (juce::Colour (0xff8a93a6));
    g.setFont (12.0f);
    g.drawText (juce::String::fromUTF8 ("Cadeia vocal para louvor"), 20, 38, 420, 16,
                juce::Justification::centredLeft);

    for (auto& s : sections)
    {
        auto a = s.area.toFloat();
        g.setColour (juce::Colour (0xff1a1f2b));
        g.fillRoundedRectangle (a, 10.0f);
        g.setColour (juce::Colour (0xff2c3345));
        g.drawRoundedRectangle (a.reduced (0.5f), 10.0f, 1.0f);
        g.setColour (gold);
        g.setFont (13.0f);
        g.drawText (s.title, s.area.getX() + 14, s.area.getY() + 6, 160, 20, juce::Justification::centredLeft);
    }
}

void WorshipVoiceFXEditor::resized()
{
    auto r = getLocalBounds().reduced (14);
    auto header = r.removeFromTop (48);
    presetBox.setBounds (header.removeFromRight (240).withSizeKeepingCentre (240, 28));
    r.removeFromTop (6);

    const int gap = 8;
    const int rowH = (r.getHeight() - gap) / 2;

    auto layoutRow = [&] (juce::Rectangle<int> row, size_t first, size_t last)
    {
        int total = 0;
        for (size_t i = first; i <= last; ++i)
            total += (int) sections[i].knobs.size();

        const int avail = row.getWidth() - gap * (int) (last - first);

        for (size_t i = first; i <= last; ++i)
        {
            const int n = (int) sections[i].knobs.size();
            auto area = row.removeFromLeft (i == last ? row.getWidth() : avail * n / total);
            row.removeFromLeft (gap);
            sections[i].area = area;

            auto inner = area.reduced (6);
            inner.removeFromTop (26);
            const int kw = inner.getWidth() / n;

            for (auto* k : sections[i].knobs)
            {
                auto cell = inner.removeFromLeft (kw);
                k->label.setBounds (cell.removeFromTop (18));
                k->slider.setBounds (cell);
            }
        }
    };

    layoutRow (r.removeFromTop (rowH), 0, 2);
    r.removeFromTop (gap);
    layoutRow (r.removeFromTop (rowH), 3, 5);

    // seletor de sincronia no cabeçalho da seção DELAY
    auto d = sections[4].area;
    syncBox.setBounds (d.getRight() - 124, d.getY() + 5, 114, 20);
}
