#include "Processor.h"
using namespace juce;
namespace {
float powerDb(float power) { return power>0 ? 10.0f*std::log10(power) : -std::numeric_limits<float>::infinity(); }
float amplitudeDb(float amplitude) { return amplitude>0 ? 20.0f*std::log10(amplitude) : -std::numeric_limits<float>::infinity(); }
String dbText(float db) { return std::isfinite(db)?String(db,1):String("-inf"); }
}
class RmsEditor final : public AudioProcessorEditor,private Timer {
    LumaRms& meterProcessor;
    LookAndFeel_V4 theme;
    ComboBox window;
    Label windowLabel;
    TextButton reset{"Reset peak hold"};
    std::unique_ptr<AudioProcessorValueTreeState::ComboBoxAttachment> attachment;
    std::array<float,2> rmsDb{},heldDb{};
    float combinedDb=-std::numeric_limits<float>::infinity();
    float windowFill=0;
    int channels=2;
    unsigned lastBlock=0;
    double lastAudioTime=0;
    bool receivingAudio=false;
    void timerCallback() override {
        channels=meterProcessor.measuredChannels.load(std::memory_order_relaxed);
        float combinedPower=0;
        for(size_t ch=0;ch<2;++ch) {
            const auto power=meterProcessor.power[ch].load(std::memory_order_relaxed);
            rmsDb[ch]=powerDb(power);
            heldDb[ch]=amplitudeDb(meterProcessor.peakHold[ch].load(std::memory_order_relaxed));
            if(int(ch)<channels) combinedPower+=power;
        }
        combinedDb=powerDb(combinedPower/float(jmax(1,channels)));
        windowFill=meterProcessor.fill.load(std::memory_order_relaxed); repaint();
        const auto block=meterProcessor.processedBlocks.load(std::memory_order_relaxed);
        const auto now=Time::getMillisecondCounterHiRes();
        if(block!=lastBlock) { lastBlock=block; lastAudioTime=now; }
        receivingAudio=lastAudioTime>0 && now-lastAudioTime<500;
    }
public:
    explicit RmsEditor(LumaRms& p) : AudioProcessorEditor(p),meterProcessor(p) {
        theme.setColour(ComboBox::backgroundColourId,Colour(0xff24343a));
        theme.setColour(ComboBox::outlineColourId,Colour(0xff40565d));
        theme.setColour(ComboBox::textColourId,Colour(0xffedf4ee));
        theme.setColour(TextButton::buttonColourId,Colour(0xff24343a));
        setLookAndFeel(&theme);
        window.addItemList({"100 ms","400 ms","1000 ms"},1); window.setTitle("Finestra RMS");
        window.setTooltip("Durata della finestra mobile: un valore piu lungo rende la lettura piu stabile.");
        addAndMakeVisible(window);
        attachment=std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment>(p.state,"window",window);
        windowLabel.setText("FINESTRA RMS",dontSendNotification); windowLabel.setFont(FontOptions(11.0f));
        windowLabel.setColour(Label::textColourId,Colour(0xff9fb2b8)); addAndMakeVisible(windowLabel);
        reset.onClick=[this] { meterProcessor.resetRequested.store(true,std::memory_order_relaxed); };
        reset.setTooltip("Azzera i picchi massimi rilevati dal prossimo blocco audio."); addAndMakeVisible(reset);
        setSize(660,508); timerCallback(); startTimerHz(30);
    }
    ~RmsEditor() override { stopTimer(); setLookAndFeel(nullptr); }
    void resized() override {
        windowLabel.setBounds(446,88,188,20); window.setBounds(448,112,186,40); reset.setBounds(448,164,186,40);
    }
    void paint(Graphics& g) override {
        g.fillAll(Colour(0xff101b21));
        g.setColour(Colour(0xffedf4ee)); g.setFont(FontOptions(27.0f,Font::bold));
        g.drawText("luma / RMS",26,18,300,38,Justification::centredLeft);
        g.setFont(FontOptions(11.0f)); g.setColour(Colour(0xffaee6cb));
        g.drawText("AUDIO INALTERATO / 0 LATENZA",350,27,284,22,Justification::centredRight);
        g.setColour(Colour(0xff92aab4)); g.setFont(FontOptions(11.0f));
        g.drawText(channels==1?"RMS / MONO":"RMS / MEDIA ENERGETICA L + R",28,84,395,22,Justification::centredLeft);
        g.setColour(Colour(0xffd3f3d2)); g.setFont(FontOptions(64.0f,Font::bold));
        g.drawText(dbText(combinedDb),24,103,300,82,Justification::centredLeft);
        g.setColour(Colour(0xff9fb2b8)); g.setFont(FontOptions(16.0f)); g.drawText("dBFS",332,142,80,26,Justification::centredLeft);
        g.setFont(FontOptions(11.0f));
        const auto status=windowFill==0 ? String("Avvia la riproduzione per misurare l'audio.")
            : !receivingAudio ? String("Riproduzione ferma / ultima lettura RMS")
            : windowFill<1 ? "Riempimento finestra: "+String(roundToInt(windowFill*100))+"%"
            : String("Audio in ingresso / misura in tempo reale");
        g.drawText(status,28,187,400,22,Justification::centredLeft);
        const Rectangle<float> panel{24,222,612,218};
        g.setColour(Colour(0xff192a31)); g.fillRoundedRectangle(panel,12);
        g.setColour(Colour(0xff354851)); g.drawRoundedRectangle(panel.reduced(0.5f),12,1);
        auto position=[](float db) { return 44.0f+572.0f*jlimit(0.0f,1.0f,(db+60.0f)/60.0f); };
        for(size_t ch=0;ch<2;++ch) {
            const auto y=240+int(ch)*82;
            const bool active=int(ch)<channels;
            const auto accent=ch==0?Colour(0xffaee6cb):Colour(0xff8ecbdc);
            g.setColour(active?Colour(0xffedf4ee):Colour(0xff6b7b82)); g.setFont(FontOptions(12.0f,Font::bold));
            g.drawText(channels==1?(ch==0?"MONO":"R / NON ATTIVO"):(ch==0?"LEFT":"RIGHT"),44,y,180,20,Justification::centredLeft);
            g.setFont(FontOptions(16.0f,Font::bold));
            g.drawText(active?dbText(rmsDb[ch])+" dBFS":"--",400,y,216,22,Justification::centredRight);
            const Rectangle<float> bar{44,float(y+28),572,16};
            g.setColour(Colour(0xff0d171d)); g.fillRoundedRectangle(bar,4);
            if(active) {
                const auto width=position(rmsDb[ch])-44;
                if(width>0) {
                    g.setGradientFill(ColourGradient(accent.darker(0.3f),44,0,accent,616,0,false));
                    g.fillRoundedRectangle(bar.withWidth(width),4);
                }
                if(heldDb[ch]>-60) {
                    g.setColour(heldDb[ch]>=0?Colour(0xffff8578):Colour(0xfff0c88a));
                    g.fillRect(position(heldDb[ch])-2,bar.getY()-2,2.0f,20.0f);
                }
                g.setColour(heldDb[ch]>=0?Colour(0xffff8578):Colour(0xff9fb2b8)); g.setFont(FontOptions(11.0f));
                g.drawText("PEAK HOLD  "+dbText(heldDb[ch])+" dBFS",44,y+48,572,18,Justification::centredRight);
            }
        }
        g.setFont(FontOptions(10.0f)); g.setColour(Colour(0xff829ba5));
        for(int tick=-60;tick<=0;tick+=12) {
            const auto x=position(float(tick)); g.drawVerticalLine(roundToInt(x),406,412);
            g.drawText(String(tick),roundToInt(x)-16,415,32,16,Justification::centred);
        }
        g.setColour(Colour(0xff9fb2b8)); g.setFont(FontOptions(11.0f));
        g.drawText("RMS matematico: sinusoide a picco 0 dBFS = -3.01 dBFS RMS.",26,455,610,20,Justification::centredLeft);
        g.drawText("Livello non ponderato / non e una misura LUFS. Picco campione, non true peak.",26,477,610,18,Justification::centredLeft);
    }
};
AudioProcessorEditor* LumaRms::createEditor() { return new RmsEditor(*this); }
