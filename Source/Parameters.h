#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace luma {
using State = juce::AudioProcessorValueTreeState;
enum Control { blend, detune, sub, cutoff, resonance, toneAmount, toneDecay,
               attack, decay, sustain, release, chorus, gain, controlCount };
struct ParameterSpec {
    const char* id;
    const char* name;
    const char* unit;
    const char* tip;
    float min, max, initial, skew;
    int decimals;
};
// Stable IDs are shared by DSP, attachments, presets and saved sessions.
inline constexpr std::array<ParameterSpec, controlCount> specs {{
    {"blend", "OSC 2", "%", "Miscela un secondo oscillatore della stessa forma d'onda.", 0,1,0,1,0},
    {"detune", "DETUNE", "ct", "Scordatura del secondo oscillatore in centesimi di semitono.", 0,25,0,1,1},
    {"sub", "SUB", "%", "Aggiunge una sinusoide un'ottava sotto.", 0,1,0,1,0},
    {"cutoff", "CUTOFF", "Hz", "Frequenza di base del filtro passa basso.", 30,18000,4500,0.3f,0},
    {"resonance", "RESONANCE", "%", "Enfatizza le frequenze vicine al cutoff.", 0.1f,0.9f,0.2f,1,0},
    {"toneAmount", "ENV AMOUNT", "oct", "Quanto l'inviluppo apre il filtro a ogni nota.", 0,5,0,1,1},
    {"toneDecay", "ENV DECAY", "s", "Tempo di chiusura del filtro dopo l'attacco.", 0.03f,4,0.4f,0.35f,2},
    {"attack", "ATTACK", "s", "Tempo di ingresso del volume.", 0.005f,3,0.02f,0.35f,2},
    {"decay", "DECAY", "s", "Tempo per raggiungere il livello Sustain.", 0.01f,3,0.3f,0.35f,2},
    {"sustain", "SUSTAIN", "%", "Volume mantenuto mentre tieni premuta la nota.", 0,1,0.7f,1,0},
    {"release", "RELEASE", "s", "Tempo di uscita dopo il rilascio della nota.", 0.01f,5,0.5f,0.35f,2},
    {"chorus", "CHORUS", "%", "Aggiunge movimento e apertura stereo. A zero il suono resta asciutto.", 0,1,0,1,0},
    {"gain", "OUTPUT", "dB", "Volume finale del sintetizzatore.", -48,0,-12,1,1}
}};
inline State::ParameterLayout parameters() {
    State::ParameterLayout p;
    p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"wave",1}, "Wave",
        juce::StringArray{"Sine","Triangle","Saw"}, 2));
    for(const auto& s : specs)
        p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{s.id,1}, s.name,
            juce::NormalisableRange<float>{s.min,s.max,0,s.skew},s.initial));
    return p;
}
struct Preset {
    const char* name;
    const char* description;
    int wave;
    std::array<float,controlCount> values;
};
inline constexpr std::array<Preset,6> presets {{
    {"01 / Init", "Il suono originale. Una tela pulita da cui partire.", 2,
        {0,0,0,4500,0.2f,0,0.4f,0.02f,0.3f,0.7f,0.5f,0,-12}},
    {"02 / Velvet Pad", "Accordi morbidi, due oscillatori e un chorus ampio.", 2,
        {0.5f,9,0.12f,1800,0.18f,1.2f,2.5f,0.8f,1.4f,0.75f,2.5f,0.65f,-10}},
    {"03 / Rubber Bass", "Sub profondo e filtro elastico. Prova le ottave basse.", 2,
        {0.25f,4,0.65f,180,0.55f,3.5f,0.22f,0.005f,0.22f,0.65f,0.12f,0,-8}},
    {"04 / Glass Pluck", "Attacco limpido, coda breve. Suona arpeggi e accordi.", 1,
        {0.45f,12,0.05f,1200,0.28f,3.5f,0.16f,0.005f,0.65f,0.08f,0.6f,0.45f,-8}},
    {"05 / Solar Lead", "Dente di sega brillante e un leggero movimento stereo.", 2,
        {0.5f,7,0.15f,2600,0.35f,2,0.5f,0.015f,0.3f,0.8f,0.3f,0.25f,-12}},
    {"06 / Soft Keys", "Triangolari calde, risposta morbida e coda naturale.", 1,
        {0.35f,5,0.2f,2000,0.16f,1.4f,0.7f,0.015f,0.8f,0.3f,0.8f,0.35f,-8}}
}};
using ParameterPointers = std::array<std::atomic<float>*,controlCount>;
inline ParameterPointers pointers(State& state) {
    ParameterPointers result{};
    for(size_t i=0;i<result.size();++i) result[i]=state.getRawParameterValue(specs[i].id);
    return result;
}
} // namespace luma
