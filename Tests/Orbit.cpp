#include "../Source/Orbit/Processor.h"
#include <iostream>
#include <stdexcept>
using namespace juce;
void check(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
void set(LumaOrbit& p,const char* id,float value){auto* param=p.state.getParameter(id);param->setValueNotifyingHost(param->convertTo0to1(value));}
struct Host final:AudioPlayHead {
    double bpm=120,ppq=0;bool playing=true;
    Optional<PositionInfo> getPosition()const override{PositionInfo p;p.setBpm(bpm);p.setPpqPosition(ppq);p.setIsPlaying(playing);return p;}
};
int main(){
    ScopedJuceInitialiser_GUI gui;
    try{
        for(double rate:{44100.0,48000.0,96000.0})for(int sound=0;sound<5;++sound){
            orbit::Engine e;orbit::Settings settings;settings.sound=sound;settings.bpm=120;settings.explore=1;settings.locks[0]=true;e.configure(settings);e.prepare(rate);
            double energy=0;
            for(int i=0;i<int(rate*5);++i){const float x=e.sample();check(std::isfinite(x)&&std::abs(x)<1,"Nonfinite or clipped DSP");energy+=x*x;}
            check(energy>1,"Silent sound");check(e.pulseCount==20,"Clock pulse drift");check(e.notes[0]==settings.notes[0],"Locked note changed");
        }
        orbit::Engine e;orbit::Settings settings;settings.flow=true;settings.explore=1;settings.locks.fill(true);e.configure(settings);e.prepare(48000);
        for(int n=0;n<100;++n)e.evolve();check(e.notes==settings.notes,"All locks not respected");
        settings.locks.fill(false);settings.explore=0;e.configure(settings);e.reset(0);e.evolve();check(e.notes==settings.notes&&e.phraseLength==8,"Zero exploration changed phrase");
        // The host clock, not block partitioning, must determine the rendered audio.
        auto render=[](int block){
            LumaOrbit p;Host host;p.setPlayHead(&host);p.prepareToPlay(48000,block);set(p,"explore",.8f);
            std::vector<float> out;out.reserve(96000);MidiBuffer midi;
            while(out.size()<96000){int n=jmin(block,96000-int(out.size()));host.ppq=double(out.size())*120/(60*48000);AudioBuffer<float> b(2,n);p.processBlock(b,midi);for(int i=0;i<n;++i)out.push_back(b.getSample(0,i));}
            p.setPlayHead(nullptr);return out;
        };
        auto a=render(17),b=render(1024);check(a==b,"Audio depends on host block size");
        LumaOrbit p;Host host;p.setPlayHead(&host);p.prepareToPlay(48000,512);AudioBuffer<float> audio(2,512);MidiBuffer midi;
        p.processBlock(audio,midi);check(p.livePlaying.load()&&audio.getMagnitude(0,512)>0,"Host did not start");
        host.playing=false;p.processBlock(audio,midi);check(audio.getMagnitude(0,512)==0&&!p.livePlaying.load(),"Host stop not silent");
        host.playing=true;host.ppq=8;host.bpm=90;p.processBlock(audio,midi);check(p.liveBpm.load()==90&&audio.getMagnitude(0,512)>0,"Host seek/tempo failed");
        set(p,"bass",.77f);set(p,"sound",4);set(p,"note3",7);set(p,"lock3",1);set(p,"run",1);
        MemoryBlock state;p.getStateInformation(state);LumaOrbit restored;restored.setStateInformation(state.getData(),int(state.getSize()));
        check(std::abs(restored.state.getRawParameterValue("bass")->load()-.77f)<.001f,"Sub state lost");
        check(restored.state.getRawParameterValue("sound")->load()==4&&restored.state.getRawParameterValue("note3")->load()==7&&restored.state.getRawParameterValue("lock3")->load()==1,"Sequence state lost");
        check(restored.state.getRawParameterValue("run")->load()==0,"Restore auto-started");
        p.capture();check(p.state.getRawParameterValue("flow")->load()==0,"Capture did not stop evolution");
        p.setPlayHead(nullptr);set(p,"sync",0);set(p,"run",1);p.processBlock(audio,midi);check(p.livePlaying.load(),"Internal clock failed");
        set(p,"run",0);p.processBlock(audio,midi);check(audio.getMagnitude(0,512)==0,"Internal stop failed");
        std::unique_ptr<AudioProcessorEditor> editor(p.createEditor());
        auto snapshot=editor->createComponentSnapshot(editor->getLocalBounds());
        File output=File::getCurrentWorkingDirectory().getChildFile("Luma-Orbit-preview.png");FileOutputStream stream(output);stream.setPosition(0);stream.truncate();PNGImageFormat().writeImageToStream(snapshot,stream);
        std::cout<<"PASS: Orbit 5 sounds/3 rates, sample clock, locks, block invariance, DAW transport/seek/tempo, state, capture, stop and UI snapshot\n";
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
    return 0;
}
