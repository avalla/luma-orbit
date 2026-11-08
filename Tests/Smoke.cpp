#include "../Source/Plugin.h"
using namespace juce;
#include <iostream>
int main() {
    ScopedJuceInitialiser_GUI init;
    Luma p;
    {
        std::unique_ptr<AudioProcessorEditor> editor(p.createEditor());
        MidiKeyboardComponent* keyboard=nullptr;
        for(auto* child : editor->getChildren())
            if(auto* candidate=dynamic_cast<MidiKeyboardComponent*>(child)) keyboard=candidate;
        if(keyboard==nullptr) return 5;
        for(auto key : String("awsedftgyhujk"))
            if(!keyboard->keyPressed(KeyPress(key,ModifierKeys::noModifiers,0))) return 6;
        if(keyboard->keyPressed(KeyPress('z',ModifierKeys::noModifiers,0))) return 7;
    }
    for (double rate : {44100.0,48000.0,96000.0}) {
        p.prepareToPlay(rate,256);
        for(int wave=0;wave<3;++wave) {
            p.state.getParameter("wave")->setValueNotifyingHost(float(wave)/2);
            AudioBuffer<float> b(2,256); MidiBuffer midi;
            for(int n=48;n<56;++n) midi.addEvent(MidiMessage::noteOn(1,n,0.8f),0);
            double energy=0;
            for(int block=0;block<30;++block) {
                p.processBlock(b,midi);
                for(int i=0;i<256;++i) {
                    auto v=b.getSample(0,i);
                    if(!std::isfinite(v) || std::abs(v)>1) return 1;
                    energy+=v*v;
                }
            }
            if(energy<0.001) return 2;
            midi.addEvent(MidiMessage::allNotesOff(1),0);
            for(int block=0;block<int(rate*6/256);++block) p.processBlock(b,midi);
            if(b.getMagnitude(0,256)>0.00001f) return 3;
        }
        p.releaseResources();
    }
    p.state.getParameter("gain")->setValueNotifyingHost(0.25f);
    MemoryBlock saved; p.getStateInformation(saved);
    Luma restored; restored.setStateInformation(saved.getData(),int(saved.getSize()));
    if(std::abs(restored.state.getParameter("gain")->getValue()-0.25f)>0.001f) return 4;
    // Exercise every new sound and ensure chorus produces actual stereo differences.
    bool stereo=false;
    p.prepareToPlay(48000,128);
    for(size_t preset=0;preset<luma::presets.size();++preset) {
        if(!p.applyFactoryPreset(int(preset)) || p.matchingPreset()!=int(preset)) return 8;
        p.releaseResources(); p.prepareToPlay(48000,128);
        AudioBuffer<float> audio(2,1025); MidiBuffer midi;
        for(int note : {48,55,60,64}) midi.addEvent(MidiMessage::noteOn(1,note,0.8f),0);
        double energy=0;
        for(int block=0;block<35;++block) {
            p.processBlock(audio,midi);
            for(int i=0;i<audio.getNumSamples();++i) {
                const auto left=audio.getSample(0,i),right=audio.getSample(1,i);
                if(!std::isfinite(left) || !std::isfinite(right) || std::abs(left)>1 || std::abs(right)>1) return 9;
                energy+=left*left;
                if(preset==1 && std::abs(left-right)>0.00001f) stereo=true;
            }
        }
        if(energy<0.001) return 10;
        midi.addEvent(MidiMessage::allNotesOff(1),0);
        for(int block=0;block<300;++block) p.processBlock(audio,midi);
        if(audio.getMagnitude(0,audio.getNumSamples())>0.00001f) return 11;
    }
    if(!stereo || p.applyFactoryPreset(-1) || p.applyFactoryPreset(99)) return 12;
    // Simulate a v0.1 session: all new parameters are absent from its state.
    p.applyFactoryPreset(0);
    auto legacy=p.state.copyState();
    for(const auto* id : {"blend","detune","sub","toneAmount","toneDecay","chorus"}) {
        auto child=legacy.getChildWithProperty("id",id); legacy.removeChild(child,nullptr);
    }
    auto legacyXml=legacy.createXml(); MemoryBlock legacyData;
    AudioProcessor::copyXmlToBinary(*legacyXml,legacyData);
    restored.applyFactoryPreset(1);
    restored.setStateInformation(legacyData.getData(),int(legacyData.getSize()));
    if(restored.matchingPreset()!=0) return 13;
    restored.state.getParameter("cutoff")->setValueNotifyingHost(0.5f);
    if(restored.matchingPreset()!=-1) return 14;
    std::unique_ptr<AudioProcessorEditor> editor(restored.createEditor());
    auto image=editor->createComponentSnapshot(editor->getLocalBounds(),true,1.5f);
    auto output=File::getCurrentWorkingDirectory().getChildFile("Luma-Synth-preview.png").createOutputStream();
    if(output) { output->setPosition(0); output->truncate(); PNGImageFormat().writeImageToStream(image,*output); }
    std::cout << "PASS: waveforms, sample rates, polyphony, release, state restore, keyboard mapping, six presets, stereo chorus, oversized blocks, legacy sessions\n";
}
