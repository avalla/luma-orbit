#include "Processor.h"
#include "Localization.h"
using namespace juce;
namespace {
const Colour background(0xff1c1d19),surface(0xff272922),line(0xff4b4e42),ink(0xffeeeee3),muted(0xffb8bca9),lime(0xffc5e66c),peach(0xffefaf80);
String noteName(int note){return MidiMessage::getMidiNoteName(note,true,true,4);}
struct OrbitTheme final:LookAndFeel_V4 {
    Font getTextButtonFont(TextButton&,int)override{return Font(FontOptions(17));}
    Font getComboBoxFont(ComboBox&)override{return Font(FontOptions(17));}
    Label* createSliderTextBox(Slider& slider)override{auto* label=LookAndFeel_V4::createSliderTextBox(slider);label->setFont(FontOptions(16));return label;}
};
class OrbitEditor final:public AudioProcessorEditor,private Timer {
public:
    explicit OrbitEditor(LumaOrbit& p):AudioProcessorEditor(p),owner(p){
        theme.setColour(ResizableWindow::backgroundColourId,background);
        theme.setColour(Label::textColourId,ink);theme.setColour(ComboBox::backgroundColourId,surface);theme.setColour(ComboBox::textColourId,ink);
        theme.setColour(ComboBox::outlineColourId,line);theme.setColour(PopupMenu::backgroundColourId,surface);theme.setColour(PopupMenu::textColourId,ink);
        theme.setColour(Slider::thumbColourId,lime);theme.setColour(Slider::trackColourId,lime);theme.setColour(Slider::backgroundColourId,line);
        theme.setColour(Slider::textBoxTextColourId,ink);theme.setColour(Slider::textBoxOutlineColourId,line);
        theme.setColour(TextButton::buttonColourId,surface);theme.setColour(TextButton::buttonOnColourId,Colour(0xff53652c));theme.setColour(TextButton::textColourOffId,ink);theme.setColour(TextButton::textColourOnId,ink);
        setLookAndFeel(&theme);setSize(1040,900);
        const char* toggleIds[]={"run","sync","flow","drums","kick","clap","hat"};
        const char* toggleNames[]={"Ascolta","Sync DAW","Flusso","Ritmo","Cassa","Clap","Hi-hat"};
        for(size_t i=0;i<buttons.size();++i){auto& b=buttons[i];b.setButtonText(toggleNames[i]);b.setClickingTogglesState(true);addAndMakeVisible(b);buttonLinks[i]=std::make_unique<State::ButtonAttachment>(owner.state,toggleIds[i],b);}
        addAndMakeVisible(capture);capture.setButtonText("Cattura la frase");capture.onClick=[this]{owner.capture();};capture.setTooltip("Conserva note, pause, durate e batteria. La frase si salva nella sessione DAW.");
        scale.addItemList({"La minore","Do maggiore","La pentatonica"},1);sound.addItemList({"Glass","Rubber","Mist","Soft Keys","Bell"},1);accents.addItemList({"5 accenti","7 accenti","8 accenti"},1);
        for(auto* c:{&scale,&sound,&accents})addAndMakeVisible(c);
        scaleLink=std::make_unique<State::ComboBoxAttachment>(owner.state,"scale",scale);soundLink=std::make_unique<State::ComboBoxAttachment>(owner.state,"sound",sound);accentLink=std::make_unique<State::ComboBoxAttachment>(owner.state,"accents",accents);
        const char* sliderIds[]={"bpm","explore","tone","bass","volume","density","depth","drumVolume","synthVolume","body"};
        const char* names[]={"Tempo interno","Esplora","Luminosita","Sub intonato","Volume generale","Densita ritmo","Profondita cassa","Volume ritmo","Volume synth","Corpo"};
        for(size_t i=0;i<sliders.size();++i){auto& s=sliders[i];s.setSliderStyle(Slider::LinearHorizontal);s.setTextBoxStyle(Slider::TextBoxRight,false,78,30);s.setNumDecimalPlacesToDisplay(i==0?0:2);addAndMakeVisible(s);
            labels[i].setText(names[i],dontSendNotification);labels[i].setFont(FontOptions(17));addAndMakeVisible(labels[i]);sliderLinks[i]=std::make_unique<State::SliderAttachment>(owner.state,sliderIds[i],s);}
        for(int i=0;i<8;++i){auto index=size_t(i);for(int n=0;n<8;++n)notes[index].addItem(String(n+1),n+1);addAndMakeVisible(notes[index]);
            noteLinks[index]=std::make_unique<State::ComboBoxAttachment>(owner.state,"note"+String(i),notes[index]);
            locks[index].setButtonText("Fisso");locks[index].setClickingTogglesState(true);addAndMakeVisible(locks[index]);lockLinks[index]=std::make_unique<State::ButtonAttachment>(owner.state,"lock"+String(i),locks[index]);}
        addAndMakeVisible(effectPage);effectPage.setComponentID("effects-page");effectPage.setButtonText("Effetti e scene");effectPage.setClickingTogglesState(true);
        effectPage.onClick=[this]{showEffects=effectPage.getToggleState();updatePage();};
        division.addItemList({"Delay: 1/8","Delay: 1/8 puntato","Delay: 1/4"},1);addChildComponent(division);
        divisionLink=std::make_unique<State::ComboBoxAttachment>(owner.state,"delayDivision",division);
        const char* fxIds[]={"delayMix","delayFeedback","roomMix","roomSize","swing","probability","energy"};
        const char* fxNames[]={"Quantita delay","Ripetizioni","Riverbero","Dimensione stanza","Swing","Probabilita colpi","Energia (0.50 neutra)"};
        for(size_t i=0;i<fx.size();++i){fx[i].setSliderStyle(Slider::LinearHorizontal);fx[i].setTextBoxStyle(Slider::TextBoxRight,false,78,30);fx[i].setNumDecimalPlacesToDisplay(2);addChildComponent(fx[i]);fxLabels[i].setText(fxNames[i],dontSendNotification);fxLabels[i].setFont(FontOptions(17));addChildComponent(fxLabels[i]);fxLinks[i]=std::make_unique<State::SliderAttachment>(owner.state,fxIds[i],fx[i]);}
        for(size_t i=0;i<sceneButtons.size();++i){const char* sceneNames[]={"Intro","Groove","Apertura","Finale"};sceneButtons[i].setButtonText(sceneNames[i]);sceneSaves[i].setButtonText("Salva qui");addChildComponent(sceneButtons[i]);addChildComponent(sceneSaves[i]);sceneButtons[i].onClick=[this,i]{owner.recallScene(int(i));};sceneSaves[i].onClick=[this,i]{owner.saveScene(int(i));fileStatus="Scena salvata nel preset";};}
        for(auto* b:{&midiExport,&stemRecord,&fill})addChildComponent(*b);
        midiExport.setButtonText("Esporta MIDI");midiExport.onClick=[this]{chooseFile(3);};stemRecord.setButtonText("Registra 3 tracce");stemRecord.onClick=[this]{if(owner.stems.active.load()){owner.stems.stop();stemsRecording=false;fileStatus="Tre WAV salvati";}else chooseFile(4);};fill.setButtonText("Fill: una battuta");fill.onClick=[this]{owner.fillRequested=true;fileStatus="Fill alla prossima battuta";};
        for(auto* b:{&savePreset,&loadPreset,&record})addAndMakeVisible(*b);
        savePreset.setButtonText("Salva preset");loadPreset.setButtonText("Apri preset");record.setButtonText("Registra WAV");
        savePreset.onClick=[this]{chooseFile(0);};loadPreset.onClick=[this]{chooseFile(1);};
        record.onClick=[this]{if(owner.recorder.active.load()){owner.recorder.stop();recording=false;fileStatus="WAV salvato";}else chooseFile(2);};
        fileMessage.setFont(FontOptions(16));addAndMakeVisible(fileMessage);
        addAndMakeVisible(languageSelector);languageSelector.setComponentID("language");
        for(size_t i=0;i<std::size(orbit::localization::languageIds);++i)languageSelector.addItem(String::fromUTF8(orbit::localization::languageNames[i]),int(i+1));
        for(size_t i=0;i<std::size(orbit::localization::languageIds);++i)if(language==orbit::localization::languageIds[i])languageSelector.setSelectedId(int(i+1),dontSendNotification);
        rememberTranslations();refreshLanguage();
        languageSelector.onChange=[this]{language=orbit::localization::languageIds[size_t(languageSelector.getSelectedId()-1)];if(isShowing())orbit::localization::save(language);refreshLanguage();};
        resized();timerCallback();startTimerHz(30);
    }
    ~OrbitEditor() override{stopTimer();owner.recorder.stop();owner.stems.stop();setLookAndFeel(nullptr);}
    void resized()override{
        languageSelector.setBounds(458,22,164,44);
        buttons[0].setBounds(640,22,112,44);buttons[1].setBounds(764,22,120,44);buttons[2].setBounds(896,22,112,44);
        scale.setBounds(32,106,220,42);sound.setBounds(548,106,220,42);accents.setBounds(780,106,228,42);
        for(int i=0;i<10;++i){labels[size_t(i)].setBounds(548,170+i*46,200,24);sliders[size_t(i)].setBounds(738,166+i*46,270,36);}
        for(int i=3;i<7;++i)buttons[size_t(i)].setBounds(32+(i-3)*122,540,112,42);
        capture.setBounds(32,601,228,44);effectPage.setBounds(280,601,228,44);
        savePreset.setBounds(32,824,180,42);loadPreset.setBounds(224,824,180,42);record.setBounds(416,824,200,42);fileMessage.setBounds(636,822,372,48);
        division.setBounds(548,106,460,42);
        for(int i=0;i<7;++i){fxLabels[size_t(i)].setBounds(548,210+i*52,200,28);fx[size_t(i)].setBounds(738,206+i*52,270,36);}
        for(int i=0;i<4;++i){sceneButtons[size_t(i)].setBounds(32+i*122,360,112,44);sceneSaves[size_t(i)].setBounds(32+i*122,412,112,40);}
        midiExport.setBounds(32,470,228,44);stemRecord.setBounds(280,470,228,44);fill.setBounds(548,588,460,44);
        for(int i=0;i<8;++i){notes[size_t(i)].setBounds(32+i*122,694,112,38);locks[size_t(i)].setBounds(32+i*122,740,112,34);}
    }
    void paint(Graphics& g)override{
        g.fillAll(background);g.setColour(ink);g.setFont(FontOptions(34,Font::bold));g.drawText("luma / orbit",32,18,420,50,Justification::centredLeft);
        g.setColour(line);g.drawHorizontalLine(86,32,1008);g.drawHorizontalLine(660,32,1008);
        const float cx=274,cy=320,r=150;
        if(!showEffects){
        g.setColour(line);g.drawEllipse(cx-r,cy-r,2*r,2*r,1.5f);g.drawEllipse(cx-112,cy-112,224,224,1.5f);g.drawEllipse(cx-72,cy-72,144,144,1);
        const int selected=int(owner.state.getRawParameterValue("scale")->load());
        for(int i=0;i<8;++i){const float a=float(i*orbit::pi/4-orbit::pi/2),x=cx+r*std::cos(a),y=cy+r*std::sin(a);const bool current=owner.liveStep.load()==i;
            g.setColour(current?lime:surface);g.fillEllipse(x-27,y-27,54,54);g.setColour(locks[size_t(i)].getToggleState()?peach:lime);g.drawEllipse(x-27,y-27,54,54,2);
            g.setColour(current?background:ink);g.setFont(FontOptions(17));g.drawText(noteName(orbit::midi(owner.liveNotes[size_t(i)].load(),selected)),int(x-28),int(y-14),56,28,Justification::centred);
            g.setColour(current?lime:muted);g.drawText(String(i+1),32+i*122,668,112,22,Justification::centred);
        }
        const int accentCount=accents.getSelectedId()==2?7:accents.getSelectedId()==3?8:5;
        for(int i=0;i<accentCount;++i){const auto a=i*2*orbit::pi/accentCount-orbit::pi/2;const float size=owner.livePlaying.load()&&owner.liveAccent.load()==i?16.0f:10.0f;g.setColour(peach);g.fillEllipse(cx+112*float(std::cos(a))-size/2,cy+112*float(std::sin(a))-size/2,size,size);}
        g.setColour(muted);g.setFont(FontOptions(17));g.drawText(tr("FRASE"),int(cx-60),int(cy-38),120,25,Justification::centred);g.setColour(ink);g.setFont(FontOptions(32));g.drawText(String(owner.livePhrase.load()),int(cx-60),int(cy-9),120,43,Justification::centred);
        g.setFont(FontOptions(16));g.setColour(muted);g.drawText(tr("8 note. Blocca quelle da conservare."),32,501,480,30,Justification::centred);
        }
        if(showEffects){g.setColour(ink);g.setFont(FontOptions(26));g.drawText(tr("Le tue scene"),32,180,476,40,Justification::centredLeft);g.setFont(FontOptions(17));g.setColour(muted);g.drawFittedText(tr("Salva qui conserva suono, frase e ritmo.\nRichiama una scena alla prossima battuta.\nI tre WAV sono asciutti e allineati.\nMIDI: frase, sub e batteria senza probabilita."),32,232,476,112,Justification::topLeft,4);g.setColour(ink);g.setFont(FontOptions(20));g.drawText(tr("Spazio e movimento"),548,162,460,30,Justification::centredLeft);g.setColour(muted);g.setFont(FontOptions(16));g.drawText(tr("Energia modula timbro, variazioni e densita."),548,558,460,26,Justification::centredLeft);}
        if(showEffects)for(int i=0;i<8;++i)g.drawText(String(i+1),32+i*122,668,112,22,Justification::centred);
        const auto status=owner.hostAvailable.load()?String("DAW: ")+(owner.livePlaying.load()?tr("in riproduzione"):tr("in pausa")):String(tr("Clock interno: "))+(owner.livePlaying.load()?tr("in riproduzione"):tr("premi Ascolta"));
        g.drawText(status+"  |  "+String(owner.liveBpm.load(),1)+" BPM",32,786,976,24,Justification::centredLeft);
    }
private:
    String language=orbit::localization::preferred();ComboBox languageSelector;
    String tr(const String& key)const{return orbit::localization::translate(key,language);}
    static bool hasTranslation(const String& key){for(const auto& entry:orbit::localization::messages)if(key==String::fromUTF8(entry.source))return true;return false;}
    void rememberTranslations(){
        // Only top-level controls: slider text boxes and combo labels hold live values.
        for(auto* child:getChildren()){
            if(auto* b=dynamic_cast<TextButton*>(child)){if(hasTranslation(b->getButtonText()))b->getProperties().set("translation",b->getButtonText());}
            else if(auto* l=dynamic_cast<Label*>(child)){if(hasTranslation(l->getText()))l->getProperties().set("translation",l->getText());}
            else if(auto* c=dynamic_cast<ComboBox*>(child))for(int i=0;i<c->getNumItems();++i)if(hasTranslation(c->getItemText(i)))c->getProperties().set("translation"+String(c->getItemId(i)),c->getItemText(i));
        }
    }
    void refreshLanguage(){
        for(auto* child:getChildren()){
            const auto key=child->getProperties()["translation"].toString();
            if(auto* b=dynamic_cast<TextButton*>(child)){if(key.isNotEmpty())b->setButtonText(tr(key));}
            else if(auto* l=dynamic_cast<Label*>(child)){if(key.isNotEmpty())l->setText(tr(key),dontSendNotification);}
            else if(auto* c=dynamic_cast<ComboBox*>(child)){
                const int selected=c->getSelectedId();
                for(int i=0;i<c->getNumItems();++i){const auto itemKey=c->getProperties()["translation"+String(c->getItemId(i))].toString();if(itemKey.isNotEmpty())c->changeItemText(c->getItemId(i),tr(itemKey));}
                c->setSelectedId(selected,dontSendNotification);
            }
        }
        languageSelector.setTitle(tr("Lingua"));languageSelector.setTooltip(tr("Lingua"));
        capture.setTooltip(tr("Conserva note, pause, durate e batteria. La frase si salva nella sessione DAW."));
        sliders[9].setTooltip(tr("Raddoppio detunato e armoniche calde. A zero resta il suono originale."));
        updatePage();timerCallback();repaint();
    }
    using State=AudioProcessorValueTreeState;
    LumaOrbit& owner;OrbitTheme theme;TooltipWindow tips{this,700};
    std::array<TextButton,7> buttons;TextButton capture;
    std::array<Slider,10> sliders;std::array<Label,10> labels;std::array<ComboBox,8> notes;std::array<TextButton,8> locks;
    ComboBox scale,sound,accents,division;TextButton effectPage;bool showEffects=false;
    std::array<Slider,7> fx;std::array<Label,7> fxLabels;
    std::array<TextButton,4> sceneButtons,sceneSaves;TextButton midiExport,stemRecord,fill;bool stemsRecording=false;
    std::array<std::unique_ptr<State::SliderAttachment>,7> fxLinks;
    std::unique_ptr<State::ComboBoxAttachment> divisionLink;
    void updatePage(){
        for(auto& s:sliders)s.setVisible(!showEffects);for(auto& l:labels)l.setVisible(!showEffects);
        sound.setVisible(!showEffects);accents.setVisible(!showEffects);division.setVisible(showEffects);
        for(auto& s:fx)s.setVisible(showEffects);for(auto& l:fxLabels)l.setVisible(showEffects);
        for(auto& b:sceneButtons)b.setVisible(showEffects);for(auto& b:sceneSaves)b.setVisible(showEffects);for(auto* b:{&midiExport,&stemRecord,&fill})b->setVisible(showEffects);
        effectPage.setButtonText(tr(showEffects?"Torna al suono":"Effetti e scene"));repaint();
    }
    std::array<std::unique_ptr<State::ButtonAttachment>,7> buttonLinks;
    std::array<std::unique_ptr<State::SliderAttachment>,10> sliderLinks;
    std::array<std::unique_ptr<State::ComboBoxAttachment>,8> noteLinks;
    std::array<std::unique_ptr<State::ButtonAttachment>,8> lockLinks;
    std::unique_ptr<State::ComboBoxAttachment> scaleLink,soundLink,accentLink;
    TextButton savePreset,loadPreset,record;Label fileMessage;
    std::unique_ptr<FileChooser> chooser;String fileStatus="WAV: massimo 5 minuti";bool recording=false;
    void chooseFile(int action){
        if(chooser)return;
        const auto extension=action==1?"*.json;*.orbit":action==0?"*.json":action==3?"*.mid":"*.wav";
        chooser=std::make_unique<FileChooser>(tr(action==1?"Apri preset Orbit":"Salva Orbit"),File::getSpecialLocation(File::userDocumentsDirectory).getChildFile(action==0||action==1?"Orbit.json":action==3?"Orbit.mid":"Orbit.wav"),extension);
        const auto flags=FileBrowserComponent::canSelectFiles|(action==1?FileBrowserComponent::openMode:FileBrowserComponent::saveMode|FileBrowserComponent::warnAboutOverwriting);
        Component::SafePointer<OrbitEditor> safe(this);
        chooser->launchAsync(flags,[safe,action](const FileChooser& dialog){
            if(!safe)return;const auto file=dialog.getResult();
            if(file!=File()){
                bool ok=false;
                if(action==0)ok=safe->owner.savePreset(file.withFileExtension("json"));
                if(action==1)ok=safe->owner.loadPreset(file);
                if(action==2){ok=safe->owner.recorder.start(file.withFileExtension("wav"),safe->owner.getSampleRate(),safe->owner.getTotalNumOutputChannels());safe->recording=ok;}
                if(action==3)ok=safe->owner.exportMidi(file.withFileExtension("mid"));
                if(action==4){ok=safe->owner.stems.start(file.withFileExtension("wav"),safe->owner.getSampleRate());safe->stemsRecording=ok;}
                safe->fileStatus=ok?(action==0?"Preset salvato":action==1?"Preset caricato":action==3?"MIDI esportato":"Registrazione avviata"):action==4?"Scegli un nome nuovo per le tracce":"Operazione non riuscita";
            }
            safe->chooser.reset();
        });
    }
    int previousScale=-1;
    void timerCallback()override{
        owner.serviceScenes();for(int i=0;i<4;++i){sceneButtons[size_t(i)].setEnabled(owner.hasScene(i));sceneButtons[size_t(i)].setToggleState(owner.activeScene.load()==i,dontSendNotification);}
        if(stemsRecording&&!owner.stems.active.load()){owner.stems.stop();stemsRecording=false;fileStatus=owner.stems.failed.load()?"Errore: tracce incomplete":"Tre WAV salvati (5 minuti)";}
        stemRecord.setButtonText(tr(owner.stems.active.load()?"Ferma le 3 tracce":"Registra 3 tracce"));
        if(owner.queuedScene.load()>=0)fileStatus="Scena in attesa della battuta";
        const bool active=owner.recorder.active.load();
        if(recording&&!active){owner.recorder.stop();recording=false;fileStatus=owner.recorder.failed.load()?"Errore: WAV incompleto":"WAV salvato (5 minuti)";}
        record.setButtonText(tr(active?"Ferma e salva WAV":"Registra WAV"));
        fileMessage.setText(active?"REC  "+String(owner.recorder.seconds(),1)+" s / 300 s":tr(fileStatus),dontSendNotification);
        const bool host=owner.hostAvailable.load();buttons[0].setEnabled(!host);sliders[0].setEnabled(!host);
        const int selected=int(owner.state.getRawParameterValue("scale")->load());
        if(selected!=previousScale){previousScale=selected;for(auto& combo:notes){const int selectedId=combo.getSelectedId();for(int n=0;n<8;++n)combo.changeItemText(n+1,noteName(orbit::midi(n,selected)));combo.setSelectedId(selectedId,dontSendNotification);}}
        repaint();
    }
};
}
AudioProcessorEditor* LumaOrbit::createEditor(){return new OrbitEditor(*this);}
