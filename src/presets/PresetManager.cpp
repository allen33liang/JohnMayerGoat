#include "PresetManager.h"

juce::File PresetManager::getPresetFolder() const
{
    auto dir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                   .getChildFile("JohnMayerGoat")
                   .getChildFile("presets");
    dir.createDirectory();
    return dir;
}

juce::StringArray PresetManager::listPresetNames() const
{
    juce::StringArray names;
    for (const auto& f : getPresetFolder().findChildFiles(juce::File::findFiles, false, "*.json"))
        names.add(f.getFileNameWithoutExtension());
    names.sortNatural();
    return names;
}

bool PresetManager::savePreset(const juce::String& name, const juce::var& json) const
{
    if (name.trim().isEmpty())
        return false;

    auto file = getPresetFolder().getChildFile(name.trim() + ".json");
    return file.replaceWithText(juce::JSON::toString(json, true));
}

juce::var PresetManager::loadPreset(const juce::String& name) const
{
    auto file = getPresetFolder().getChildFile(name + ".json");
    if (! file.existsAsFile())
        return {};
    return juce::JSON::parse(file);
}
