#include "Processor.h"
using namespace juce;
namespace {
constexpr const char* ids[]={"run","sync","flow","bpm","explore","tone","volume","density","depth","drumVolume","bass","scale","sound","accents","drums","kick","clap","hat","delayMix","delayFeedback","roomMix","roomSize","delayDivision","synthVolume","swing","probability","energy"};
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
    const char* fxIds[]={"delayMix","delayFeedback","roomMix","roomSize"};
    const char* fxNames[]={"Delay Mix","Delay Feedback","Reverb Mix","Reverb Size"};const float fxDefaults[]={0,.35f,0,.5f};
    for(int i=0;i<4;++i)p.add(std::make_unique<AudioParameterFloat>(ParameterID{fxIds[i],1},fxNames[i],NormalisableRange<float>(0,i==1?.75f:1.0f,.01f),fxDefaults[i]));
    p.add(std::make_unique<AudioParameterChoice>(ParameterID{"delayDivision",1},"Delay Division",StringArray{"1/8","1/8 puntato","1/4"},0));
    p.add(std::make_unique<AudioParameterFloat>(ParameterID{"synthVolume",1},"Synth Volume",NormalisableRange<float>(0,1,.01f),1));
    p.add(std::make_unique<AudioParameterFloat>(ParameterID{"swing",1},"Swing",NormalisableRange<float>(0,.6f,.01f),0));
    p.add(std::make_unique<AudioParameterFloat>(ParameterID{"probability",1},"Probability",NormalisableRange<float>(0,1,.01f),1));
    p.add(std::make_unique<AudioParameterFloat>(ParameterID{"energy",1},"Energy",NormalisableRange<float>(0,1,.01f),.5f));
    for(int i=0;i<8;++i){
        p.add(std::make_unique<AudioParameterInt>(ParameterID{"note"+String(i),1},"Nota "+String(i+1),0,7,orbit::initialNotes[size_t(i)]));
        p.add(std::make_unique<AudioParameterBool>(ParameterID{"lock"+String(i),1},"Blocca "+String(i+1),false));
    }
    return p;
}
}
LumaOrbit::LumaOrbit():AudioProcessor(BusesProperties().withOutput("Output",AudioChannelSet::stereo(),true)),state(*this,nullptr,"LumaOrbit",layout()){
    for(size_t i=0;i<controls.size();++i)controls[i]=state.getRawParameterValue(ids[i]);
    initialState=state.copyState();startTimerHz(30);
    for(int i=0;i<8;++i){noteParams[size_t(i)]=state.getRawParameterValue("note"+String(i));lockParams[size_t(i)]=state.getRawParameterValue("lock"+String(i));liveNotes[size_t(i)].store(orbit::initialNotes[size_t(i)]);}
}
void LumaOrbit::prepareToPlay(double sampleRate,int){recorder.stop();stems.stop();for(auto& b:stemBuffers)b.setSize(2,512);rate=sampleRate;outputGain.reset(rate,.02);outputGain.setCurrentAndTargetValue(value(volume));engine.prepare(rate);effects.prepare(rate);wasPlaying=false;wasHost=false;}
void LumaOrbit::releaseResources(){recorder.stop();stems.stop();engine.silence();effects.reset();wasPlaying=false;livePlaying.store(false);liveStep.store(-1);}
bool LumaOrbit::isBusesLayoutSupported(const BusesLayout& l)const{return l.getMainInputChannelSet().isDisabled()&&(l.getMainOutputChannelSet()==AudioChannelSet::stereo()||l.getMainOutputChannelSet()==AudioChannelSet::mono());}
void LumaOrbit::processBlock(AudioBuffer<float>& buffer,MidiBuffer& midi){
    ScopedNoDenormals guard;buffer.clear();midi.clear();
    bool reset=false;
    {const SpinLock::ScopedTryLockType lock(snapshotMutex);if(lock.isLocked()){
        audioBase=patternState;reset=resetRequested.exchange(false);
        if(commandPending){sceneOverride=queued;waitingScene=true;waitingIndex=queuedScene.load();sceneBeat=commandQuantized&&wasPlaying?(std::floor(engine.beat/4)+1)*4:engine.beat;commandPending=false;}
    }}
    auto current=sceneNeedsSync.load()?activeAudio:readControls(audioBase);auto s=current.settings;
    bool playing=value(run)>.5f,usingHost=false,positionValid=false;double position=0;
    // Standalone wrappers may offer a dummy playhead; use the internal transport there.
    if(value(sync)>.5f&&wrapperType!=wrapperType_Standalone){
        if(auto* head=getPlayHead())if(auto info=head->getPosition()){
            usingHost=true;playing=info->getIsPlaying();
            if(auto tempo=info->getBpm())if(std::isfinite(*tempo)&&*tempo>0)s.bpm=jlimit(1.0,999.0,*tempo);
            if(auto ppq=info->getPpqPosition())if(std::isfinite(*ppq)){position=*ppq;positionValid=true;}
        }
    }
    engine.configure(s);if(fillRequested.exchange(false))engine.fillPending=true;
    if(playing&&(!wasPlaying||usingHost!=wasHost||reset||(positionValid&&std::abs(position-engine.beat)>std::max(.00001,2*s.bpm/(60*rate))))){engine.reset(positionValid?position:0);effects.reset();}
    if(reset){waitingScene=false;queuedScene=-1;}
    hostAvailable.store(usingHost);liveBpm.store(s.bpm);livePlaying.store(playing);
    auto processOutput=[&](int offset,int length){if(length<=0)return;AudioBuffer<float> view(buffer.getArrayOfWritePointers(),buffer.getNumChannels(),offset,length);effects.process(view,s.bpm,int(current.effects[4]),current.effects[0],current.effects[1],current.effects[2],current.effects[3]);outputGain.setTargetValue(current.master);for(int n=0;n<length;++n){const auto gain=outputGain.getNextValue();for(int c=0;c<view.getNumChannels();++c)view.setSample(c,n,view.getSample(c,n)*gain);}};
    if(playing){
        int stemUsed=0,segmentStart=0;
        for(int i=0;i<buffer.getNumSamples();++i){
            if(waitingScene&&engine.beat+1e-9>=sceneBeat){
                const SpinLock::ScopedTryLockType lock(snapshotMutex);if(lock.isLocked()){
                    processOutput(segmentStart,i-segmentStart);segmentStart=i;current=sceneOverride;s=current.settings;if(usingHost)s.bpm=liveBpm.load();engine.configure(s);engine.reset(engine.beat);waitingScene=false;
                    applied=current;activeAudio=current;patternState=current;sceneNeedsSync=true;activeScene=waitingIndex;queuedScene=-1;
                }
            }
            const auto sample=engine.sample();for(int c=0;c<buffer.getNumChannels();++c)buffer.setSample(c,i,sample);
            for(int t=0;t<3;++t)for(int c=0;c<2;++c)stemBuffers[size_t(t)].setSample(c,stemUsed,engine.stems[size_t(t)]);
            if(++stemUsed==512){stems.push(stemBuffers,stemUsed);stemUsed=0;}
        }
        if(stemUsed)stems.push(stemBuffers,stemUsed);
        processOutput(segmentStart,buffer.getNumSamples()-segmentStart);
    }else{waitingScene=false;queuedScene=-1;engine.silence();if(wasPlaying||reset)effects.reset();if(reset)engine.reset(0);}
    recorder.push(buffer);
    if(!playing&&stems.active.load())for(int offset=0;offset<buffer.getNumSamples();offset+=512){for(auto& b:stemBuffers)b.clear();stems.push(stemBuffers,std::min(512,buffer.getNumSamples()-offset));}
    current.settings=s;current.settings.notes=engine.notes;current.settings.customPattern=current.settings.customDrums=true;current.settings.pattern=engine.phrase;current.settings.patternLength=engine.phraseLength;current.settings.percussion=engine.drumsPattern();current.settings.percussionVelocity=engine.drumVelocities();
    {const SpinLock::ScopedTryLockType lock(snapshotMutex);if(lock.isLocked()){published=current;publishedBaseNotes=s.notes;if(playing)liveSnapshotValid=true;}}
    wasPlaying=playing;wasHost=usingHost;
    for(size_t i=0;i<8;++i)liveNotes[i].store(engine.notes[i],std::memory_order_relaxed);
    liveStep.store(playing?engine.currentStep:-1);liveAccent.store(engine.currentAccent);livePhrase.store(engine.phraseNumber);
}
void LumaOrbit::capture(){auto snapshot=currentSnapshot();snapshot.settings.flow=false;applyControls(snapshot);queueSnapshot(snapshot,false);}
void LumaOrbit::getStateInformation(MemoryBlock& data){auto tree=state.copyState();tree.setProperty("orbitSession",JSON::toString(sessionJson()),nullptr);auto xml=tree.createXml();copyXmlToBinary(*xml,data);}
void LumaOrbit::setStateInformation(const void* data,int size){
    auto xml=getXmlFromBinary(data,size);if(xml==nullptr||!xml->hasTagName("LumaOrbit"))return;
    auto incoming=ValueTree::fromXml(*xml),merged=initialState.createCopy();
    if(incoming.hasProperty("orbitSession")&&restoreJson(JSON::parse(incoming["orbitSession"].toString()))){auto saved=incoming.getChildWithProperty("id","sync");if(saved.isValid())state.getParameter("sync")->setValueNotifyingHost(float(saved["value"])>.5f?1:0);return;}
    for(auto child:merged){const auto id=child["id"].toString();auto saved=incoming.getChildWithProperty("id",id);if(!saved.isValid()||!saved.hasProperty("value"))continue;
        const float v=float(saved["value"]);if(!std::isfinite(v))continue;auto* p=state.getParameter(id);child.setProperty("value",p->convertFrom0to1(jlimit(0.0f,1.0f,p->convertTo0to1(v))),nullptr);}
    // Restoring a session must not unexpectedly start the standalone transport.
    merged.getChildWithProperty("id","run").setProperty("value",0,nullptr);state.replaceState(merged);{const SpinLock::ScopedLockType lock(snapshotMutex);patternState={};published={};scenePresent.fill(false);commandPending=false;}sceneNeedsSync=false;liveSnapshotValid=false;queuedScene=-1;activeScene=-1;resetRequested.store(true);
}
AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new LumaOrbit();}

