#include "Plugin.h"
#include "Voice.h"
using namespace juce;

Luma::Luma() : AudioProcessor(BusesProperties().withOutput("Output",AudioChannelSet::stereo(),true)),
    state(*this,nullptr,"Luma",luma::parameters()),params(luma::pointers(state)),initialState(state.copyState()) {
    for(int i=0;i<8;++i) synth.addVoice(new luma::Voice(state));
    synth.addSound(new luma::Sound());
}
void Luma::prepareToPlay(double rate,int block) {
    synth.allNotesOff(0,false);
    synth.setCurrentPlaybackSampleRate(rate);
    for(int i=0;i<synth.getNumVoices();++i) static_cast<luma::Voice*>(synth.getVoice(i))->prepare(rate,block);
    outputGain.reset(rate,0.02);
    outputGain.setCurrentAndTargetValue(Decibels::decibelsToGain(params[luma::gain]->load()));
    for(size_t channel=0;channel<stereoChorus.size();++channel) {
        auto& effect=stereoChorus[channel];
        effect.setRate(channel==0?0.27f:0.33f);
        effect.setDepth(0.3f); effect.setCentreDelay(channel==0?7.0f:11.0f); effect.setFeedback(0);
        effect.setMix(params[luma::chorus]->load()*0.5f);
        effect.prepare({rate,static_cast<uint32>(effectBlockSize),1});
    }
}
void Luma::releaseResources() {
    synth.allNotesOff(0,false); keyboard.reset();
    for(auto& effect : stereoChorus) effect.reset();
}
bool Luma::isBusesLayoutSupported(const BusesLayout& l) const {
    return l.getMainInputChannelSet().isDisabled() && (l.getMainOutputChannelSet()==AudioChannelSet::stereo() || l.getMainOutputChannelSet()==AudioChannelSet::mono());
}
void Luma::processBlock(AudioBuffer<float>& b,MidiBuffer& midi) {
    ScopedNoDenormals guard; b.clear(); keyboard.processNextMidiBuffer(midi,0,b.getNumSamples(),true);
    synth.renderNextBlock(b,midi,0,b.getNumSamples());
    const auto mix=params[luma::chorus]->load(std::memory_order_relaxed)*0.5f;
    dsp::AudioBlock<float> audio(b);
    for(int channel=0;channel<b.getNumChannels();++channel) {
        auto& effect=stereoChorus[size_t(channel)]; effect.setMix(mix);
        // Hosts may deliver blocks larger than prepareToPlay's hint. No audio-thread resizing.
        for(int start=0;start<b.getNumSamples();start+=effectBlockSize) {
            auto chunk=audio.getSingleChannelBlock(size_t(channel)).getSubBlock(size_t(start),size_t(jmin(effectBlockSize,b.getNumSamples()-start)));
            dsp::ProcessContextReplacing<float> context(chunk); effect.process(context);
        }
    }
    outputGain.setTargetValue(Decibels::decibelsToGain(params[luma::gain]->load(std::memory_order_relaxed)));
    for(int i=0;i<b.getNumSamples();++i) {
        const auto gain=outputGain.getNextValue();
        for(int c=0;c<b.getNumChannels();++c) b.setSample(c,i,b.getSample(c,i)*gain);
    }
    midi.clear();
}
bool Luma::applyFactoryPreset(int index) {
    if(!isPositiveAndBelow(index,int(luma::presets.size()))) return false;
    const auto& preset=luma::presets[size_t(index)];
    auto set=[this](const char* id,float value) {
        auto* parameter=state.getParameter(id);
        parameter->beginChangeGesture(); parameter->setValueNotifyingHost(parameter->convertTo0to1(value)); parameter->endChangeGesture();
    };
    set("wave",float(preset.wave));
    for(size_t i=0;i<luma::specs.size();++i) set(luma::specs[i].id,preset.values[i]);
    return true;
}
int Luma::matchingPreset() const {
    for(size_t preset=0;preset<luma::presets.size();++preset) {
        const auto& candidate=luma::presets[preset];
        if(roundToInt(state.getRawParameterValue("wave")->load())!=candidate.wave) continue;
        bool matches=true;
        for(size_t i=0;i<params.size();++i)
            if(std::abs(params[i]->load()-candidate.values[i])>jmax(0.0001f,std::abs(candidate.values[i])*0.0001f)) { matches=false; break; }
        if(matches) return int(preset);
    }
    return -1;
}
void Luma::getStateInformation(MemoryBlock& data) {
    auto xml=state.copyState().createXml(); copyXmlToBinary(*xml,data);
}
void Luma::setStateInformation(const void* data,int size) {
    auto xml=getXmlFromBinary(data,size);
    if(xml==nullptr || !xml->hasTagName(state.state.getType())) return;
    const auto incoming=ValueTree::fromXml(*xml);
    auto merged=initialState.createCopy();
    // Missing parameters in v0.1 sessions receive their original neutral defaults.
    for(auto child : merged) {
        const auto id=child.getProperty("id").toString();
        const auto saved=incoming.getChildWithProperty("id",id);
        if(!saved.isValid() || !saved.hasProperty("value")) continue;
        const auto value=float(saved.getProperty("value"));
        if(!std::isfinite(value)) continue;
        auto* parameter=state.getParameter(id);
        child.setProperty("value",parameter->convertFrom0to1(jlimit(0.0f,1.0f,parameter->convertTo0to1(value))),nullptr);
    }
    state.replaceState(merged);
}
AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new Luma(); }
