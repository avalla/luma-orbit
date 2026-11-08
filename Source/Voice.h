#pragma once
#include "Parameters.h"
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>

namespace luma {
struct Sound final : juce::SynthesiserSound {
    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};
struct Voice final : juce::SynthesiserVoice {
    explicit Voice(State& s) : params(pointers(s)),wave(s.getRawParameterValue("wave")) {}
    ParameterPointers params;
    std::atomic<float>* wave;
    juce::ADSR ampEnv, filterEnv;
    juce::dsp::StateVariableTPTFilter<float> filter;
    juce::SmoothedValue<float> cutoffHz, oscBlend, subLevel, detuneRatio, filterAmount, resonanceQ;
    double phase=0, phase2=0, subPhase=0, step=0, base=440;
    float velocity=0;
    float value(Control id) const { return params[size_t(id)]->load(std::memory_order_relaxed); }
    void prepare(double rate,int block) {
        ampEnv.setSampleRate(rate); filterEnv.setSampleRate(rate);
        filter.prepare({rate,static_cast<juce::uint32>(block),1});
        filter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
        for(auto* s : {&cutoffHz,&oscBlend,&subLevel,&detuneRatio,&filterAmount,&resonanceQ}) s->reset(rate,0.02);
        setTargets(true);
    }
    void setTargets(bool immediate=false) {
        auto set=[immediate](auto& s,float target) {
            if(immediate) s.setCurrentAndTargetValue(target); else s.setTargetValue(target);
        };
        set(cutoffHz,juce::jmin(value(cutoff),float(getSampleRate()*0.45)));
        set(oscBlend,value(blend)); set(subLevel,value(sub));
        set(detuneRatio,std::pow(2.0f,value(detune)/1200.0f));
        set(filterAmount,value(toneAmount)); set(resonanceQ,0.5f+value(resonance)*2.5f);
        ampEnv.setParameters({value(attack),value(decay),value(sustain),value(release)});
        filterEnv.setParameters({0.005f,value(toneDecay),0.0f,0.05f});
    }
    bool canPlaySound(juce::SynthesiserSound* s) override { return dynamic_cast<Sound*>(s)!=nullptr; }
    void startNote(int note,float vel,juce::SynthesiserSound*,int wheel) override {
        velocity=vel; phase=phase2=subPhase=0;
        base=juce::MidiMessage::getMidiNoteInHertz(note); pitchWheelMoved(wheel);
        filter.reset(); ampEnv.reset(); filterEnv.reset(); setTargets(true);
        ampEnv.noteOn(); filterEnv.noteOn();
    }
    void stopNote(float,bool tail) override {
        if(tail) { ampEnv.noteOff(); filterEnv.noteOff(); }
        else { ampEnv.reset(); filterEnv.reset(); clearCurrentNote(); }
    }
    void pitchWheelMoved(int wheel) override {
        step=juce::jmin(0.45,base*std::pow(2.0,(wheel-8192)/8192.0*2/12)/getSampleRate());
    }
    void controllerMoved(int,int) override {}
    static double blep(double t,double dt) {
        if(t<dt) { t/=dt; return t+t-t*t-1; }
        if(t>1-dt) { t=(t-1)/dt; return t*t+t+t+1; } return 0;
    }
    static double oscillator(int shape,double t,double dt) {
        if(shape==2) return 2*t-1-blep(t,dt);
        if(shape==0) return std::sin(juce::MathConstants<double>::twoPi*t);
        double sample=0;
        // Eight odd harmonics at most, discarding harmonics above Nyquist.
        for(int h=1;h<=15 && h*dt<0.48;h+=2)
            sample+=((h%4==1)?1:-1)*std::sin(juce::MathConstants<double>::twoPi*h*t)/(h*h);
        return sample*8/(juce::MathConstants<double>::pi*juce::MathConstants<double>::pi);
    }
    static void advance(double& t,double dt) { t+=dt; if(t>=1) t-=1; }
    void renderNextBlock(juce::AudioBuffer<float>& b,int start,int count) override {
        setTargets(); const auto shape=juce::roundToInt(wave->load(std::memory_order_relaxed));
        while(--count>=0) {
            const auto dt2=juce::jmin(0.45,step*detuneRatio.getNextValue());
            const auto mix=oscBlend.getNextValue(); const auto low=subLevel.getNextValue();
            const auto first=oscillator(shape,phase,step);
            const auto second=mix>0 ? oscillator(shape,phase2,dt2) : 0.0;
            const auto subSample=low>0 ? std::sin(juce::MathConstants<double>::twoPi*subPhase) : 0.0;
            const auto sample=((1-mix)*first+mix*second+low*subSample)/(1+low);
            advance(phase,step); advance(phase2,dt2); advance(subPhase,step*0.5);
            const auto amount=filterAmount.getNextValue()*filterEnv.getNextSample();
            filter.setCutoffFrequency(juce::jlimit(20.0f,float(getSampleRate()*0.45),cutoffHz.getNextValue()*std::exp2(amount)));
            filter.setResonance(resonanceQ.getNextValue());
            const auto out=filter.processSample(0,float(sample))*ampEnv.getNextSample()*velocity*0.12f;
            for(int ch=0;ch<b.getNumChannels();++ch) b.addSample(ch,start,out);
            ++start; if(!ampEnv.isActive()) { clearCurrentNote(); break; }
        }
    }
};
} // namespace luma
