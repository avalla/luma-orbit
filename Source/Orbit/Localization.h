#pragma once
#include "Translations.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace orbit::localization {
inline juce::String resolve(juce::String language){
    language=language.toLowerCase().replaceCharacter('_','-').upToFirstOccurrenceOf("-",false,false);
    for(const auto* id:languageIds)if(language==id)return language;
    return "en";
}
inline juce::String translate(const juce::String& key,const juce::String& language){
    const auto code=resolve(language);size_t index=0;for(size_t i=0;i<std::size(languageIds);++i)if(code==languageIds[i])index=i;
    for(const auto& entry:messages)if(key==juce::String::fromUTF8(entry.source))return juce::String::fromUTF8(entry.values[index]);
    return key;
}
inline juce::File preferenceFile(){
    auto folder=juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
   #if JUCE_MAC
    folder=folder.getChildFile("Application Support");
   #endif
    return folder.getChildFile("Luma Audio/Orbit/language.txt");
}
inline juce::String preferred(){
    const auto saved=preferenceFile().loadFileAsString().trim();
    for(const auto* id:languageIds)if(saved==id)return saved;
    return resolve(juce::SystemStats::getUserLanguage());
}
inline bool save(const juce::String& language){
    const auto file=preferenceFile();return file.getParentDirectory().createDirectory().wasOk()&&file.replaceWithText(resolve(language));
}
}