bool LumaOrbit::savePreset(const File& file){return file.replaceWithText(JSON::toString(sessionJson()));}
bool LumaOrbit::loadPreset(const File& file){
    MemoryBlock data;if(file.getSize()>262144||!file.loadFileAsData(data))return false;
    if(restoreJson(JSON::parse(file.loadFileAsString())))return true;
    auto xml=getXmlFromBinary(data.getData(),int(data.getSize()));if(!xml||!xml->hasTagName("LumaOrbit"))return false;
    setStateInformation(data.getData(),int(data.getSize()));return true;
}
orbit::Snapshot LumaOrbit::readControls(orbit::Snapshot result){
    auto& s=result.settings;for(size_t i=0;i<8;++i){s.notes[i]=int(noteParams[i]->load());s.locks[i]=lockParams[i]->load()>.5f;}
    s.bpm=value(bpm);s.flow=value(flow)>.5f;s.explore=value(explore);s.tone=value(tone);s.volume=1;s.synthVolume=value(synthVolume);s.density=value(density);s.depth=value(depth);s.drumVolume=value(drumVolume);s.bass=value(bass);s.scale=int(value(scale));s.sound=int(value(sound));
    constexpr int accentsList[]={5,7,8};s.accents=accentsList[jlimit(0,2,int(value(accents)))];s.drums=value(drums)>.5f;s.kick=value(kick)>.5f;s.clap=value(clap)>.5f;s.hat=value(hat)>.5f;s.swing=value(swing);s.probability=value(probability);s.energy=value(energy);
    result.master=value(volume);result.effects={{value(delayMix),value(delayFeedback),value(roomMix),value(roomSize),value(delayDivision)}};return result;
}
orbit::Snapshot LumaOrbit::currentSnapshot(){
    orbit::Snapshot result;std::array<int,8> previous;
    {const SpinLock::ScopedLockType lock(snapshotMutex);result=liveSnapshotValid.load()?published:patternState;previous=publishedBaseNotes;}
    const auto notes=result.settings.notes;result=readControls(result);if(liveSnapshotValid.load())for(size_t i=0;i<8;++i)if(result.settings.notes[i]==previous[i])result.settings.notes[i]=notes[i];return result;
}
void LumaOrbit::applyControls(const orbit::Snapshot& snapshot){
    const auto& s=snapshot.settings;auto set=[this](const String& id,float v){auto* p=state.getParameter(id);p->setValueNotifyingHost(p->convertTo0to1(v));};
    set("bpm",float(s.bpm));set("flow",s.flow);set("explore",s.explore);set("tone",s.tone);set("volume",snapshot.master);set("density",s.density);set("depth",s.depth);set("drumVolume",s.drumVolume);set("bass",s.bass);set("scale",float(s.scale));set("sound",float(s.sound));set("accents",s.accents==5?0:s.accents==7?1:2);set("drums",s.drums);set("kick",s.kick);set("clap",s.clap);set("hat",s.hat);set("synthVolume",s.synthVolume);set("swing",s.swing);set("probability",s.probability);set("energy",s.energy);
    const char* fx[]={"delayMix","delayFeedback","roomMix","roomSize","delayDivision"};for(int i=0;i<5;++i)set(fx[i],snapshot.effects[size_t(i)]);
    for(int i=0;i<8;++i){set("note"+String(i),float(s.notes[size_t(i)]));set("lock"+String(i),s.locks[size_t(i)]);}
}
void LumaOrbit::queueSnapshot(const orbit::Snapshot& snapshot,bool quantized,int index){
    const SpinLock::ScopedLockType lock(snapshotMutex);queued=snapshot;queuedScene=index;commandQuantized=quantized;commandPending=true;
    if(!livePlaying.load()){patternState=snapshot;published=snapshot;commandPending=false;activeScene=index;queuedScene=-1;resetRequested=true;}
}
void LumaOrbit::saveScene(int index){if(index<0||index>3)return;const auto snapshot=currentSnapshot();const SpinLock::ScopedLockType lock(snapshotMutex);scenes[size_t(index)]=snapshot;scenePresent[size_t(index)]=true;}
bool LumaOrbit::hasScene(int index){const SpinLock::ScopedLockType lock(snapshotMutex);return index>=0&&index<4&&scenePresent[size_t(index)];}
void LumaOrbit::recallScene(int index){if(!hasScene(index))return;orbit::Snapshot snapshot;{const SpinLock::ScopedLockType lock(snapshotMutex);snapshot=scenes[size_t(index)];}if(!livePlaying.load())applyControls(snapshot);queueSnapshot(snapshot,true,index);}
void LumaOrbit::serviceScenes(){if(!sceneNeedsSync.load())return;orbit::Snapshot snapshot;{const SpinLock::ScopedLockType lock(snapshotMutex);snapshot=applied;}applyControls(snapshot);sceneNeedsSync=false;}
var LumaOrbit::sessionJson(){auto p=orbit::toJson(currentSnapshot());Array<var> slots;std::array<orbit::Snapshot,4> copies;std::array<bool,4> present;{const SpinLock::ScopedLockType lock(snapshotMutex);copies=scenes;present=scenePresent;}for(int i=0;i<4;++i)slots.add(present[size_t(i)]?orbit::toJson(copies[size_t(i)]):var());orbit::put(p,"scenes",slots);return p;}
bool LumaOrbit::restoreJson(const var& p){
    orbit::Snapshot snapshot;std::array<orbit::Snapshot,4> slots;std::array<bool,4> present{};if(!orbit::fromJson(p,snapshot))return false;
    if(p.hasProperty("scenes")){auto values=p["scenes"];if(!values.isArray()||values.size()!=4)return false;for(int i=0;i<4;++i)if(!values[i].isVoid()){if(values[i].hasProperty("scenes")||!orbit::fromJson(values[i],slots[size_t(i)]))return false;present[size_t(i)]=true;}}
    auto* runParam=state.getParameter("run");runParam->setValueNotifyingHost(0);applyControls(snapshot);
    {const SpinLock::ScopedLockType lock(snapshotMutex);patternState=published=snapshot;scenes=slots;scenePresent=present;commandPending=false;}
    sceneNeedsSync=false;liveSnapshotValid=false;queuedScene=-1;activeScene=-1;resetRequested=true;return true;
}
