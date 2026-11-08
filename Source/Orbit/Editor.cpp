#include "Processor.h"
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
        addAndMakeVisible(capture);capture.setButtonText("Cattura le note");capture.onClick=[this]{owner.capture();};capture.setTooltip("Conserva le note correnti e disattiva Flusso. Le note si salvano nella sessione DAW.");
        scale.addItemList({"La minore","Do maggiore","La pentatonica"},1);sound.addItemList({"Glass","Rubber","Mist","Soft Keys","Bell"},1);accents.addItemList({"5 accenti","7 accenti","8 accenti"},1);
        for(auto* c:{&scale,&sound,&accents})addAndMakeVisible(c);
        scaleLink=std::make_unique<State::ComboBoxAttachment>(owner.state,"scale",scale);soundLink=std::make_unique<State::ComboBoxAttachment>(owner.state,"sound",sound);accentLink=std::make_unique<State::ComboBoxAttachment>(owner.state,"accents",accents);
        const char* sliderIds[]={"bpm","explore","tone","bass","volume","density","depth","drumVolume","synthVolume"};
        const char* names[]={"Tempo interno","Esplora","Luminosita","Sub intonato","Volume generale","Densita ritmo","Profondita cassa","Volume ritmo","Volume synth"};
        for(size_t i=0;i<sliders.size();++i){auto& s=sliders[i];s.setSliderStyle(Slider::LinearHorizontal);s.setTextBoxStyle(Slider::TextBoxRight,false,78,30);s.setNumDecimalPlacesToDisplay(i==0?0:2);addAndMakeVisible(s);
            labels[i].setText(names[i],dontSendNotification);labels[i].setFont(FontOptions(17));addAndMakeVisible(labels[i]);sliderLinks[i]=std::make_unique<State::SliderAttachment>(owner.state,sliderIds[i],s);}
        for(int i=0;i<8;++i){auto index=size_t(i);for(int n=0;n<8;++n)notes[index].addItem(String(n+1),n+1);addAndMakeVisible(notes[index]);
            noteLinks[index]=std::make_unique<State::ComboBoxAttachment>(owner.state,"note"+String(i),notes[index]);
            locks[index].setButtonText("Fisso");locks[index].setClickingTogglesState(true);addAndMakeVisible(locks[index]);lockLinks[index]=std::make_unique<State::ButtonAttachment>(owner.state,"lock"+String(i),locks[index]);}
        addAndMakeVisible(effectPage);effectPage.setButtonText("Effetti");effectPage.setClickingTogglesState(true);
        effectPage.onClick=[this]{showEffects=effectPage.getToggleState();updatePage();};
        division.addItemList({"Delay: 1/8","Delay: 1/8 puntato","Delay: 1/4"},1);addChildComponent(division);
        divisionLink=std::make_unique<State::ComboBoxAttachment>(owner.state,"delayDivision",division);
        const char* fxIds[]={"delayMix","delayFeedback","roomMix","roomSize"};
        const char* fxNames[]={"Quantita delay","Ripetizioni","Riverbero","Dimensione stanza"};
        for(size_t i=0;i<fx.size();++i){fx[i].setSliderStyle(Slider::LinearHorizontal);fx[i].setTextBoxStyle(Slider::TextBoxRight,false,78,30);fx[i].setNumDecimalPlacesToDisplay(2);addChildComponent(fx[i]);fxLabels[i].setText(fxNames[i],dontSendNotification);fxLabels[i].setFont(FontOptions(17));addChildComponent(fxLabels[i]);fxLinks[i]=std::make_unique<State::SliderAttachment>(owner.state,fxIds[i],fx[i]);}
        for(auto* b:{&savePreset,&loadPreset,&record})addAndMakeVisible(*b);
        savePreset.setButtonText("Salva preset");loadPreset.setButtonText("Apri preset");record.setButtonText("Registra WAV");
        savePreset.onClick=[this]{chooseFile(0);};loadPreset.onClick=[this]{chooseFile(1);};
        record.onClick=[this]{if(owner.recorder.active.load()){owner.recorder.stop();recording=false;fileStatus="WAV salvato";}else chooseFile(2);};
        fileMessage.setFont(FontOptions(16));addAndMakeVisible(fileMessage);
        resized();timerCallback();startTimerHz(30);
    }
    ~OrbitEditor() override{stopTimer();owner.recorder.stop();setLookAndFeel(nullptr);}
    void resized()override{
        buttons[0].setBounds(640,22,112,44);buttons[1].setBounds(764,22,120,44);buttons[2].setBounds(896,22,112,44);
        scale.setBounds(32,106,220,42);sound.setBounds(548,106,220,42);accents.setBounds(780,106,228,42);
        for(int i=0;i<9;++i){labels[size_t(i)].setBounds(548,170+i*52,200,24);sliders[size_t(i)].setBounds(738,166+i*52,270,36);}
        for(int i=3;i<7;++i)buttons[size_t(i)].setBounds(32+(i-3)*122,540,112,42);
        capture.setBounds(32,601,228,44);effectPage.setBounds(280,601,228,44);
        savePreset.setBounds(32,824,180,42);loadPreset.setBounds(224,824,180,42);record.setBounds(416,824,200,42);fileMessage.setBounds(636,822,372,48);
        division.setBounds(548,106,460,42);
        for(int i=0;i<4;++i){fxLabels[size_t(i)].setBounds(548,210+i*78,200,28);fx[size_t(i)].setBounds(738,206+i*78,270,36);}
        for(int i=0;i<8;++i){notes[size_t(i)].setBounds(32+i*122,694,112,38);locks[size_t(i)].setBounds(32+i*122,740,112,34);}
    }
    void paint(Graphics& g)override{
        g.fillAll(background);g.setColour(ink);g.setFont(FontOptions(34,Font::bold));g.drawText("luma / orbit",32,18,420,50,Justification::centredLeft);
        g.setColour(line);g.drawHorizontalLine(86,32,1008);g.drawHorizontalLine(660,32,1008);
        const float cx=274,cy=320,r=150;
        g.setColour(line);g.drawEllipse(cx-r,cy-r,2*r,2*r,1.5f);g.drawEllipse(cx-112,cy-112,224,224,1.5f);g.drawEllipse(cx-72,cy-72,144,144,1);
        const int selected=int(owner.state.getRawParameterValue("scale")->load());
        for(int i=0;i<8;++i){const float a=float(i*orbit::pi/4-orbit::pi/2),x=cx+r*std::cos(a),y=cy+r*std::sin(a);const bool current=owner.liveStep.load()==i;
            g.setColour(current?lime:surface);g.fillEllipse(x-27,y-27,54,54);g.setColour(locks[size_t(i)].getToggleState()?peach:lime);g.drawEllipse(x-27,y-27,54,54,2);
            g.setColour(current?background:ink);g.setFont(FontOptions(17));g.drawText(noteName(orbit::midi(owner.liveNotes[size_t(i)].load(),selected)),int(x-28),int(y-14),56,28,Justification::centred);
            g.setColour(current?lime:muted);g.drawText(String(i+1),32+i*122,668,112,22,Justification::centred);
        }
        const int accentCount=accents.getSelectedId()==2?7:accents.getSelectedId()==3?8:5;
        for(int i=0;i<accentCount;++i){const auto a=i*2*orbit::pi/accentCount-orbit::pi/2;const float size=owner.livePlaying.load()&&owner.liveAccent.load()==i?16.0f:10.0f;g.setColour(peach);g.fillEllipse(cx+112*float(std::cos(a))-size/2,cy+112*float(std::sin(a))-size/2,size,size);}
        g.setColour(muted);g.setFont(FontOptions(17));g.drawText("FRASE",int(cx-60),int(cy-38),120,25,Justification::centred);g.setColour(ink);g.setFont(FontOptions(32));g.drawText(String(owner.livePhrase.load()),int(cx-60),int(cy-9),120,43,Justification::centred);
        g.setFont(FontOptions(16));g.setColour(muted);g.drawText("8 note. Blocca quelle da conservare.",32,501,480,30,Justification::centred);
        if(showEffects){g.setColour(ink);g.setFont(FontOptions(20));g.drawText("Spazio e movimento",548,162,460,30,Justification::centredLeft);g.setColour(muted);g.setFont(FontOptions(16));g.drawFittedText("Delay a tempo con il synth.\nA zero, ogni effetto e disattivato.\nStop interrompe anche le code.",548,544,460,95,Justification::topLeft,3);}
        const auto status=owner.hostAvailable.load()?String("DAW: ")+(owner.livePlaying.load()?"in riproduzione":"in pausa"):String("Clock interno: ")+(owner.livePlaying.load()?"in riproduzione":"premi Ascolta");
        g.drawText(status+"  |  "+String(owner.liveBpm.load(),1)+" BPM",32,786,976,24,Justification::centredLeft);
    }
