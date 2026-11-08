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
        setLookAndFeel(&theme);setSize(1040,820);
        const char* toggleIds[]={"run","sync","flow","drums","kick","clap","hat"};
        const char* toggleNames[]={"Ascolta","Sync DAW","Flusso","Ritmo","Cassa","Clap","Hi-hat"};
        for(size_t i=0;i<buttons.size();++i){auto& b=buttons[i];b.setButtonText(toggleNames[i]);b.setClickingTogglesState(true);addAndMakeVisible(b);buttonLinks[i]=std::make_unique<State::ButtonAttachment>(owner.state,toggleIds[i],b);}
        addAndMakeVisible(capture);capture.setButtonText("Cattura le note");capture.onClick=[this]{owner.capture();};capture.setTooltip("Conserva le note correnti e disattiva Flusso. Le note si salvano nella sessione DAW.");
        scale.addItemList({"La minore","Do maggiore","La pentatonica"},1);sound.addItemList({"Glass","Rubber","Mist","Soft Keys","Bell"},1);accents.addItemList({"5 accenti","7 accenti","8 accenti"},1);
        for(auto* c:{&scale,&sound,&accents})addAndMakeVisible(c);
        scaleLink=std::make_unique<State::ComboBoxAttachment>(owner.state,"scale",scale);soundLink=std::make_unique<State::ComboBoxAttachment>(owner.state,"sound",sound);accentLink=std::make_unique<State::ComboBoxAttachment>(owner.state,"accents",accents);
        const char* sliderIds[]={"bpm","explore","tone","bass","volume","density","depth","drumVolume"};
        const char* names[]={"Tempo interno","Esplora","Luminosita","Sub intonato","Volume generale","Densita ritmo","Profondita cassa","Volume ritmo"};
        for(size_t i=0;i<sliders.size();++i){auto& s=sliders[i];s.setSliderStyle(Slider::LinearHorizontal);s.setTextBoxStyle(Slider::TextBoxRight,false,78,30);s.setNumDecimalPlacesToDisplay(i==0?0:2);addAndMakeVisible(s);
            labels[i].setText(names[i],dontSendNotification);labels[i].setFont(FontOptions(17));addAndMakeVisible(labels[i]);sliderLinks[i]=std::make_unique<State::SliderAttachment>(owner.state,sliderIds[i],s);}
        for(int i=0;i<8;++i){auto index=size_t(i);for(int n=0;n<8;++n)notes[index].addItem(String(n+1),n+1);addAndMakeVisible(notes[index]);
            noteLinks[index]=std::make_unique<State::ComboBoxAttachment>(owner.state,"note"+String(i),notes[index]);
            locks[index].setButtonText("Fisso");locks[index].setClickingTogglesState(true);addAndMakeVisible(locks[index]);lockLinks[index]=std::make_unique<State::ButtonAttachment>(owner.state,"lock"+String(i),locks[index]);}
        timerCallback();startTimerHz(30);
    }
    ~OrbitEditor() override{stopTimer();setLookAndFeel(nullptr);}
    void resized()override{
        buttons[0].setBounds(640,22,112,44);buttons[1].setBounds(764,22,120,44);buttons[2].setBounds(896,22,112,44);
        scale.setBounds(32,106,220,42);sound.setBounds(548,106,220,42);accents.setBounds(780,106,228,42);
        for(int i=0;i<8;++i){labels[size_t(i)].setBounds(548,170+i*58,200,24);sliders[size_t(i)].setBounds(738,166+i*58,270,36);}
        for(int i=3;i<7;++i)buttons[size_t(i)].setBounds(32+(i-3)*122,540,112,42);
        capture.setBounds(32,601,228,44);
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
        g.drawText("5 timbri / sub intonato",280,606,240,35,Justification::centredLeft);
        const auto status=owner.hostAvailable.load()?String("DAW: ")+(owner.livePlaying.load()?"in riproduzione":"in pausa"):String("Clock interno: ")+(owner.livePlaying.load()?"in riproduzione":"premi Ascolta");
        g.drawText(status+"  |  "+String(owner.liveBpm.load(),1)+" BPM",32,786,976,24,Justification::centredLeft);
    }
private:
    using State=AudioProcessorValueTreeState;
    LumaOrbit& owner;OrbitTheme theme;TooltipWindow tips{this,700};
    std::array<TextButton,7> buttons;TextButton capture;
    std::array<Slider,8> sliders;std::array<Label,8> labels;std::array<ComboBox,8> notes;std::array<TextButton,8> locks;
    ComboBox scale,sound,accents;
    std::array<std::unique_ptr<State::ButtonAttachment>,7> buttonLinks;
    std::array<std::unique_ptr<State::SliderAttachment>,8> sliderLinks;
    std::array<std::unique_ptr<State::ComboBoxAttachment>,8> noteLinks;
    std::array<std::unique_ptr<State::ButtonAttachment>,8> lockLinks;
    std::unique_ptr<State::ComboBoxAttachment> scaleLink,soundLink,accentLink;
    int previousScale=-1;
    void timerCallback()override{
        const bool host=owner.hostAvailable.load();buttons[0].setEnabled(!host);sliders[0].setEnabled(!host);
        const int selected=int(owner.state.getRawParameterValue("scale")->load());
        if(selected!=previousScale){previousScale=selected;for(auto& combo:notes){const int selectedId=combo.getSelectedId();for(int n=0;n<8;++n)combo.changeItemText(n+1,noteName(orbit::midi(n,selected)));combo.setSelectedId(selectedId,dontSendNotification);}}
        repaint();
    }
};
}
AudioProcessorEditor* LumaOrbit::createEditor(){return new OrbitEditor(*this);}
