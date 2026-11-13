#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    bool changed (float& last, float now)
    {
        if (std::abs (last - now) < 1.0e-6f)
            return false;
        last = now;
        return true;
    }

    struct Preset
    {
        const char* name;
        std::vector<std::pair<const char*, float>> values; // só o que difere do padrão
    };

    const std::vector<Preset>& presets()
    {
        static const std::vector<Preset> list =
        {
            { "Worship Hall (padrão)", {} },

            { "Catedral",
              { { "revSize", 0.95f }, { "revDamp", 0.35f }, { "revPre", 60.0f }, { "revMix", 0.45f },
                { "dlySync", 4.0f }, { "dlyMix", 0.15f }, { "dlyFb", 0.35f }, { "air", 3.0f } } },

            { "Intimista / Acústico",
              { { "revSize", 0.35f }, { "revMix", 0.14f }, { "revPre", 15.0f }, { "dlySync", 2.0f },
                { "dlyMix", 0.06f }, { "warmth", 3.0f }, { "compRatio", 2.5f }, { "chorus", 0.0f } } },

            { "Ambiente (delay longo)",
              { { "dlySync", 3.0f }, { "dlyFb", 0.55f }, { "dlyMix", 0.30f }, { "dlyTone", 3500.0f },
                { "revSize", 0.85f }, { "revMix", 0.40f }, { "revPre", 45.0f }, { "chorus", 0.30f } } },

            { "Líder (presença)",
              { { "presence", 3.5f }, { "air", 4.0f }, { "compThresh", -22.0f }, { "compRatio", 4.0f },
                { "compMakeup", 5.0f }, { "deEss", 0.5f }, { "revSize", 0.55f }, { "revMix", 0.20f },
                { "dlyMix", 0.08f }, { "sat", 0.20f } } },

            { "Coro largo",
              { { "chorus", 0.6f }, { "revSize", 0.8f }, { "revMix", 0.35f }, { "revPre", 25.0f },
                { "warmth", 2.0f }, { "dlyMix", 0.10f } } },

            { "Neutro (limpo)",
              { { "deEss", 0.0f }, { "sat", 0.0f }, { "compRatio", 1.0f }, { "compMakeup", 0.0f },
                { "warmth", 0.0f }, { "presence", 0.0f }, { "air", 0.0f }, { "chorus", 0.0f },
                { "revMix", 0.0f }, { "dlyMix", 0.0f } } }
        };
        return list;
    }
}

//==============================================================================
WorshipVoiceFXProcessor::WorshipVoiceFXProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    auto g = [this] (const char* id) { return apvts.getRawParameterValue (id); };

    p.inGain = g ("inGain");         p.lowCut = g ("lowCut");       p.deEss = g ("deEss");
    p.sat = g ("sat");               p.compThresh = g ("compThresh"); p.compRatio = g ("compRatio");
    p.compMakeup = g ("compMakeup"); p.warmth = g ("warmth");       p.presence = g ("presence");
    p.air = g ("air");               p.chorus = g ("chorus");       p.revSize = g ("revSize");
    p.revDamp = g ("revDamp");       p.revPre = g ("revPre");       p.revTone = g ("revTone");
    p.revMix = g ("revMix");         p.dlySync = g ("dlySync");     p.dlyTime = g ("dlyTime");
    p.dlyFb = g ("dlyFb");           p.dlyTone = g ("dlyTone");     p.dlyMix = g ("dlyMix");
    p.mix = g ("mix");               p.outGain = g ("outGain");
}

