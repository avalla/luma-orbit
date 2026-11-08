#include "Plugin.h"
using namespace juce;
class Editor final : public AudioProcessorEditor, private Timer {
    Luma& synthProcessor;
    LookAndFeel_V4 theme;
    ComboBox wave,preset;
    Label description;
    std::unique_ptr<luma::State::ComboBoxAttachment> waveAttachment;
    std::array<Slider,luma::controlCount> knobs;
    std::array<Label,luma::controlCount> labels;
    std::array<std::unique_ptr<luma::State::SliderAttachment>,luma::controlCount> attachments;
    MidiKeyboardComponent keys;
    TextButton playKeyboard{"Suona con la tastiera"};
    TooltipWindow tips{this,650};
    void timerCallback() override {
        const auto match=synthProcessor.matchingPreset();
        if(preset.getSelectedId()!=match+1) preset.setSelectedId(match+1,dontSendNotification);
        description.setText(match>=0?luma::presets[size_t(match)].description:"Suono personalizzato / scegli un preset per esplorare altri timbri.",dontSendNotification);
    }
public:
    explicit Editor(Luma& p) : AudioProcessorEditor(p),synthProcessor(p),keys(p.keyboard,MidiKeyboardComponent::horizontalKeyboard) {
        theme.setColour(Slider::rotarySliderFillColourId,Colour(0xffb9ed8b));
        theme.setColour(Slider::rotarySliderOutlineColourId,Colour(0xff34433c));
        theme.setColour(Slider::thumbColourId,Colour(0xffe7f0e6));
        theme.setColour(Slider::textBoxTextColourId,Colour(0xffe7f0e6));
        theme.setColour(Slider::textBoxOutlineColourId,Colours::transparentBlack);
        theme.setColour(ComboBox::backgroundColourId,Colour(0xff26372e));
        theme.setColour(ComboBox::outlineColourId,Colour(0xff42544a));
        theme.setColour(TextButton::buttonColourId,Colour(0xff304534));
        setLookAndFeel(&theme);
        wave.addItemList({"Sine","Triangle","Saw"},1); wave.setTitle("Forma d'onda"); addAndMakeVisible(wave);
        waveAttachment=std::make_unique<luma::State::ComboBoxAttachment>(p.state,"wave",wave);
        for(size_t i=0;i<luma::presets.size();++i) preset.addItem(luma::presets[i].name,int(i)+1);
        preset.setTextWhenNothingSelected("Custom / modified"); preset.setTitle("Preset");
        preset.onChange=[this] { if(preset.getSelectedId()>0) synthProcessor.applyFactoryPreset(preset.getSelectedId()-1); };
        addAndMakeVisible(preset); addAndMakeVisible(description);
        description.setColour(Label::textColourId,Colour(0xffa6b8aa)); description.setFont(FontOptions(12.0f));
        for(size_t i=0;i<knobs.size();++i) {
            const auto& s=luma::specs[i]; auto& k=knobs[i];
            k.setSliderStyle(Slider::RotaryHorizontalVerticalDrag); k.setTextBoxStyle(Slider::TextBoxBelow,false,105,22);
            k.setName(s.name); k.setTooltip(s.tip);
            k.setDoubleClickReturnValue(true,s.initial); addAndMakeVisible(k);
            labels[i].setText(s.name,dontSendNotification); labels[i].setJustificationType(Justification::centred);
            labels[i].setFont(FontOptions(11.0f)); labels[i].setColour(Label::textColourId,Colour(0xffafc1b4)); addAndMakeVisible(labels[i]);
            attachments[i]=std::make_unique<luma::State::SliderAttachment>(p.state,s.id,k);
            const bool percent=String(s.unit)=="%";
            k.textFromValueFunction=[s,percent](double v) { return String::formatted("%.*f",s.decimals,v*(percent?100:1))+" "+s.unit; };
            k.valueFromTextFunction=[percent](const String& text) { return text.getDoubleValue()/(percent?100:1); }; k.updateText();
        }
        keys.clearKeyMappings(); const String mapping="awsedftgyhujk";
        for(int i=0;i<mapping.length();++i) keys.setKeyPressForNote(KeyPress(mapping[i],ModifierKeys::noModifiers,0),i);
        keys.setKeyPressBaseOctave(5); keys.setAvailableRange(36,84); keys.setLowestVisibleKey(48); keys.setKeyWidth(25);
        keys.setWantsKeyboardFocus(true); addAndMakeVisible(keys);
        playKeyboard.onClick=[this] { keys.grabKeyboardFocus(); }; addAndMakeVisible(playKeyboard);
        setSize(960,656); timerCallback(); startTimerHz(5);
        Component::SafePointer<Editor> safe(this);
        MessageManager::callAsync([safe] { if(safe!=nullptr && safe->isShowing()) safe->keys.grabKeyboardFocus(); });
    }
    ~Editor() override { stopTimer(); setLookAndFeel(nullptr); }
    void paint(Graphics& g) override {
        g.fillAll(Colour(0xff111b19)); g.setColour(Colour(0xffe7f0e6));
        g.setFont(FontOptions(36.0f,Font::bold)); g.drawText("luma",26,14,170,48,Justification::centredLeft);
        g.setColour(Colour(0xffa6b8aa)); g.setFont(FontOptions(11.0f));
        g.drawText("EIGHT VOICES / TWO OSCILLATORS / A LITTLE MORE COLOUR",28,64,550,22,Justification::centredLeft);
        auto panel=[&g](Rectangle<float> rect,const String& title,const String& subtitle) {
            g.setColour(Colour(0xff1c2a24)); g.fillRoundedRectangle(rect,10);
            g.setColour(Colour(0xff3b4b41)); g.drawRoundedRectangle(rect.reduced(0.5f),10,1);
            g.setFont(FontOptions(12.0f,Font::bold)); g.setColour(Colour(0xffd2e5c4));
            g.drawText(title,int(rect.getX()+18),int(rect.getY()+12),int(rect.getWidth()-36),20,Justification::centredLeft);
            g.setFont(FontOptions(11.0f)); g.setColour(Colour(0xff91a796));
            g.drawText(subtitle,int(rect.getX()+18),int(rect.getY()+34),int(rect.getWidth()-36),18,Justification::centredLeft);
        };
        panel({24,128,342,204},"01 / SOURCES","Forma d'onda, spessore e un'ottava di profondita.");
        panel({382,128,554,204},"02 / TONE","Il filtro si apre a ogni nota, poi torna al cutoff.");
        panel({24,348,596,172},"03 / AMPLITUDE","Disegna l'ingresso, il corpo e la coda del suono.");
        panel({636,348,300,172},"04 / SPACE + LEVEL","Movimento stereo e volume finale.");
        g.setColour(Colour(0xffa6b8aa)); g.setFont(FontOptions(11.0f));
        g.drawText("A S D F G H J K = Do Re Mi Fa Sol La Si Do   /   W E T Y U = diesis",26,613,700,24,Justification::centredLeft);
    }
    void resized() override {
        preset.setBounds(644,25,292,40); description.setBounds(25,91,910,24); wave.setBounds(44,185,302,30);
        auto place=[this](int id,int x,int y,int w,int h) { labels[size_t(id)].setBounds(x,y,w,20); knobs[size_t(id)].setBounds(x,y+20,w,h-20); };
        for(int i=0;i<3;++i) place(i,42+i*101,219,101,100);
        for(int i=0;i<4;++i) place(luma::cutoff+i,397+i*131,192,131,126);
        for(int i=0;i<4;++i) place(luma::attack+i,42+i*140,403,140,106);
        place(luma::chorus,654,403,132,106); place(luma::gain,786,403,132,106);
        keys.setBounds(24,538,912,64); playKeyboard.setBounds(740,607,196,40);
    }
};
AudioProcessorEditor* Luma::createEditor() { return new Editor(*this); }
