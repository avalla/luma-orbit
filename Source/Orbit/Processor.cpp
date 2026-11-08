#include "Processor.h"
using namespace juce;
namespace {
constexpr const char* ids[]={"run","sync","flow","bpm","explore","tone","volume","density","depth","drumVolume","bass","scale","sound","accents","drums","kick","clap","hat"};
AudioProcessorValueTreeState::ParameterLayout layout(){
    AudioProcessorValueTreeState::ParameterLayout p;
    auto boolean=[&](const char* id,const char* name,bool initial){p.add(std::make_unique<AudioParameterBool>(ParameterID{id,1},name,initial));};
    boolean("run","Play",false);boolean("sync","DAW Sync",true);boolean("flow","Flow",true);
    p.add(std::make_unique<AudioParameterFloat>(ParameterID{"bpm",1},"Tempo",NormalisableRange<float>(40,200,1),108));
    const char* names[]={"Explore","Brightness","Output","Density","Depth","Drum Volume","Sub Bass"};
    const float defaults[]={.35f,.62f,.35f,.45f,.65f,.55f,.3f};
    for(int i=0;i<7;++i)p.add(std::make_unique<AudioParameterFloat>(ParameterID{ids[4+i],1},names[i],NormalisableRange<float>(0,1,.01f),defaults[i]));
    p.add(std::make_unique<AudioParameterChoice>(ParameterID{"scale",1},"Scale",StringArray{"La minore","Do maggiore","La pentatonica"},0));
    p.add(std::make_unique<AudioParameterChoice>(ParameterID{"sound",1},"Sound",StringArray{"Glass","Rubber","Mist","Soft Keys","Bell"},0));
    p.add(std::make_unique<AudioParameterChoice>(ParameterID{"accents",1},"Accents",StringArray{"5 accenti","7 accenti","8 accenti"},0));
    boolean("drums","Drums",true);boolean("kick","Kick",true);boolean("clap","Clap",true);boolean("hat","Hi-hat",true);
    for(int i=0;i<8;++i){
        p.add(std::make_unique<AudioParameterInt>(ParameterID{"note"+String(i),1},"Nota "+String(i+1),0,7,orbit::initialNotes[size_t(i)]));
        p.add(std::make_unique<AudioParameterBool>(ParameterID{"lock"+String(i),1},"Blocca "+String(i+1),false));
    }
    return p;
}
}
LumaOrbit::LumaOrbit():AudioProcessor(BusesProperties().withOutput("Output",AudioChannelSet::stereo(),true)),state(*this,nullptr,"LumaOrbit",layout()){
    for(size_t i=0;i<controls.size();++i)controls[i]=state.getRawParameterValue(ids[i]);
    for(int i=0;i<8;++i){noteParams[size_t(i)]=state.getRawParameterValue("note"+String(i));lockParams[size_t(i)]=state.getRawParameterValue("lock"+String(i));liveNotes[size_t(i)].store(orbit::initialNotes[size_t(i)]);}
}
void LumaOrbit::prepareToPlay(double sampleRate,int){rate=sampleRate;engine.prepare(rate);wasPlaying=false;wasHost=false;}
void LumaOrbit::releaseResources(){engine.silence();wasPlaying=false;livePlaying.store(false);liveStep.store(-1);}
bool LumaOrbit::isBusesLayoutSupported(const BusesLayout& l)const{return l.getMainInputChannelSet().isDisabled()&&(l.getMainOutputChannelSet()==AudioChannelSet::stereo()||l.getMainOutputChannelSet()==AudioChannelSet::mono());}
void LumaOrbit::processBlock(AudioBuffer<float>& buffer,MidiBuffer& midi){
    ScopedNoDenormals guard;buffer.clear();midi.clear();
    orbit::Settings s;
    for(size_t i=0;i<8;++i){s.notes[i]=int(noteParams[i]->load());s.locks[i]=lockParams[i]->load()>.5f;}
    s.bpm=value(bpm);s.flow=value(flow)>.5f;s.explore=value(explore);s.tone=value(tone);s.volume=value(volume);
    s.density=value(density);s.depth=value(depth);s.drumVolume=value(drumVolume);s.bass=value(bass);s.scale=int(value(scale));s.sound=int(value(sound));
    constexpr int accentValues[]={5,7,8};s.accents=accentValues[jlimit(0,2,int(value(accents)))];
    s.drums=value(drums)>.5f;s.kick=value(kick)>.5f;s.clap=value(clap)>.5f;s.hat=value(hat)>.5f;
    bool playing=value(run)>.5f,usingHost=false,positionValid=false;double position=0;
    // Standalone wrappers may offer a dummy playhead; use the internal transport there.
    if(value(sync)>.5f&&wrapperType!=wrapperType_Standalone){
        if(auto* head=getPlayHead())if(auto info=head->getPosition()){
            usingHost=true;playing=info->getIsPlaying();
            if(auto tempo=info->getBpm())if(std::isfinite(*tempo)&&*tempo>0)s.bpm=jlimit(1.0,999.0,*tempo);
            if(auto ppq=info->getPpqPosition())if(std::isfinite(*ppq)){position=*ppq;positionValid=true;}
        }
    }
    engine.configure(s);
    const bool reset=resetRequested.exchange(false);
    if(playing&&(!wasPlaying||usingHost!=wasHost||reset||(positionValid&&std::abs(position-engine.beat)>std::max(.00001,2*s.bpm/(60*rate)))))engine.reset(positionValid?position:0);
    hostAvailable.store(usingHost);liveBpm.store(s.bpm);livePlaying.store(playing);
    if(playing){
        for(int i=0;i<buffer.getNumSamples();++i){const auto sample=engine.sample();for(int c=0;c<buffer.getNumChannels();++c)buffer.setSample(c,i,sample);}
    }else{engine.silence();if(reset)engine.reset(0);}
    wasPlaying=playing;wasHost=usingHost;
    for(size_t i=0;i<8;++i)liveNotes[i].store(engine.notes[i],std::memory_order_relaxed);
    liveStep.store(playing?engine.currentStep:-1);liveAccent.store(engine.currentAccent);livePhrase.store(engine.phraseNumber);
}
void LumaOrbit::capture(){
    auto set=[this](const String& id,float v){auto* p=state.getParameter(id);p->beginChangeGesture();p->setValueNotifyingHost(p->convertTo0to1(v));p->endChangeGesture();};
    set("flow",0);
    for(int i=0;i<8;++i)set("note"+String(i),float(liveNotes[size_t(i)].load()));
}
void LumaOrbit::getStateInformation(MemoryBlock& data){auto xml=state.copyState().createXml();copyXmlToBinary(*xml,data);}
void LumaOrbit::setStateInformation(const void* data,int size){
    auto xml=getXmlFromBinary(data,size);if(xml==nullptr||!xml->hasTagName("LumaOrbit"))return;
    auto incoming=ValueTree::fromXml(*xml),merged=state.copyState();
    for(auto child:merged){const auto id=child["id"].toString();auto saved=incoming.getChildWithProperty("id",id);if(!saved.isValid()||!saved.hasProperty("value"))continue;
        const float v=float(saved["value"]);if(!std::isfinite(v))continue;auto* p=state.getParameter(id);child.setProperty("value",p->convertFrom0to1(jlimit(0.0f,1.0f,p->convertTo0to1(v))),nullptr);}
    // Restoring a session must not unexpectedly start the standalone transport.
    merged.getChildWithProperty("id","run").setProperty("value",0,nullptr);state.replaceState(merged);resetRequested.store(true);
}
AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new LumaOrbit();}