juce::AudioProcessorValueTreeState::ParameterLayout WorshipVoiceFXProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    auto unit = [] (const char* u, int dec)
    {
        return [u, dec] (float v, int) { return juce::String (v, dec) + " " + u; };
    };
    auto pct = [] (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)) + " %"; };
    auto pctParse = [] (const juce::String& s) { return s.getFloatValue() / 100.0f; };

    auto add = [&] (const char* id, const char* name, float lo, float hi, float def,
                    std::function<juce::String (float, int)> fmt, float skew, bool percent)
    {
        auto attr = juce::AudioParameterFloatAttributes().withStringFromValueFunction (std::move (fmt));
        if (percent)
            attr = attr.withValueFromStringFunction (pctParse);

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name,
            juce::NormalisableRange<float> (lo, hi, 0.0f, skew), def, attr));
    };

    // Entrada
    add ("inGain",     "Entrada",    -24.0f, 24.0f,   0.0f, unit ("dB", 1), 1.0f, false);
    add ("lowCut",     "Low Cut",     20.0f, 400.0f, 80.0f, unit ("Hz", 0), 0.5f, false);
    add ("deEss",      "De-Esser",     0.0f, 1.0f,    0.3f, pct,            1.0f, true);
    add ("sat",        "Saturacao",    0.0f, 1.0f,    0.1f, pct,            1.0f, true);
    // Dinâmica
    add ("compThresh", "Threshold",  -40.0f, 0.0f,  -18.0f, unit ("dB", 1), 1.0f, false);
    add ("compRatio",  "Ratio",        1.0f, 10.0f,   3.0f, unit (":1", 1), 1.0f, false);
    add ("compMakeup", "Makeup",       0.0f, 18.0f,   3.0f, unit ("dB", 1), 1.0f, false);
    // Tom
    add ("warmth",     "Calor",       -9.0f, 9.0f,    1.5f, unit ("dB", 1), 1.0f, false);
    add ("presence",   "Presenca",    -9.0f, 9.0f,    1.5f, unit ("dB", 1), 1.0f, false);
    add ("air",        "Ar",          -9.0f, 9.0f,    2.5f, unit ("dB", 1), 1.0f, false);
    add ("chorus",     "Coro",         0.0f, 1.0f,   0.15f, pct,            1.0f, true);
    // Reverb
    add ("revSize",    "Tamanho",      0.0f, 1.0f,   0.75f, pct,            1.0f, true);
    add ("revDamp",    "Damping",      0.0f, 1.0f,    0.5f, pct,            1.0f, true);
    add ("revPre",     "Pre-delay",    0.0f, 200.0f, 30.0f, unit ("ms", 0), 1.0f, false);
    add ("revTone",    "Tom Reverb", 2000.0f, 16000.0f, 9000.0f, unit ("Hz", 0), 0.5f, false);
    add ("revMix",     "Reverb Mix",   0.0f, 1.0f,   0.28f, pct,            1.0f, true);
    // Delay
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "dlySync", 1 }, "Delay Sync",
        juce::StringArray { "Livre", "1/4", "1/8", "1/8 pont.", "1/2" }, 1));
    add ("dlyTime",    "Delay Tempo", 40.0f, 1000.0f, 350.0f, unit ("ms", 0), 0.6f, false);
    add ("dlyFb",      "Feedback",     0.0f, 0.9f,    0.3f, pct,            1.0f, true);
    add ("dlyTone",    "Tom Delay", 1000.0f, 12000.0f, 4500.0f, unit ("Hz", 0), 0.5f, false);
    add ("dlyMix",     "Delay Mix",    0.0f, 1.0f,   0.12f, pct,            1.0f, true);
    // Saída
    add ("mix",        "Mix Global",   0.0f, 1.0f,    1.0f, pct,            1.0f, true);
    add ("outGain",    "Saida",      -24.0f, 12.0f,   0.0f, unit ("dB", 1), 1.0f, false);

    return { params.begin(), params.end() };
}

//==============================================================================
bool WorshipVoiceFXProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return out == layouts.getMainInputChannelSet();
}

void WorshipVoiceFXProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    last = {};

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock,
                                  (juce::uint32) juce::jmax (1, getTotalNumOutputChannels()) };

    for (auto* g : { &inGain, &makeup, &outGain })
    {
        g->setRampDurationSeconds (0.05);
        g->prepare (spec);
    }

    hpf.prepare (spec);   eqLow.prepare (spec);  eqMid.prepare (spec);
    eqHigh.prepare (spec); revHP.prepare (spec); revLP.prepare (spec);
    *revHP.state = *Coeff::makeHighPass (sr, 220.0f, 0.707f);   // limpa graves da reverb

    comp.prepare (spec);
    comp.setAttack (15.0f);
    comp.setRelease (150.0f);

    chorus.prepare (spec);
    chorus.setRate (0.7f);
    chorus.setDepth (0.3f);
    chorus.setCentreDelay (8.0f);
    chorus.setFeedback (0.0f);

    reverb.prepare (spec);
    reverb.reset();

    preDelay.setMaximumDelayInSamples ((int) (0.25 * sr));
    preDelay.prepare (spec);
    echo.setMaximumDelayInSamples ((int) (2.0 * sr) + 8);
    echo.prepare (spec);

    mixer.prepare (spec);
    mixer.setMixingRule (juce::dsp::DryWetMixingRule::balanced);

    for (auto& f : deHP)
    {
        f.coefficients = Coeff::makeHighPass (sr, 6000.0f, 0.707f);
        f.reset();
    }
    deEnv = 0.0f;
    deAtk = std::exp (-1.0f / (0.001f * (float) sr));
    deRel = std::exp (-1.0f / (0.060f * (float) sr));
    dlyLP[0] = dlyLP[1] = 0.0f;

    deBand.setSize (2, samplesPerBlock);
    revBuf.setSize (2, samplesPerBlock);

    smDelay.reset (sr, 0.1);   smDelay.setCurrentAndTargetValue (0.35f * (float) sr);
    smDlyMix.reset (sr, 0.05); smDlyMix.setCurrentAndTargetValue (0.0f);
    smRevMix.reset (sr, 0.05); smRevMix.setCurrentAndTargetValue (0.0f);
}

