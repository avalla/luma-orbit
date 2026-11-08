#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <algorithm>
#include <cmath>
#include <vector>

namespace orbit {
class Effects {
public:
    void prepare(double sampleRate){
        rate=sampleRate;for(auto& channel:delay)channel.assign(size_t(std::ceil(rate*4))+2,0);
        tap.reset(rate,.05);wet.reset(rate,.02);feedback.reset(rate,.02);
        tap.setCurrentAndTargetValue(float(rate*.25));wet.setCurrentAndTargetValue(0);feedback.setCurrentAndTargetValue(.35f);
        juce::Reverb::Parameters p;p.dryLevel=.5f;p.wetLevel=0;room.setParameters(p);room.setSampleRate(rate);reset();
    }
    void reset(){for(auto& channel:delay)std::fill(channel.begin(),channel.end(),0);write=0;room.reset();enabled=false;}
    void process(juce::AudioBuffer<float>& buffer,double bpm,int division,float delayMix,float repeats,float roomMix,float size){
        if(delayMix<=0&&roomMix<=0){if(enabled)reset();return;}
        enabled=true;
        const double beats[]={.5,.75,1};
        tap.setTargetValue(float(std::clamp(60/std::max(1.0,bpm)*beats[std::clamp(division,0,2)]*rate,1.0,double(delay[0].size()-2))));
        wet.setTargetValue(delayMix);feedback.setTargetValue(std::clamp(repeats,0.0f,.75f));
        const auto capacity=int(delay[0].size());const int channels=buffer.getNumChannels();
        for(int i=0;i<buffer.getNumSamples();++i){
            const float distance=tap.getNextValue();float read=float(write)-distance;if(read<0)read+=float(capacity);if(read>=float(capacity))read=0;
            const int first=int(read),second=(first+1)%capacity;const float fraction=read-float(first);
            const float left=delay[0][size_t(first)]*(1-fraction)+delay[0][size_t(second)]*fraction;
            const float right=delay[1][size_t(first)]*(1-fraction)+delay[1][size_t(second)]*fraction;
            const float mix=wet.getNextValue(),repeat=feedback.getNextValue();
            const float inL=buffer.getSample(0,i),inR=channels>1?buffer.getSample(1,i):inL;
            delay[0][size_t(write)]=inL+right*repeat;delay[1][size_t(write)]=inR+left*repeat;
            buffer.setSample(0,i,inL+left*mix*.7f);if(channels>1)buffer.setSample(1,i,inR+right*mix*.7f);
            write=(write+1)%capacity;
        }
        juce::Reverb::Parameters p;p.roomSize=.15f+size*.75f;p.damping=.55f;p.wetLevel=roomMix*.25f;p.dryLevel=.5f;p.width=1;p.freezeMode=0;
        room.setParameters(p);
        if(channels>1)room.processStereo(buffer.getWritePointer(0),buffer.getWritePointer(1),buffer.getNumSamples());
        else room.processMono(buffer.getWritePointer(0),buffer.getNumSamples());
        for(int c=0;c<channels;++c)for(int i=0;i<buffer.getNumSamples();++i)buffer.setSample(c,i,std::clamp(buffer.getSample(c,i),-1.0f,1.0f));
    }
private:
    double rate=44100;int write=0;bool enabled=false;
    std::array<std::vector<float>,2> delay;
    juce::SmoothedValue<float> tap,wet,feedback;
    juce::Reverb room;
};
}
