#include "../Source/Orbit/Processor.h"
#include "../Source/Orbit/Localization.h"
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
        for(const auto* language:orbit::localization::languageIds){
            check(orbit::localization::resolve(String(language)+"-XX")==language,"Locale resolution failed");
            check(orbit::localization::translate("Corpo",language).isNotEmpty(),"Missing Body translation");
        }
        check(orbit::localization::resolve("ja-JP")=="en","Unsupported locale fallback failed");
        // Body must add lower harmonic energy, not merely amplify the original sine.
        auto bodySpectrum=[](float body){orbit::ToneVoice voice;voice.frequency=440;voice.level=.3f;voice.duration=1;double sine=0,cosine=0,power=0;
            for(int i=0;i<24000;++i){const auto x=voice.sample(48000,.7f,body);check(std::isfinite(x)&&std::abs(x)<=.3f,"Body exceeded voice headroom");sine+=x*std::sin(2*orbit::pi*220*i/48000);cosine+=x*std::cos(2*orbit::pi*220*i/48000);power+=x*x;}return std::array<double,2>{{sine*sine+cosine*cosine,power}};};
        const auto dryBody=bodySpectrum(0),fullBody=bodySpectrum(1);check(fullBody[0]>dryBody[0]*10,"Body did not add lower harmonics");check(fullBody[1]<dryBody[1]*1.2,"Body is only a level boost");
        // A quarter-second echo at 120 BPM must land on sample 12000 at 48 kHz.
        orbit::Effects fx;fx.prepare(48000);AudioBuffer<float> impulse(2,24001);impulse.clear();impulse.setSample(0,0,.5f);
        fx.process(impulse,120,0,1,0,0,.5f);
        check(impulse.getSample(0,12000)>.3f,"Delay not synchronized to eighth note");
        fx.reset();AudioBuffer<float> silence(2,24001);silence.clear();fx.process(silence,120,0,1,.75f,1,1);check(silence.getMagnitude(0,24001)==0,"Effects reset left a tail");
        for(int channels:{1,2}){
            orbit::Effects reverb;reverb.prepare(48000);AudioBuffer<float> room(channels,24000);room.clear();for(int c=0;c<channels;++c)room.setSample(c,0,.5f);
            reverb.process(room,120,0,0,0,1,.8f);check(room.getMagnitude(3000,21000)>.00001f,"Missing reverb tail");
            for(int c=0;c<channels;++c)for(int i=0;i<room.getNumSamples();++i)check(std::isfinite(room.getSample(c,i))&&std::abs(room.getSample(c,i))<=1,"Invalid effect output");
        }
        orbit::Effects bypass;bypass.prepare(48000);AudioBuffer<float> dry(1,32);for(int i=0;i<32;++i)dry.setSample(0,i,float(i)/64);bypass.process(dry,120,0,0,.35f,0,.5f);for(int i=0;i<32;++i)check(dry.getSample(0,i)==float(i)/64,"Default effects altered dry signal");
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
        for(float swing:{0.0f,.4f,.6f}){
            orbit::Engine swung;orbit::Settings groove;groove.bpm=120;groove.swing=swing;groove.flow=false;swung.configure(groove);swung.prepare(48000);
            std::vector<int> onsets;uint64_t previous=0;
            for(int i=0;i<96001;++i){swung.sample();if(swung.pulseCount!=previous){onsets.push_back(i);previous=swung.pulseCount;}}
            check(onsets.size()==9&&onsets[8]==96000,"Swing changed bar length");check(std::abs(onsets[1]-int(std::round(12000*(1+swing))))<=1,"Swing offbeat misplaced");
        }
        orbit::Engine probabilityEngine;orbit::Settings probabilitySettings;probabilitySettings.probability=0;probabilityEngine.configure(probabilitySettings);probabilityEngine.prepare(48000);
        for(int i=0;i<24000;++i)check(probabilityEngine.sample()==0,"Zero probability produced audio");
        probabilitySettings.locks.fill(true);probabilitySettings.drums=false;probabilityEngine.configure(probabilitySettings);probabilityEngine.reset(0);double lockedEnergy=0;for(int i=0;i<1000;++i)lockedEnergy+=std::abs(probabilityEngine.sample());check(lockedEnergy>0,"Probability silenced locked notes");
        orbit::Engine fillEngine;orbit::Settings fillSettings;fillSettings.bpm=120;fillSettings.flow=false;fillSettings.synthVolume=fillSettings.bass=0;fillSettings.customDrums=true;fillEngine.configure(fillSettings);fillEngine.prepare(48000);fillEngine.fillPending=true;double fillEnergy=0;
        for(int i=0;i<192000;++i){const auto sample=fillEngine.sample();if(i<96000)fillEnergy+=std::abs(sample);if(i>125000)check(sample==0,"Fill lasted beyond one bar and decay");}check(fillEnergy>0,"Fill produced no drum hits");check(fillEngine.drumsPattern()==fillSettings.percussion,"Fill rewrote saved drum pattern");
        const auto sharedFile=File(__FILE__).getParentDirectory().getChildFile("fixtures/orbit-shared.json");
        LumaOrbit shared;check(shared.loadPreset(sharedFile),"Shared web preset rejected");auto captured=shared.currentSnapshot();
        check(captured.settings.patternLength==12&&captured.settings.pattern[1].rest&&captured.settings.pattern[0].beats==2,"Shared phrase lost rhythm");
        check(captured.settings.body==0,"Legacy JSON enabled Body");
        auto bodyJson=orbit::toJson(captured);bodyJson["controls"].getDynamicObject()->setProperty("body",70);orbit::Snapshot bodySnapshot;check(orbit::fromJson(bodyJson,bodySnapshot)&&std::abs(bodySnapshot.settings.body-.7f)<.001f,"Body preset roundtrip failed");
        auto invalid=orbit::toJson(captured);invalid["controls"].getDynamicObject()->setProperty("probability",101);orbit::Snapshot untouched;check(!orbit::fromJson(invalid,untouched),"Invalid probability accepted");
        auto legacyWeb=orbit::toJson(captured);legacyWeb.getDynamicObject()->setProperty("format","luma-orbit-web");legacyWeb.getDynamicObject()->setProperty("version",1);for(const char* id:{"swing","probability","energy"})legacyWeb["controls"].getDynamicObject()->removeProperty(id);check(orbit::fromJson(legacyWeb,untouched)&&untouched.settings.swing==0&&untouched.settings.probability==1&&untouched.settings.energy==.5f,"Legacy web migration failed");
        shared.prepareToPlay(48000,256);set(shared,"sync",0);set(shared,"run",1);AudioBuffer<float> sharedAudio(2,256);MidiBuffer sharedMidi;shared.processBlock(sharedAudio,sharedMidi);shared.capture();shared.processBlock(sharedAudio,sharedMidi);shared.serviceScenes();
        set(shared,"run",0);shared.processBlock(sharedAudio,sharedMidi);set(shared,"run",1);shared.processBlock(sharedAudio,sharedMidi);
        check(shared.currentSnapshot().settings.patternLength==12&&shared.currentSnapshot().settings.pattern[1].rest,"Capture lost phrase after restart");
        MemoryBlock capturedState;shared.getStateInformation(capturedState);LumaOrbit capturedRestored;capturedRestored.setStateInformation(capturedState.getData(),int(capturedState.getSize()));check(capturedRestored.currentSnapshot().settings.patternLength==12,"DAW restore lost phrase");
        set(shared,"run",0);shared.processBlock(sharedAudio,sharedMidi);set(shared,"tone",.2f);shared.saveScene(0);set(shared,"tone",.8f);shared.saveScene(1);shared.recallScene(0);check(std::abs(shared.state.getRawParameterValue("tone")->load()-.2f)<.001f,"Stopped scene recall failed");
        set(shared,"run",1);shared.processBlock(sharedAudio,sharedMidi);shared.recallScene(1);for(int i=0;i<373;++i)shared.processBlock(sharedAudio,sharedMidi);check(shared.queuedScene.load()==1,"Scene switched before bar");
        for(int i=0;i<3;++i)shared.processBlock(sharedAudio,sharedMidi);check(shared.activeScene.load()==1&&shared.queuedScene.load()==-1,"Scene missed bar boundary");shared.serviceScenes();check(std::abs(shared.state.getRawParameterValue("tone")->load()-.8f)<.001f,"Scene controls did not synchronize");
        const auto exported=File::getCurrentWorkingDirectory().getChildFile("orbit-native-preset.json");check(shared.savePreset(exported),"Shared native preset save failed");LumaOrbit bank;check(bank.loadPreset(exported)&&bank.hasScene(0)&&bank.hasScene(1),"Scene bank roundtrip failed");
        const auto midiFile=File::getSpecialLocation(File::tempDirectory).getNonexistentChildFile("orbit-score",".mid");check(shared.exportMidi(midiFile),"MIDI export failed");FileInputStream midiStream(midiFile);MidiFile score;check(score.readFrom(midiStream)&&score.getNumTracks()==3&&score.getTimeFormat()==480,"Invalid MIDI file");for(int t=0;t<3;++t)check(score.getTrack(t)->getNumEvents()>1,"Empty MIDI track");midiFile.deleteFile();
        const auto stemsBase=File::getSpecialLocation(File::tempDirectory).getNonexistentChildFile("orbit-stems-test",".wav");check(shared.stems.start(stemsBase,48000),"Stems start failed");for(int i=0;i<10;++i)shared.processBlock(sharedAudio,sharedMidi);shared.stems.stop();check(!shared.stems.failed.load(),"Stem recorder overflow");
        AudioFormatManager stemFormats;stemFormats.registerBasicFormats();for(const char* name:{"synth","sub","drums"}){auto file=stemsBase.getSiblingFile(stemsBase.getFileNameWithoutExtension()+"-"+name+".wav");std::unique_ptr<AudioFormatReader> reader(stemFormats.createReaderFor(file));check(reader&&reader->lengthInSamples==2560&&reader->numChannels==2,"Stem alignment failed");reader.reset();file.deleteFile();}
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
        set(restored,"delayMix",.7f);set(restored,"roomMix",.4f);set(restored,"delayDivision",2);MemoryBlock fxState;restored.getStateInformation(fxState);
        LumaOrbit fxRestored;fxRestored.setStateInformation(fxState.getData(),int(fxState.getSize()));check(std::abs(fxRestored.state.getRawParameterValue("delayMix")->load()-.7f)<.001f&&fxRestored.state.getRawParameterValue("delayDivision")->load()==2,"Effects state lost");
        auto legacy=restored.state.copyState();for(const char* id:{"delayMix","delayFeedback","roomMix","roomSize","delayDivision"})legacy.removeChild(legacy.getChildWithProperty("id",id),nullptr);
        auto oldXml=legacy.createXml();MemoryBlock oldData;AudioProcessor::copyXmlToBinary(*oldXml,oldData);fxRestored.setStateInformation(oldData.getData(),int(oldData.getSize()));check(fxRestored.state.getRawParameterValue("delayMix")->load()==0&&fxRestored.state.getRawParameterValue("roomMix")->load()==0,"Legacy session enabled effects");
        p.capture();check(p.state.getRawParameterValue("flow")->load()==0,"Capture did not stop evolution");
        p.setPlayHead(nullptr);set(p,"sync",0);set(p,"run",1);p.processBlock(audio,midi);check(p.livePlaying.load(),"Internal clock failed");
        set(p,"delayMix",.6f);set(p,"roomMix",.5f);p.processBlock(audio,midi);
        set(p,"run",0);p.processBlock(audio,midi);check(audio.getMagnitude(0,512)==0,"Internal stop failed");
        // Presets preserve controls and never start playback.
        const auto presetFile=File::getSpecialLocation(File::tempDirectory).getNonexistentChildFile("orbit-test",".orbit");
        set(p,"synthVolume",.23f);check(p.savePreset(presetFile),"Preset save failed");
        LumaOrbit presetLoaded;check(presetLoaded.loadPreset(presetFile),"Preset load failed");
        check(std::abs(presetLoaded.state.getRawParameterValue("synthVolume")->load()-.23f)<.001f,"Mixer preset lost");
        presetFile.replaceWithText("invalid");check(!presetLoaded.loadPreset(presetFile),"Invalid preset accepted");presetFile.deleteFile();
        // Capture final output and verify the actual WAV payload, not only a file's existence.
        const auto wavFile=File::getSpecialLocation(File::tempDirectory).getNonexistentChildFile("orbit-test",".wav");
        check(p.recorder.start(wavFile,48000,2),"WAV start failed");set(p,"run",1);
        for(int block=0;block<8;++block)p.processBlock(audio,midi);
        p.recorder.stop();check(!p.recorder.failed.load(),"WAV FIFO overflow");
        AudioFormatManager formats;formats.registerBasicFormats();std::unique_ptr<AudioFormatReader> reader(formats.createReaderFor(wavFile));
        check(reader&&reader->lengthInSamples==4096&&reader->numChannels==2&&reader->sampleRate==48000,"WAV header or length incorrect");
        AudioBuffer<float> recorded(2,4096);reader->read(&recorded,0,4096,0,true,true);check(recorded.getMagnitude(0,4096)>0,"Recorded WAV silent");reader.reset();wavFile.deleteFile();
        set(p,"synthVolume",0);set(p,"bass",0);set(p,"drumVolume",0);set(p,"delayMix",0);set(p,"roomMix",0);
        for(int block=0;block<10;++block)p.processBlock(audio,midi);check(audio.getMagnitude(0,512)==0,"Mixer mute failed");
        set(p,"synthVolume",1);set(p,"roomMix",1);set(p,"volume",0);
        for(int block=0;block<10;++block)p.processBlock(audio,midi);check(audio.getMagnitude(0,512)==0,"Master did not mute effects");
        std::unique_ptr<AudioProcessorEditor> editor(p.createEditor());
        const auto parametersBeforeLanguage=p.state.copyState().toXmlString();
        for(auto* child:editor->getChildren())if(child->getComponentID()=="language")if(auto* selector=dynamic_cast<ComboBox*>(child)){
            for(int i=0;i<selector->getNumItems();++i){selector->setSelectedId(i+1,dontSendNotification);selector->onChange();check(p.state.copyState().toXmlString()==parametersBeforeLanguage,"Language changed audio parameters");
                auto languageImage=editor->createComponentSnapshot(editor->getLocalBounds());FileOutputStream languageStream(File::getCurrentWorkingDirectory().getChildFile("Luma-Orbit-"+String(orbit::localization::languageIds[size_t(i)])+".png"));languageStream.setPosition(0);languageStream.truncate();PNGImageFormat().writeImageToStream(languageImage,languageStream);
            }
        }
        for(auto* child:editor->getChildren())if(auto* combo=dynamic_cast<ComboBox*>(child))check(combo->getSelectedId()>0&&combo->getText().isNotEmpty(),"Editor selection lost while naming notes");
        auto snapshot=editor->createComponentSnapshot(editor->getLocalBounds());
        File output=File::getCurrentWorkingDirectory().getChildFile("Luma-Orbit-preview.png");FileOutputStream stream(output);stream.setPosition(0);stream.truncate();PNGImageFormat().writeImageToStream(snapshot,stream);
        for(auto* child:editor->getChildren())if(auto* button=dynamic_cast<TextButton*>(child))if(button->getComponentID()=="effects-page"){button->setToggleState(true,dontSendNotification);button->onClick();}
        auto performanceImage=editor->createComponentSnapshot(editor->getLocalBounds());FileOutputStream performanceStream(File::getCurrentWorkingDirectory().getChildFile("Luma-Orbit-scenes-preview.png"));performanceStream.setPosition(0);performanceStream.truncate();PNGImageFormat().writeImageToStream(performanceImage,performanceStream);
        std::cout<<"PASS: Orbit 5 sounds/3 rates, sample clock, locks, block invariance, DAW transport/seek/tempo, state, capture, stop and UI snapshot\n";
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
    return 0;
}