void WorshipVoiceFXProcessor::updateParameters()
{
    inGain.setGainDecibels (p.inGain->load());
    makeup.setGainDecibels (p.compMakeup->load());
    outGain.setGainDecibels (p.outGain->load());
    mixer.setWetMixProportion (p.mix->load());

    const float lowCut = p.lowCut->load();
    if (changed (last.lowCut, lowCut))
        *hpf.state = *Coeff::makeHighPass (sr, lowCut, 0.707f);

    const float warm = p.warmth->load();
    if (changed (last.warm, warm))
        *eqLow.state = *Coeff::makeLowShelf (sr, 150.0f, 0.7f, juce::Decibels::decibelsToGain (warm));

    const float pres = p.presence->load();
    if (changed (last.pres, pres))
        *eqMid.state = *Coeff::makePeakFilter (sr, 3500.0f, 0.8f, juce::Decibels::decibelsToGain (pres));

    const float air = p.air->load();
    if (changed (last.air, air))
        *eqHigh.state = *Coeff::makeHighShelf (sr, 10000.0f, 0.7f, juce::Decibels::decibelsToGain (air));

    const float revTone = p.revTone->load();
    if (changed (last.revTone, revTone))
        *revLP.state = *Coeff::makeLowPass (sr, revTone, 0.707f);

    comp.setThreshold (p.compThresh->load());
    comp.setRatio (p.compRatio->load());
    chorus.setMix (0.5f * p.chorus->load());

    juce::dsp::Reverb::Parameters rp;
    rp.roomSize = p.revSize->load();
    rp.damping = p.revDamp->load();
    rp.wetLevel = 1.0f;
    rp.dryLevel = 0.0f;
    rp.width = 1.0f;
    rp.freezeMode = 0.0f;
    reverb.setParameters (rp);
    preDelay.setDelay (p.revPre->load() * 0.001f * (float) sr);
    smRevMix.setTargetValue (p.revMix->load());

    // Delay (livre ou sincronizado ao BPM da DAW)
    double bpm = 120.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto b = pos->getBpm())
                bpm = *b;

    const int sync = (int) p.dlySync->load();
    float delayMs = p.dlyTime->load();
    if (sync > 0)
    {
        static const double mult[] = { 0.0, 1.0, 0.5, 0.75, 2.0 };
        delayMs = (float) (60000.0 / juce::jmax (20.0, bpm) * mult[juce::jlimit (0, 4, sync)]);
    }
    smDelay.setTargetValue (juce::jlimit (10.0f, 1900.0f, delayMs) * 0.001f * (float) sr);
    smDlyMix.setTargetValue (p.dlyMix->load());

    cur.deEss = p.deEss->load();
    cur.sat = p.sat->load();
    cur.dlyFb = p.dlyFb->load();
    cur.dlyTone = p.dlyTone->load();
}

//==============================================================================
void WorshipVoiceFXProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int n = buffer.getNumSamples();
    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, n);
    if (n == 0)
        return;

    updateParameters();

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> ctx (block);

    mixer.pushDrySamples (block);

    inGain.process (ctx);
    hpf.process (ctx);
    processDeEsser (buffer);
    comp.process (ctx);
    makeup.process (ctx);
    processSaturation (buffer);
    eqLow.process (ctx);
    eqMid.process (ctx);
    eqHigh.process (ctx);
    chorus.process (ctx);
    processDelay (buffer);
    processReverb (buffer);

    mixer.mixWetSamples (block);
    outGain.process (ctx);
}