private:
    using State=AudioProcessorValueTreeState;
    LumaOrbit& owner;OrbitTheme theme;TooltipWindow tips{this,700};
    std::array<TextButton,7> buttons;TextButton capture;
    std::array<Slider,9> sliders;std::array<Label,9> labels;std::array<ComboBox,8> notes;std::array<TextButton,8> locks;
    ComboBox scale,sound,accents,division;TextButton effectPage;bool showEffects=false;
    std::array<Slider,4> fx;std::array<Label,4> fxLabels;
    std::array<std::unique_ptr<State::SliderAttachment>,4> fxLinks;
    std::unique_ptr<State::ComboBoxAttachment> divisionLink;
    void updatePage(){
        for(auto& s:sliders)s.setVisible(!showEffects);for(auto& l:labels)l.setVisible(!showEffects);
        sound.setVisible(!showEffects);accents.setVisible(!showEffects);division.setVisible(showEffects);
        for(auto& s:fx)s.setVisible(showEffects);for(auto& l:fxLabels)l.setVisible(showEffects);
        effectPage.setButtonText(showEffects?"Torna al suono":"Effetti");repaint();
    }
    std::array<std::unique_ptr<State::ButtonAttachment>,7> buttonLinks;
    std::array<std::unique_ptr<State::SliderAttachment>,9> sliderLinks;
    std::array<std::unique_ptr<State::ComboBoxAttachment>,8> noteLinks;
    std::array<std::unique_ptr<State::ButtonAttachment>,8> lockLinks;
    std::unique_ptr<State::ComboBoxAttachment> scaleLink,soundLink,accentLink;
    TextButton savePreset,loadPreset,record;Label fileMessage;
    std::unique_ptr<FileChooser> chooser;String fileStatus="WAV: massimo 5 minuti";bool recording=false;
    void chooseFile(int action){
        if(chooser)return;
        const auto extension=action==2?"*.wav":"*.orbit";
        chooser=std::make_unique<FileChooser>(action==1?"Apri preset Orbit":"Salva Orbit",File::getSpecialLocation(File::userDocumentsDirectory).getChildFile(action==2?"Orbit.wav":"Orbit.orbit"),extension);
        const auto flags=FileBrowserComponent::canSelectFiles|(action==1?FileBrowserComponent::openMode:FileBrowserComponent::saveMode|FileBrowserComponent::warnAboutOverwriting);
        Component::SafePointer<OrbitEditor> safe(this);
        chooser->launchAsync(flags,[safe,action](const FileChooser& dialog){
            if(!safe)return;const auto file=dialog.getResult();
            if(file!=File()){
                bool ok=false;
                if(action==0)ok=safe->owner.savePreset(file.withFileExtension("orbit"));
                if(action==1)ok=safe->owner.loadPreset(file);
                if(action==2){ok=safe->owner.recorder.start(file.withFileExtension("wav"),safe->owner.getSampleRate(),safe->owner.getTotalNumOutputChannels());safe->recording=ok;}
                safe->fileStatus=ok?(action==0?"Preset salvato":action==1?"Preset caricato":"Registrazione avviata"):"Operazione non riuscita";
            }
            safe->chooser.reset();
        });
    }
    int previousScale=-1;
    void timerCallback()override{
        const bool active=owner.recorder.active.load();
        if(recording&&!active){owner.recorder.stop();recording=false;fileStatus=owner.recorder.failed.load()?"Errore: WAV incompleto":"WAV salvato (5 minuti)";}
        record.setButtonText(active?"Ferma e salva WAV":"Registra WAV");
        fileMessage.setText(active?"REC  "+String(owner.recorder.seconds(),1)+" s / 300 s":fileStatus,dontSendNotification);
        const bool host=owner.hostAvailable.load();buttons[0].setEnabled(!host);sliders[0].setEnabled(!host);
        const int selected=int(owner.state.getRawParameterValue("scale")->load());
        if(selected!=previousScale){previousScale=selected;for(auto& combo:notes){const int selectedId=combo.getSelectedId();for(int n=0;n<8;++n)combo.changeItemText(n+1,noteName(orbit::midi(n,selected)));combo.setSelectedId(selectedId,dontSendNotification);}}
        repaint();
    }
};
}
AudioProcessorEditor* LumaOrbit::createEditor(){return new OrbitEditor(*this);}
