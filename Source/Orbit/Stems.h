#pragma once
#include "Recorder.h"
namespace orbit {
class Stems {
public:
    std::atomic<bool> active{false},failed{false};
    bool start(const juce::File& base,double rate){
        stop();failed=false;const char* names[]={"synth","sub","drums"};
        // The chooser approves a base name, not overwriting three sibling files.
        for(const auto* name:names)if(base.getSiblingFile(base.getFileNameWithoutExtension()+"-"+name+".wav").existsAsFile()){failed=true;return false;}
        for(int i=0;i<3;++i)if(!recorders[size_t(i)].start(base.getSiblingFile(base.getFileNameWithoutExtension()+"-"+names[i]+".wav"),rate,2)){stop();failed=true;return false;}
        active=true;return true;
    }
    void stop(){active=false;for(auto& r:recorders)r.stop();}
    void push(std::array<juce::AudioBuffer<float>,3>& buffers,int size){
        if(!active.load())return;
        for(int i=0;i<3;++i){juce::AudioBuffer<float> view(buffers[size_t(i)].getArrayOfWritePointers(),2,size);recorders[size_t(i)].push(view);}
        for(auto& r:recorders)if(!r.active.load()){failed=failed.load()||r.failed.load();active=false;}
    }
    double seconds()const{return recorders[0].seconds();}
private:
    std::array<Recorder,3> recorders;
};
}