// De-esser split-band: reduz só a faixa de sibilância (acima de ~6 kHz)
void WorshipVoiceFXProcessor::processDeEsser (juce::AudioBuffer<float>& buf)
{
    if (cur.deEss < 0.01f)
        return;

    const int n = buf.getNumSamples();
    const int nch = juce::jmin (2, buf.getNumChannels());
    deBand.setSize (nch, n, false, false, true);

    for (int ch = 0; ch < nch; ++ch)
    {
        deBand.copyFrom (ch, 0, buf, ch, 0, n);
        auto* b = deBand.getWritePointer (ch);
        for (int i = 0; i < n; ++i)
            b[i] = deHP[ch].processSample (b[i]);
    }

    const float thr = juce::Decibels::decibelsToGain (juce::jmap (cur.deEss, 0.0f, 1.0f, -20.0f, -45.0f));

    for (int i = 0; i < n; ++i)
    {
        float peak = 0.0f;
        for (int ch = 0; ch < nch; ++ch)
            peak = juce::jmax (peak, std::abs (deBand.getSample (ch, i)));

        deEnv = peak > deEnv ? deAtk * (deEnv - peak) + peak
                             : deRel * (deEnv - peak) + peak;

        const float g = deEnv > thr ? std::pow (thr / deEnv, 0.7f) : 1.0f;

        for (int ch = 0; ch < nch; ++ch)
            buf.getWritePointer (ch)[i] += deBand.getSample (ch, i) * (g - 1.0f);
    }
}

// Saturação suave (calor analógico)
void WorshipVoiceFXProcessor::processSaturation (juce::AudioBuffer<float>& buf)
{
    if (cur.sat < 0.01f)
        return;

    for (int ch = 0; ch < buf.getNumChannels(); ++ch)
    {
        auto* d = buf.getWritePointer (ch);
        for (int i = 0; i < buf.getNumSamples(); ++i)
        {
            const float x = d[i];
            d[i] = x + cur.sat * (0.5f * std::tanh (2.0f * x) - x);
        }
    }
}

// Delay estéreo com filtro passa-baixa no feedback
void WorshipVoiceFXProcessor::processDelay (juce::AudioBuffer<float>& buf)
{
    const int n = buf.getNumSamples();
    const int nch = juce::jmin (2, buf.getNumChannels());
    const float a = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * cur.dlyTone / (float) sr);

    for (int i = 0; i < n; ++i)
    {
        const float dt = smDelay.getNextValue();
        const float mix = smDlyMix.getNextValue();

        for (int ch = 0; ch < nch; ++ch)
        {
            auto* d = buf.getWritePointer (ch);
            const float in = d[i];
            const float echoed = echo.popSample (ch, dt, true);
            dlyLP[ch] += a * (echoed - dlyLP[ch]);
            echo.pushSample (ch, in + dlyLP[ch] * cur.dlyFb);
            d[i] = in + dlyLP[ch] * mix;
        }
    }
}

// Reverb em "send": pre-delay -> low cut -> reverb -> high cut
void WorshipVoiceFXProcessor::processReverb (juce::AudioBuffer<float>& buf)
{
    const int n = buf.getNumSamples();
    const int nch = buf.getNumChannels();
    revBuf.setSize (nch, n, false, false, true);

    for (int ch = 0; ch < nch; ++ch)
        revBuf.copyFrom (ch, 0, buf, ch, 0, n);

    juce::dsp::AudioBlock<float> rb (revBuf);
    juce::dsp::ProcessContextReplacing<float> rc (rb);
    preDelay.process (rc);
    revHP.process (rc);
    reverb.process (rc);
    revLP.process (rc);

    for (int i = 0; i < n; ++i)
    {
        const float mix = smRevMix.getNextValue();
        for (int ch = 0; ch < nch; ++ch)
            buf.getWritePointer (ch)[i] += revBuf.getSample (ch, i) * mix;
    }
}

//==============================================================================
int WorshipVoiceFXProcessor::getNumPrograms() { return (int) presets().size(); }

const juce::String WorshipVoiceFXProcessor::getProgramName (int index)
{
    if (juce::isPositiveAndBelow (index, getNumPrograms()))
        return juce::String::fromUTF8 (presets()[(size_t) index].name);
    return {};
}

void WorshipVoiceFXProcessor::setCurrentProgram (int index)
{
    currentProgram = juce::jlimit (0, getNumPrograms() - 1, index);

    for (auto* prm : getParameters())
        if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (prm))
            r->setValueNotifyingHost (r->getDefaultValue());

    for (auto& [id, value] : presets()[(size_t) currentProgram].values)
        if (auto* r = apvts.getParameter (id))
            r->setValueNotifyingHost (r->convertTo0to1 (value));
}

void WorshipVoiceFXProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void WorshipVoiceFXProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* WorshipVoiceFXProcessor::createEditor()
{
    return new WorshipVoiceFXEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new WorshipVoiceFXProcessor();
}
