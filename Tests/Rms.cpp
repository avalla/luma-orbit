#include "../Source/Rms/Processor.h"
#include <cstring>
#include <iostream>
#include <stdexcept>
using namespace juce;
static void check(bool condition,const char* message) { if(!condition) throw std::runtime_error(message); }
static bool near(double a,double b,double tolerance=1e-6) { return std::abs(a-b)<tolerance; }
int main() {
    ScopedJuceInitialiser_GUI gui;
    try {
        luma::RmsEngine engine; engine.prepare(1000); engine.setWindow(0.4);
        std::array<float,400> ones,zeros{}; ones.fill(1);
        const float* data[]={ones.data(),zeros.data()};
        auto reading=engine.process(data,2,200);
        check(near(reading.meanSquare[0],1),"Warmup must use captured samples");
        data[0]=zeros.data(); reading=engine.process(data,2,200);
        check(near(reading.meanSquare[0],0.5),"400 ms window must contain 200 loud + 200 silent samples");
        reading=engine.process(data,2,200);
        check(near(reading.meanSquare[0],0),"Old samples must leave the rolling window");
        engine.setWindow(0.1); data[0]=ones.data(); reading=engine.process(data,2,100);
        check(near(reading.meanSquare[0],1) && near(reading.fill,1),"Window change must reset measurement cleanly");

        engine.prepare(48000);
        std::vector<float> sine(19200);
        for(size_t i=0;i<sine.size();++i) sine[i]=float(std::sin(2*MathConstants<double>::pi*1000*double(i)/48000));
        data[0]=sine.data(); reading=engine.process(data,1,int(sine.size()));
        check(near(10*std::log10(reading.meanSquare[0]),-3.0102999566),"Full scale sine must read -3.0103 dBFS RMS");
        engine.reset();
        for(size_t offset=0;offset<sine.size();offset+=37) {
            data[0]=sine.data()+offset;
            reading=engine.process(data,1,int(std::min(size_t(37),sine.size()-offset)));
        }
        check(near(reading.meanSquare[0],0.5),"Measurement must be independent of block boundaries");

        LumaRms meter;
        for(double rate : {44100.0,48000.0,96000.0}) {
            for(int channels : {1,2}) {
                auto layout=meter.getBusesLayout(); const auto set=channels==1?AudioChannelSet::mono():AudioChannelSet::stereo();
                layout.inputBuses.set(0,set); layout.outputBuses.set(0,set);
                check(meter.setBusesLayout(layout),"Mono/stereo layout rejected");
                meter.prepareToPlay(rate,256);
                for(int frames : {0,1,17,256,4097}) {
                    AudioBuffer<float> audio(channels,frames);
                    for(int ch=0;ch<channels;++ch) for(int i=0;i<frames;++i) audio.setSample(ch,i,(ch==0?1.0f:-1.0f)*0.5f);
                    AudioBuffer<float> original; original.makeCopyOf(audio);
                    MidiBuffer midi; midi.addEvent(MidiMessage::noteOn(1,60,0.5f),0);
                    meter.processBlock(audio,midi);
                    for(int ch=0;ch<channels;++ch)
                        check(frames==0 || std::memcmp(audio.getReadPointer(ch),original.getReadPointer(ch),size_t(frames)*sizeof(float))==0,"Audio must pass through bit-identically");
                    check(midi.getNumEvents()==1,"Passthrough must not consume MIDI");
                }
                check(near(meter.power[0].load(),0.25),"Constant 0.5 must read -6.0206 dBFS RMS");
                check(near(meter.power[1].load(),channels==2?0.25:0),"Stereo anti-phase channels must not cancel in the RMS reading");
                check(near(meter.peakHold[0].load(),0.5),"Sample peak incorrect");
                check(meter.getLatencySamples()==0,"Meter must add no latency");
                meter.releaseResources();
                check(near(meter.power[0].load(),0.25),"Stopping playback must preserve the last RMS reading");
                check(near(meter.peakHold[0].load(),0.5),"Stopping playback must preserve peak hold");
                meter.prepareToPlay(rate,256);
                check(near(meter.power[0].load(),0) && near(meter.fill.load(),0),"A new playback must begin with a fresh window");
            }
        }
        meter.prepareToPlay(48000,256);
        AudioBuffer<float> quiet(2,256); quiet.clear(); MidiBuffer midi;
        for(int i=0;i<256;++i) quiet.setSample(0,i,0.8f);
        meter.processBlock(quiet,midi); quiet.applyGain(0.125f); meter.processBlock(quiet,midi);
        check(near(meter.peakHold[0].load(),0.8),"Peak hold must retain maximum");
        meter.resetRequested.store(true); meter.processBlockBypassed(quiet,midi);
        check(near(meter.peakHold[0].load(),0.1),"Peak reset must start a new measurement");
        meter.state.getParameter("window")->setValueNotifyingHost(1);
        MemoryBlock saved; meter.getStateInformation(saved);
        LumaRms restored; restored.setStateInformation(saved.getData(),int(saved.getSize()));
        check(near(restored.state.getRawParameterValue("window")->load(),2),"RMS window must survive session restore");

        meter.prepareToPlay(48000,256);
        AudioBuffer<float> reference(2,48000);
        for(int i=0;i<reference.getNumSamples();++i) { reference.setSample(0,i,0.5f); reference.setSample(1,i,0.25f); }
        meter.processBlock(reference,midi);
        std::unique_ptr<AudioProcessorEditor> editor(meter.createEditor());
        const auto image=editor->createComponentSnapshot(editor->getLocalBounds(),true,1.5f);
        auto output=File::getCurrentWorkingDirectory().getChildFile("Luma-RMS-preview.png").createOutputStream();
        if(output) { output->setPosition(0); output->truncate(); PNGImageFormat().writeImageToStream(image,*output); }
        std::cout<<"PASS: exact RMS, sine calibration, rolling window, block independence, mono/stereo, 3 sample rates, bit-identical audio, peak reset, bypass, state restore\n";
    } catch(const std::exception& error) { std::cerr<<error.what()<<'\n'; return 1; }
}
