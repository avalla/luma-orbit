#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "RmsEngine.h"

class LumaRms final : public juce::AudioProcessor {
public:
    LumaRms();
    juce::AudioProcessorValueTreeState state;
    // Audio publishes linear powers; conversion to dBFS happens on the UI thread.
    std::array<std::atomic<float>,2> power{};
    std::array<std::atomic<float>,2> peakHold{};
    std::atomic<float> fill{0};
    std::atomic<int> measuredChannels{2};
    std::atomic<unsigned> processedBlocks{0};
    std::atomic<bool> resetRequested{false};
    const juce::String getName() const override { return "Luma RMS"; }
    void prepareToPlay(double,int) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
    void processBlockBypassed(juce::AudioBuffer<float>& b,juce::MidiBuffer& m) override { processBlock(b,m); }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0; }
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
    luma::RmsEngine engine;
    std::atomic<float>* window;
    void clearReadings();
};
