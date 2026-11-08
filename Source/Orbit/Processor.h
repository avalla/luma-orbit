#pragma once
#include "Engine.h"
#include "Effects.h"
#include "Recorder.h"
#include <juce_audio_utils/juce_audio_utils.h>

class LumaOrbit final : public juce::AudioProcessor {
public:
    LumaOrbit();
    juce::AudioProcessorValueTreeState state;
    std::array<std::atomic<int>,8> liveNotes{};
    std::atomic<int> liveStep{-1},liveAccent{0},livePhrase{1};
    std::atomic<bool> livePlaying{false},hostAvailable{false};
    std::atomic<double> liveBpm{108};
    void capture();
    orbit::Recorder recorder;
    bool savePreset(const juce::File&);
    bool loadPreset(const juce::File&);
    const juce::String getName() const override {return "Luma Orbit";}
    void prepareToPlay(double,int) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    bool acceptsMidi() const override {return false;}
    bool producesMidi() const override {return false;}
    double getTailLengthSeconds() const override {return 0;}
    bool hasEditor() const override {return true;}
    juce::AudioProcessorEditor* createEditor() override;
    int getNumPrograms() override {return 1;}
    int getCurrentProgram() override {return 0;}
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override {return "Orbit";}
    void changeProgramName(int,const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*,int) override;
private:
    orbit::Engine engine;
    orbit::Effects effects;
    juce::ValueTree initialState;
    double rate=44100;
    juce::SmoothedValue<float> outputGain;
    bool wasPlaying=false,wasHost=false;
    std::atomic<bool> resetRequested{false};
    std::array<std::atomic<float>*,8> noteParams{},lockParams{};
    enum Control {run,sync,flow,bpm,explore,tone,volume,density,depth,drumVolume,bass,scale,sound,accents,drums,kick,clap,hat,delayMix,delayFeedback,roomMix,roomSize,delayDivision,synthVolume,count};
    std::array<std::atomic<float>*,count> controls{};
    float value(Control c) const {return controls[size_t(c)]->load(std::memory_order_relaxed);}
};
