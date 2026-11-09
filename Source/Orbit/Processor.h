#pragma once
#include "Engine.h"
#include "Effects.h"
#include "Recorder.h"
#include "Session.h"
#include "Stems.h"
#include <juce_audio_utils/juce_audio_utils.h>

class LumaOrbit final : public juce::AudioProcessor,private juce::Timer {
public:
    LumaOrbit();
    ~LumaOrbit()override{stopTimer();stems.stop();recorder.stop();}
    juce::AudioProcessorValueTreeState state;
    std::array<std::atomic<int>,8> liveNotes{};
    std::atomic<int> liveStep{-1},liveAccent{0},livePhrase{1};
    std::atomic<bool> livePlaying{false},hostAvailable{false};
    std::atomic<double> liveBpm{108};
    void capture();
    orbit::Stems stems;
    orbit::Snapshot currentSnapshot();
    void saveScene(int);
    void recallScene(int);
    void serviceScenes();
    bool hasScene(int);
    bool exportMidi(const juce::File& file){return orbit::writeMidi(currentSnapshot(),file);}
    std::atomic<int> queuedScene{-1},activeScene{-1};
    std::atomic<bool> fillRequested{false};
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
    void timerCallback()override{serviceScenes();}
    std::array<int,8> publishedBaseNotes=orbit::initialNotes;
    juce::SpinLock snapshotMutex;
    orbit::Snapshot patternState,published,queued,sceneOverride,applied,audioBase,activeAudio;
    std::array<orbit::Snapshot,4> scenes;
    std::array<bool,4> scenePresent{};
    bool commandPending=false,commandQuantized=false,waitingScene=false;
    int waitingIndex=-1;double sceneBeat=0;
    std::atomic<bool> sceneNeedsSync{false},liveSnapshotValid{false};
    std::array<juce::AudioBuffer<float>,3> stemBuffers;
    orbit::Snapshot readControls(orbit::Snapshot);
    void applyControls(const orbit::Snapshot&);
    void queueSnapshot(const orbit::Snapshot&,bool,int=-1);
    juce::var sessionJson();
    bool restoreJson(const juce::var&);
    orbit::Effects effects;
    juce::ValueTree initialState;
    double rate=44100;
    juce::SmoothedValue<float> outputGain;
    bool wasPlaying=false,wasHost=false;
    std::atomic<bool> resetRequested{false};
    std::array<std::atomic<float>*,8> noteParams{},lockParams{};
    enum Control {run,sync,flow,bpm,explore,tone,volume,density,depth,drumVolume,bass,scale,sound,accents,drums,kick,clap,hat,delayMix,delayFeedback,roomMix,roomSize,delayDivision,synthVolume,swing,probability,energy,body,count};
    std::array<std::atomic<float>*,count> controls{};
    float value(Control c) const {return controls[size_t(c)]->load(std::memory_order_relaxed);}
};
