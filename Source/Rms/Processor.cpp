#include "Processor.h"
using namespace juce;
namespace {
AudioProcessorValueTreeState::ParameterLayout rmsParameters() {
    AudioProcessorValueTreeState::ParameterLayout result;
    result.add(std::make_unique<AudioParameterChoice>(ParameterID{"window",1},"RMS window",StringArray{"100 ms","400 ms","1000 ms"},1));
    return result;
}
}
LumaRms::LumaRms() : AudioProcessor(BusesProperties().withInput("Input",AudioChannelSet::stereo(),true).withOutput("Output",AudioChannelSet::stereo(),true)),
    state(*this,nullptr,"LumaRms",rmsParameters()),window(state.getRawParameterValue("window")) {}
void LumaRms::clearReadings() {
    for(auto& value : power) value.store(0,std::memory_order_relaxed);
    for(auto& value : peakHold) value.store(0,std::memory_order_relaxed);
    fill.store(0,std::memory_order_relaxed);
}
void LumaRms::prepareToPlay(double rate,int) {
    engine.prepare(rate); clearReadings(); measuredChannels.store(getTotalNumInputChannels());
}
void LumaRms::releaseResources() {
    engine.reset();
    // Hosts call this at Stop. Keep the last measurement visible until the next playback.
}
bool LumaRms::isBusesLayoutSupported(const BusesLayout& layout) const {
    const auto input=layout.getMainInputChannelSet();
    return (input==AudioChannelSet::mono() || input==AudioChannelSet::stereo()) && input==layout.getMainOutputChannelSet();
}
void LumaRms::processBlock(AudioBuffer<float>& audio,MidiBuffer&) {
    static constexpr std::array<double,3> seconds{0.1,0.4,1.0};
    engine.setWindow(seconds[size_t(jlimit(0,2,roundToInt(window->load(std::memory_order_relaxed))))]);
    if(resetRequested.exchange(false,std::memory_order_relaxed)) {
        for(auto& held : peakHold) held.store(0,std::memory_order_relaxed);
    }
    const auto channels=audio.getNumChannels();
    const auto reading=engine.process(audio.getArrayOfReadPointers(),channels,audio.getNumSamples());
    for(size_t ch=0;ch<2;++ch) {
        power[ch].store(float(reading.meanSquare[ch]),std::memory_order_relaxed);
        const auto old=peakHold[ch].load(std::memory_order_relaxed);
        peakHold[ch].store(jmax(old,reading.peak[ch]),std::memory_order_relaxed);
    }
    fill.store(float(reading.fill),std::memory_order_relaxed); measuredChannels.store(channels,std::memory_order_relaxed);
    if(audio.getNumSamples()>0) processedBlocks.fetch_add(1,std::memory_order_relaxed);
    // Deliberately no write access to the audio buffer: bit-identical, zero-latency passthrough.
}
void LumaRms::getStateInformation(MemoryBlock& data) { auto xml=state.copyState().createXml(); copyXmlToBinary(*xml,data); }
void LumaRms::setStateInformation(const void* data,int size) {
    if(auto xml=getXmlFromBinary(data,size)) if(xml->hasTagName(state.state.getType())) state.replaceState(ValueTree::fromXml(*xml));
}
AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new LumaRms(); }
