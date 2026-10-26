#pragma once
#include "ParameterManager.h"
namespace aurum
{
class PresetManager
{
    ParameterManager &parameters;
    bool compact;
    std::atomic<int> &width;
    std::array<juce::MemoryBlock, 2> states;
    int active = 0;
    bool tracking = true;
    juce::File directory() const
    {
        const auto overrideDir = juce::SystemStats::getEnvironmentVariable("AURUM_PRESET_DIR", "");
        if (overrideDir.isNotEmpty())
            return juce::File(overrideDir).getChildFile(compact ? "Track" : "Master");
        return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
            .getChildFile("AurumClip")
            .getChildFile(compact ? "Track" : "Master");
    }
    void capture(juce::MemoryBlock &b)
    {
        parameters.save(b, compact, width.load());
    }
    bool apply(const juce::MemoryBlock &b)
    {
        int w = width.load();
        if (!parameters.restore(b.getData(), int(b.getSize()), compact, w))
            return false;
        width = w;
        return true;
    }

  public:
    juce::String name = "Default";
    bool userPreset = false;
    explicit PresetManager(ParameterManager &p, bool c, std::atomic<int> &w)
        : parameters(p), compact(c), width(w)
    {
        if (juce::SystemStats::getEnvironmentVariable("AURUM_NO_SETTINGS", "") == "1")
            return;
        juce::MemoryBlock bytes;
        if (directory().getChildFile("Default.aurum").loadFileAsData(bytes))
        {
            restoreSession(bytes.getData(), int(bytes.getSize()));
            name = "Default";
            userPreset = true;
        }
    }
    void selectAB(int index)
    {
        index = juce::jlimit(0, 1, index);
        if (index == active)
            return;
        capture(states[size_t(active)]);
        if (tracking)
        {
            states[size_t(index)] = states[size_t(active)];
            tracking = false;
        }
        active = index;
        apply(states[size_t(active)]);
    }
    int activeAB() const noexcept
    {
        return active;
    }
    void copyAB(int from, int to)
    {
        if (from == active)
            capture(states[size_t(from)]);
        if (states[size_t(from)].isEmpty())
            capture(states[size_t(from)]);
        states[size_t(to)] = states[size_t(from)];
        tracking = false;
        if (to == active)
            apply(states[size_t(to)]);
    }
    void saveSession(juce::MemoryBlock &bytes)
    {
        capture(states[size_t(active)]);
        if (tracking)
            states[size_t(1 - active)] = states[size_t(active)];
        auto xml = juce::AudioProcessor::getXmlFromBinary(states[size_t(active)].getData(),
                                                          int(states[size_t(active)].getSize()));
        xml->setAttribute("abSchema", 1);
        xml->setAttribute("abActive", active);
        xml->setAttribute("abTracking", tracking);
        xml->setAttribute("presetName", name);
        xml->setAttribute("presetUser", userPreset);
        xml->setAttribute("abA", states[0].toBase64Encoding());
        xml->setAttribute("abB", states[1].toBase64Encoding());
        juce::AudioProcessor::copyXmlToBinary(*xml, bytes);
    }
    bool restoreSession(const void *data, int size)
    {
        if (size <= 0 || size > 1024 * 1024)
            return false;
        auto xml = juce::AudioProcessor::getXmlFromBinary(data, size);
        if (!xml)
            return false;
        std::array<juce::MemoryBlock, 2> restored;
        bool has = xml->hasAttribute("abSchema");
        int w = width.load();
        if (has)
        {
            if (xml->getIntAttribute("abSchema") != 1)
                return false;
            for (int i = 0; i < 2; ++i)
            {
                if (!restored[size_t(i)].fromBase64Encoding(
                        xml->getStringAttribute(i == 0 ? "abA" : "abB")))
                    return false;
                if (!parameters.restore(restored[size_t(i)].getData(),
                                        int(restored[size_t(i)].getSize()), compact, w, false))
                    return false;
            }
        }
        if (!parameters.restore(data, size, compact, w))
            return false;
        width = w;
        if (has)
        {
            states = std::move(restored);
            active = juce::jlimit(0, 1, xml->getIntAttribute("abActive"));
            tracking = xml->getBoolAttribute("abTracking");
        }
        else
        {
            states = {};
            active = 0;
            tracking = true;
        }
        name = xml->getStringAttribute("presetName", "Session").substring(0, 80);
        userPreset = xml->getBoolAttribute("presetUser", false);
        return true;
    }
    juce::StringArray factoryNames() const
    {
        return {"Default", "Transparent Master", "Drum Density", "Parallel Punch"};
    }
    void factory(int index)
    {
        const auto names = factoryNames();
        index = juce::jlimit(0, names.size() - 1, index);
        for (auto *param : parameters.state.processor.getParameters())
            if (auto *p = dynamic_cast<juce::RangedAudioParameter *>(param))
                p->setValueNotifyingHost(p->getDefaultValue());
        auto set = [&](const char *id, float value)
        {
            if (auto *p = parameters.state.getParameter(id))
                p->setValueNotifyingHost(p->convertTo0to1(value));
        };
        if (index == 1)
        {
            set("clipMode", 3);
            set("ceiling", -1);
            set("goldModern", .8f);
            set("quality", 1);
        }
        if (index == 2)
        {
            set("clipMode", 2);
            set("goldMode", 1);
            set("goldClassic", 3);
            set("input", 3);
            set("ceiling", -1);
            set("output", -1);
        }
        if (index == 3)
        {
            set("clipMode", 1);
            set("input", 6);
            set("goldMode", 1);
            set("goldClassic", 4);
            set("blend", 50);
            set("ceiling", -3);
            set("output", -2);
        }
        name = names[index];
        userPreset = false;
        states = {};
        active = 0;
        tracking = true;
    }
    juce::Array<juce::File> userPresets() const
    {
        juce::Array<juce::File> files;
        if (directory().isDirectory())
            directory().findChildFiles(files, juce::File::findFiles, false, "*.aurum");
        files.sort();
        return files;
    }
    bool saveUser(juce::String presetName)
    {
        presetName = juce::File::createLegalFileName(presetName.trim()).substring(0, 64);
        if (presetName.isEmpty())
            return false;
        directory().createDirectory();
        juce::MemoryBlock bytes;
        saveSession(bytes);
        juce::TemporaryFile temp(directory().getChildFile(presetName + ".aurum"));
        if (!temp.getFile().replaceWithData(bytes.getData(), bytes.getSize()) ||
            !temp.overwriteTargetFileWithTemporary())
            return false;
        name = presetName;
        userPreset = true;
        return true;
    }
    bool loadUser(const juce::File &file)
    {
        if (file.getParentDirectory() != directory())
            return false;
        juce::MemoryBlock bytes;
        if (!file.loadFileAsData(bytes) || !restoreSession(bytes.getData(), int(bytes.getSize())))
            return false;
        name = file.getFileNameWithoutExtension();
        userPreset = true;
        return true;
    }
    bool removeUser(const juce::File &file)
    {
        return file.getParentDirectory() == directory() && file.hasFileExtension("aurum") &&
               file.deleteFile();
    }
    bool renameUser(const juce::File &file, juce::String newName)
    {
        if (file.getParentDirectory() != directory())
            return false;
        newName = juce::File::createLegalFileName(newName.trim()).substring(0, 64);
        auto target = directory().getChildFile(newName + ".aurum");
        if (newName.isEmpty() || target.exists())
            return false;
        return file.moveFileTo(target);
    }
};
} // namespace aurum
