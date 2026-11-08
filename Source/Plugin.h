#pragma once
#include "Parameters.h"
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>

class Luma final : public juce::AudioProcessor {
public:
    Luma();
    luma::State state;
    juce::Synthesiser synth;
    juce::MidiKeyboardState keyboard;
    bool applyFactoryPreset(int index);
    int matchingPreset() const;
    const juce::String getName() const override { return "Luma"; }
    void prepareToPlay(double,int) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 5.2; }
    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor* createEditor() override;
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int,const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*,int) override;
private:
    luma::ParameterPointers params;
    juce::ValueTree initialState;
    juce::SmoothedValue<float> outputGain;
    std::array<juce::dsp::Chorus<float>,2> stereoChorus;
    int effectBlockSize=512;
};
