#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace orbit {
constexpr double pi=3.14159265358979323846;
inline constexpr std::array<int,8> initialNotes{{0,4,2,6,4,7,2,3}};
struct Settings {
    std::array<int,8> notes=initialNotes;
    std::array<bool,8> locks{};
    double bpm=108;
    float explore=.35f,tone=.62f,volume=.35f,density=.45f,depth=.65f,drumVolume=.55f,bass=.3f;
    int scale=0,sound=0,accents=5;
    bool flow=true,drums=true,kick=true,clap=true,hat=true;
};
inline int midi(int degree,int scale) {
    constexpr int scales[3][8]={{57,59,60,62,64,65,67,69},{60,62,64,65,67,69,71,72},{57,60,62,64,67,69,72,74}};
    return scales[std::clamp(scale,0,2)][std::clamp(degree,0,7)];
}
struct Event { int index=0,beats=1; bool rest=false; };
struct ToneVoice {
    double phase=0,frequency=440,age=0,duration=.35,attack=.006,filtered=0;
    float level=0; int shape=0;
    static double blep(double t,double dt) {
        if(t<dt){t/=dt;return 2*t-t*t-1;}
        if(t>1-dt){t=(t-1)/dt;return t*t+2*t+1;}return 0;
    }
    float sample(double rate,float tone) {
        if(age>=duration||level<=0)return 0;
        const auto dt=std::min(.45,frequency/rate);
        double value=std::sin(2*pi*phase);
        if(shape==3)value=.75*value+.25*std::sin(4*pi*phase);
        if(shape==4)value=std::sin(2*pi*phase+1.5*std::exp(-age*6)*std::sin(4*pi*phase));
        if(shape==1){value=0;for(int h=1;h<=15&&h*dt<.48;h+=2)value+=(h%4==1?1:-1)*std::sin(2*pi*h*phase)/(h*h);value*=8/(pi*pi);}
        if(shape==2)value=2*phase-1-blep(phase,dt);
        phase+=dt;if(phase>=1)phase-=1;
        const double cutoff=std::min(rate*.4,300+13000*double(tone)*tone);
        filtered+=(1-std::exp(-2*pi*cutoff/rate))*(value-filtered);
        const double env=std::min(1.0,age/attack)*std::exp(-7*age/duration);
        age+=1/rate;return float(filtered*env*level);
    }
};
// Fixed storage; no allocation or UI/host calls in sample processing.
class Engine {
public:
    Settings settings;
    std::array<int,8> notes=initialNotes;
    std::array<Event,16> phrase{};
    int phraseLength=8,phraseNumber=1,currentStep=-1,currentAccent=0;
    uint64_t pulseCount=0;
    double beat=0;
    bool lastRest=false;
    void prepare(double sampleRate){rate=sampleRate;reset(0);}
    void configure(const Settings& value){
        for(size_t i=0;i<notes.size();++i)if(value.notes[i]!=settings.notes[i])notes[i]=value.notes[i];
        settings=value;
    }
    void reset(double position){
        notes=settings.notes;for(int i=0;i<16;++i)phrase[size_t(i)]={i%8,1,false};
        phraseLength=8;phraseNumber=1;eventIndex=remaining=accent=voiceIndex=0;currentStep=-1;
        pulseCount=0;beat=position;nextPulse=std::ceil(position*2-1e-8)*.5;
        drumStep=int(std::fmod(std::floor(position*2),16)+16)%16;
        rng=0x4c554d41u;noiseState=0x12345678u;silence();makeDrums();
    }
    void silence(){for(auto& v:voices)v.level=0;bassVoice.level=0;kickAge=clapAge=hatAge=10;kickPhase=lowNoise=0;}
    void evolve(){
        const auto amount=settings.explore;
        if(amount<=0||std::all_of(settings.locks.begin(),settings.locks.end(),[](bool x){return x;})){
            phraseLength=8;for(int i=0;i<8;++i)phrase[size_t(i)]={i,1,false};return;
        }
        phraseLength=8+4*int(random()*3);
        std::array<int,8> free{};int count=0;
        for(int i=0;i<8;++i)if(!settings.locks[size_t(i)])free[size_t(count++)]=i;
        const int edits=std::min(count,std::max(1,int(std::ceil(amount*3))));
        for(int j=0;j<edits;++j){
            const int pick=int(random()*count),i=free[size_t(pick)];free[size_t(pick)]=free[size_t(--count)];
            const int old=notes[size_t(i)],delta=(random()<.5?-1:1)*(1+int(random()*std::ceil(amount*3)));
            notes[size_t(i)]=std::clamp(old+delta,0,7);if(notes[size_t(i)]==old)notes[size_t(i)]=old==7?6:old+1;
        }
        for(int j=0;j<phraseLength;++j){
            const int i=j<8?j:int(random()*8);const bool movable=!settings.locks[size_t(i)];
            phrase[size_t(j)]={i,movable&&random()<amount*.35?2:1,movable&&j>0&&random()<amount*.28};
        }
    }
    float sample(){
        if(beat+1e-9>=nextPulse){pulse();nextPulse+=.5;}
        beat+=settings.bpm/(60*rate);
        double melody=0;for(auto& v:voices)melody+=v.sample(rate,settings.tone);
        const float sub=bassVoice.sample(rate,.05f)*settings.bass;
        const double noise=double(nextRandom(noiseState))/4294967296.0*2-1;
        lowNoise+=.22*(noise-lowNoise);
        double kick=0,clap=0,hat=0;
        if(kickAge<.6){
            const double duration=.18+settings.depth*.32;
            const double frequency=42+(1-settings.depth)*20+90*std::exp(-kickAge*60);
            kickPhase+=frequency/rate;if(kickPhase>=1)kickPhase-=1;
            kick=std::sin(2*pi*kickPhase)*std::min(1.0,kickAge/.003)*std::exp(-9*kickAge/duration)*.7;
            kickAge+=1/rate;
        }
        if(clapAge<.2){const double flutter=clapAge<.042?(.3+.7*std::pow(std::sin(pi*clapAge/.014),2)):1;clap=(noise-lowNoise)*std::exp(-clapAge*35)*flutter*.24;clapAge+=1/rate;}
        if(hatAge<.08){hat=(noise-lowNoise)*std::exp(-hatAge*100)*.1;hatAge+=1/rate;}
        const double percussion=settings.drums?settings.drumVolume*((settings.kick?kick:0)+(settings.clap?clap:0)+(settings.hat?hat:0)):0;
        // Bass ducks during the kick; fixed headroom and soft saturation bound output.
        const double duck=settings.drums&&settings.kick?1-.65*std::exp(-kickAge*20):1;
        smoothedVolume+=(settings.volume-smoothedVolume)*std::min(1.0,1/(rate*.02));
        return float(std::tanh((melody+sub*duck+percussion)*smoothedVolume));
    }
private:
    double rate=44100,nextPulse=0,kickAge=10,clapAge=10,hatAge=10,kickPhase=0,lowNoise=0,smoothedVolume=0;
    std::array<ToneVoice,8> voices{};ToneVoice bassVoice;
    std::array<std::array<bool,3>,16> drumPattern{};
    int eventIndex=0,remaining=0,accent=0,drumStep=0,voiceIndex=0;
    uint32_t rng=0x4c554d41u,noiseState=0x12345678u;
    static uint32_t nextRandom(uint32_t& value){value=value*1664525u+1013904223u;return value;}
    double random(){return double(nextRandom(rng))/4294967296.0;}
    void makeDrums(){for(int i=0;i<16;++i)drumPattern[size_t(i)]={{i==0||i==8||(i%2==0&&random()<settings.density*.25),i==4||i==12,i%2==0||random()<settings.density}};}
    void pulse(){
        ++pulseCount;
        if(drumStep==0&&settings.flow&&settings.explore>0)makeDrums();
        const auto hit=drumPattern[size_t(drumStep)];drumStep=(drumStep+1)%16;
        if(settings.drums){if(hit[0]&&settings.kick){kickAge=0;kickPhase=0;}if(hit[1]&&settings.clap)clapAge=0;if(hit[2]&&settings.hat)hatAge=0;}
        if(remaining>0){--remaining;return;}
        if(eventIndex>=phraseLength){eventIndex=0;if(settings.flow){evolve();++phraseNumber;}}
        const auto event=phrase[size_t(eventIndex++)];currentStep=event.index;currentAccent=accent%settings.accents;lastRest=event.rest;
        accent+=event.beats;remaining=event.beats-1;
        if(event.rest)return;
        const int note=midi(notes[size_t(event.index)],settings.scale);
        auto& v=voices[size_t(voiceIndex++%8)];v={};v.shape=settings.sound;
        v.frequency=440*std::pow(2.0,(note-(settings.sound==1?12:0)-69)/12.0);
        v.duration=settings.sound==2?.85:settings.sound==1?.3:settings.sound==4?1.2:.5;v.attack=settings.sound==2?.08:.006;v.level=currentAccent%2==0?.3f:.17f;
        bassVoice={};bassVoice.frequency=440*std::pow(2.0,(note-24-69)/12.0);bassVoice.duration=.65;bassVoice.attack=.008;bassVoice.level=.5f;
    }
};
} // namespace orbit
