#pragma once
#include "Engine.h"
#include <juce_audio_utils/juce_audio_utils.h>

namespace orbit {
// Fixed-size musical snapshots are safe to copy at an audio block boundary.
struct Snapshot {
    Settings settings;
    float master=.35f;
    std::array<float,5> effects{{0,.35f,0,.5f,0}};
};
inline juce::var object(){return new juce::DynamicObject();}
inline void put(juce::var& obj,const char* key,const juce::var& value){obj.getDynamicObject()->setProperty(key,value);}
inline juce::var toJson(const Snapshot& snapshot){
    using namespace juce;const auto& s=snapshot.settings;auto p=object(),c=object(),enabled=object();
    put(p,"format","luma-orbit");put(p,"version",2);
    Array<var> notes,locks,phrase,drums;
    for(int i=0;i<8;++i){notes.add(s.notes[size_t(i)]);locks.add(s.locks[size_t(i)]);}
    for(int i=0;i<s.patternLength;++i){auto e=object();const auto event=s.customPattern?s.pattern[size_t(i)]:Event{i%8,1,false};put(e,"index",event.index);put(e,"beats",event.beats);put(e,"rest",event.rest);phrase.add(e);}
    for(int i=0;i<16;++i){auto h=object();const auto hit=s.customDrums?s.percussion[size_t(i)]:std::array<bool,3>{{i==0||i==8,i==4||i==12,i%2==0}};put(h,"kick",hit[0]);put(h,"clap",hit[1]);put(h,"hat",hit[2]);put(h,"velocity",s.customDrums?double(s.percussionVelocity[size_t(i)]):1.0);drums.add(h);}
    put(p,"base",notes);put(p,"notes",notes);put(p,"locks",locks);put(p,"phrase",phrase);put(p,"drumPattern",drums);
    const char* sounds[]={"glass","rubber","mist","keys","bell"};const char* scales[]={"minor","major","pentatonic"};
    put(p,"sound",sounds[s.sound]);put(p,"scale",scales[s.scale]);put(p,"flow",s.flow);put(p,"drumsEnabled",s.drums);
    put(enabled,"kick",s.kick);put(enabled,"clap",s.clap);put(enabled,"hat",s.hat);put(p,"drumEnabled",enabled);
    auto percent=[&](const char* id,float value){put(c,id,std::round(double(value)*10000)/100);};
    put(c,"bpm",s.bpm);percent("amount",s.explore);percent("tone",s.tone);percent("bass",s.bass);percent("volume",snapshot.master);percent("density",s.density);percent("depth",s.depth);percent("drum-volume",s.drumVolume);percent("synth-volume",s.synthVolume);
    percent("swing",s.swing);percent("probability",s.probability);percent("energy",s.energy);percent("body",s.body);
    const char* fx[]={"delay-mix","delay-feedback","room-mix","room-size"};for(int i=0;i<4;++i)percent(fx[i],snapshot.effects[size_t(i)]);
    put(c,"delay-division",int(snapshot.effects[4]));put(c,"accent-length",s.accents);put(p,"controls",c);return p;
}
inline bool fromJson(const juce::var& p,Snapshot& result){
    using namespace juce;Snapshot next;auto& s=next.settings;s.volume=1;
    if(!p["format"].isString()||!p["version"].isInt())return false;
    const bool legacy=p["format"].toString()=="luma-orbit-web"&&int(p["version"])==1;
    if(!legacy&&(p["format"].toString()!="luma-orbit"||int(p["version"])!=2))return false;
    auto number=[](const var& v,double min,double max){return (v.isInt()||v.isInt64()||v.isDouble())&&std::isfinite(double(v))&&double(v)>=min&&double(v)<=max;};
    auto integer=[&](const var& v,int min,int max){return number(v,min,max)&&double(v)==std::floor(double(v));};
    for(const char* key:{"base","notes","locks"})if(!p[key].isArray()||p[key].size()!=8)return false;
    for(int i=0;i<8;++i){if(!integer(p["notes"][i],0,7)||!integer(p["base"][i],0,7)||!p["locks"][i].isBool())return false;s.notes[size_t(i)]=int(p["notes"][i]);s.locks[size_t(i)]=bool(p["locks"][i]);}
    StringArray sounds{"glass","rubber","mist","keys","bell"},scales{"minor","major","pentatonic"};s.sound=sounds.indexOf(p["sound"].toString());s.scale=scales.indexOf(p["scale"].toString());if(s.sound<0||s.scale<0)return false;
    if(!p["flow"].isBool()||!p["drumsEnabled"].isBool())return false;s.flow=bool(p["flow"]);s.drums=bool(p["drumsEnabled"]);
    const auto enabled=p["drumEnabled"];for(const char* k:{"kick","clap","hat"})if(!enabled[k].isBool())return false;s.kick=bool(enabled["kick"]);s.clap=bool(enabled["clap"]);s.hat=bool(enabled["hat"]);
    const auto c=p["controls"];bool valid=true;auto percent=[&](const char* id,float& dest,double max=100){if(!number(c[id],0,max)){valid=false;return;}dest=float(double(c[id])/100);};
    if(!number(c["bpm"],40,200))return false;s.bpm=double(c["bpm"]);
    percent("amount",s.explore);percent("tone",s.tone);percent("bass",s.bass);percent("volume",next.master);percent("density",s.density);percent("depth",s.depth);percent("drum-volume",s.drumVolume);percent("synth-volume",s.synthVolume);
    s.body=0;if(c.hasProperty("body"))percent("body",s.body);
    if(!legacy){percent("swing",s.swing,60);percent("probability",s.probability);percent("energy",s.energy);}
    const char* fx[]={"delay-mix","delay-feedback","room-mix","room-size"};for(int i=0;i<4;++i)percent(fx[i],next.effects[size_t(i)],i==1?75:100);
    if(!integer(c["delay-division"],0,2)||!integer(c["accent-length"],5,8))return false;next.effects[4]=float(c["delay-division"]);s.accents=int(c["accent-length"]);if(s.accents!=5&&s.accents!=7&&s.accents!=8)return false;
    const auto phrase=p["phrase"],drums=p["drumPattern"];if(!phrase.isArray()||phrase.size()<8||phrase.size()>16||!drums.isArray()||drums.size()!=16)return false;
    s.customPattern=s.customDrums=true;s.patternLength=phrase.size();for(int i=0;i<phrase.size();++i){const auto e=phrase[i];if(!integer(e["index"],0,7)||!integer(e["beats"],1,2)||!e["rest"].isBool())return false;s.pattern[size_t(i)]={int(e["index"]),int(e["beats"]),bool(e["rest"])};}
    for(int i=0;i<16;++i){const auto h=drums[i];for(const char* k:{"kick","clap","hat"})if(!h[k].isBool())return false;if(!number(h["velocity"],0,1))return false;s.percussion[size_t(i)]={{bool(h["kick"]),bool(h["clap"]),bool(h["hat"])}};s.percussionVelocity[size_t(i)]=float(h["velocity"]);}
    if(!valid)return false;result=next;return true;
}
inline bool writeMidi(const Snapshot& snapshot,const juce::File& file){
    using namespace juce;const auto& s=snapshot.settings;MidiFile midi;midi.setTicksPerQuarterNote(480);std::array<MidiMessageSequence,3> tracks;
    auto add=[&](int t,MidiMessage m,double time){m.setTimeStamp(time);tracks[size_t(t)].addEvent(m);};
    add(0,MidiMessage::tempoMetaEvent(int(60000000/s.bpm)),0);add(0,MidiMessage::timeSignatureMetaEvent(4,4),0);
    std::array<double,33> times{};int total=0;for(int i=0;i<s.patternLength;++i)total+=s.customPattern?s.pattern[size_t(i)].beats:1;
    for(int i=0;i<total;++i)times[size_t(i+1)]=times[size_t(i)]+std::round(240*(1+(i%2?-1:1)*s.swing));
    auto note=[&](int track,int channel,int pitch,int velocity,double start,double end){add(track,MidiMessage::noteOn(channel,pitch,uint8_t(velocity)),start);add(track,MidiMessage::noteOff(channel,pitch),end);};
    int pulse=0,accent=0;for(int i=0;i<s.patternLength;++i){const auto e=s.customPattern?s.pattern[size_t(i)]:Event{i%8,1,false};if(!e.rest){const auto pitch=orbit::midi(s.notes[size_t(e.index)],s.scale);const auto start=times[size_t(pulse)],end=std::max(start+1,times[size_t(pulse+e.beats)]-12);note(0,1,pitch-(s.sound==1?12:0),accent%s.accents%2==0?100:65,start,end);if(s.bass>0)note(1,2,pitch-24,85,start,end);}pulse+=e.beats;accent+=e.beats;}
    if(s.drums)for(int i=0;i<total;++i){const auto h=s.customDrums?s.percussion[size_t(i%16)]:std::array<bool,3>{{i%16==0||i%16==8,i%16==4||i%16==12,i%2==0}};const bool enabled[]={s.kick,s.clap,s.hat};const int pitches[]={36,39,42};for(int k=0;k<3;++k)if(h[size_t(k)]&&enabled[k])note(2,10,pitches[k],jmax(1,int(std::round((s.customDrums?s.percussionVelocity[size_t(i%16)]:1)*100))),times[size_t(i)],times[size_t(i)]+30);}
    for(int i=0;i<3;++i){add(i,MidiMessage::endOfTrack(),times[size_t(total)]);tracks[size_t(i)].updateMatchedPairs();midi.addTrack(tracks[size_t(i)]);}
    TemporaryFile temp(file);auto stream=temp.getFile().createOutputStream();if(!stream||!midi.writeTo(*stream))return false;stream.reset();return temp.overwriteTargetFileWithTemporary();
}
}
