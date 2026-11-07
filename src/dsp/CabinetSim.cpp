#include "CabinetSim.h"

juce::File CabinetSim::defaultIrFolder()
{
    auto legacy = juce::File::getSpecialLocation(juce::File::userHomeDirectory)
                      .getChildFile("Library")
                      .getChildFile("JohnMayerGoat")
                      .getChildFile("IRs");
    auto standard = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                        .getChildFile("JohnMayerGoat")
                        .getChildFile("IRs");

    if (legacy.isDirectory() && ! legacy.findChildFiles(juce::File::findFiles, true, "*.wav").isEmpty())
        return legacy;

    standard.createDirectory();
    return standard;
}

void CabinetSim::prepare(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    bodySm.reset(sampleRate, 0.04);
    airSm.reset(sampleRate, 0.04);
    gainSm.reset(sampleRate, 0.05);
    bodySm.setCurrentAndTargetValue(body.load());
    airSm.setCurrentAndTargetValue(air.load());
    updateTargetGain();
    gainSm.setCurrentAndTargetValue(targetGain.load());

    processSpec = { sampleRate, (juce::uint32) juce::jmax(1, samplesPerBlock), 1 };
    convolution.prepare(processSpec);
    convolution.reset();

    const auto spec = dsp_utils::makeSpec(sampleRate, samplesPerBlock);
    dsp_utils::ensureCoef(hpf);
    dsp_utils::ensureCoef(bodyPeak);
    dsp_utils::ensureCoef(scoop);
    dsp_utils::ensureCoef(presence);
    dsp_utils::ensureCoef(lpf);
    hpf.prepare(spec);
    bodyPeak.prepare(spec);
    scoop.prepare(spec);
    presence.prepare(spec);
    lpf.prepare(spec);
    dsp_utils::assignCoef(hpf, juce::dsp::IIR::ArrayCoefficients<float>::makeHighPass((float) sampleRate, 85.0f, 0.7f));

    if (irFolder.getFullPathName().isEmpty())
        irFolder = defaultIrFolder();
    irFolder.createDirectory();

    lastBody = lastAir = -1.0f;
    updateFilters();
    reset();

    if (pendingIrName.isNotEmpty())
        loadImpulseFromFile(irFolder.getChildFile(pendingIrName));
}

void CabinetSim::reset()
{
    convolution.reset();
    hpf.reset();
    bodyPeak.reset();
    scoop.reset();
    presence.reset();
    lpf.reset();
    gainSm.setCurrentAndTargetValue(targetGain.load());
}

void CabinetSim::updateTargetGain() noexcept
{
    const float user = juce::Decibels::decibelsToGain(levelToDecibels(level.load()));
    const float makeup = usingIr.load() ? makeupGain.load() : 1.0f;
    targetGain.store(makeup * user);
}

void CabinetSim::updateFilters()
{
    const float bodyN = bodySm.getCurrentValue();
    const float airN = airSm.getCurrentValue();
    const float sr = (float) currentSampleRate;
    if (std::abs(bodyN - lastBody) > 1.0e-4f)
    {
        const float bodyGain = juce::Decibels::decibelsToGain(juce::jmap(bodyN, 0.0f, 1.0f, -3.0f, 4.0f));
        const float scoopGain = juce::Decibels::decibelsToGain(juce::jmap(bodyN, 0.0f, 1.0f, -1.0f, -4.0f));
        dsp_utils::assignCoef(bodyPeak, juce::dsp::IIR::ArrayCoefficients<float>::makePeakFilter(sr, 420.0f, 0.85f, bodyGain));
        dsp_utils::assignCoef(scoop, juce::dsp::IIR::ArrayCoefficients<float>::makePeakFilter(sr, 1600.0f, 0.8f, scoopGain));
        lastBody = bodyN;
    }
    if (std::abs(airN - lastAir) > 1.0e-4f)
    {
        const float presenceGain = juce::Decibels::decibelsToGain(juce::jmap(airN, 0.0f, 1.0f, -4.0f, 4.0f));
        const float lpHz = juce::jmap(airN, 0.0f, 1.0f, 4200.0f, 9000.0f);
        dsp_utils::assignCoef(presence, juce::dsp::IIR::ArrayCoefficients<float>::makePeakFilter(sr, 3200.0f, 0.9f, presenceGain));
        dsp_utils::assignCoef(lpf, juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass(sr, lpHz, 0.7f));
        lastAir = airN;
    }
}

void CabinetSim::processFallback(float* p, int n)
{
    for (int i = 0; i < n; ++i)
    {
        float s = p[i];
        s = hpf.processSample(s);
        s = bodyPeak.processSample(s);
        s = scoop.processSample(s);
        s = presence.processSample(s);
        s = lpf.processSample(s);
        p[i] = dsp_utils::softClip(s * 1.15f) * gainSm.getNextValue();
    }
}

void CabinetSim::process(juce::AudioBuffer<float>& buffer)
{
    if (! enabled.load())
        return;

    bodySm.setTargetValue(body.load());
    airSm.setTargetValue(air.load());
    updateTargetGain();
    gainSm.setTargetValue(targetGain.load());

    const int n = buffer.getNumSamples();
    auto* p = buffer.getWritePointer(0);

    if (usingIr.load())
    {
        juce::dsp::AudioBlock<float> block(buffer);
        auto mono = block.getSingleChannelBlock(0);
        juce::dsp::ProcessContextReplacing<float> ctx(mono);
        convolution.process(ctx);
        for (int i = 0; i < n; ++i)
        {
            bodySm.getNextValue();
            airSm.getNextValue();
            updateFilters();
            float s = p[i];
            s = bodyPeak.processSample(s);
            s = presence.processSample(s);
            p[i] = s * gainSm.getNextValue();
        }
        return;
    }

    for (int i = 0; i < n; ++i)
    {
        bodySm.getNextValue();
        airSm.getNextValue();
        updateFilters();
    }
    processFallback(p, n);
}

float CabinetSim::computeMakeupGain(const juce::AudioBuffer<float>& ir) const
{
    const int irN = juce::jmin(ir.getNumSamples(), 8192);
    if (irN < 16)
        return 1.0f;

    juce::HeapBlock<float> irMono((size_t) irN, true);
    const int irCh = juce::jmax(1, ir.getNumChannels());
    const float chScale = 1.0f / (float) irCh;
    for (int ch = 0; ch < irCh; ++ch)
    {
        const float* src = ir.getReadPointer(ch);
        for (int i = 0; i < irN; ++i)
            irMono[i] += src[i] * chScale;
    }

    float peak = 0.0f;
    for (int i = 0; i < irN; ++i)
        peak = juce::jmax(peak, std::abs(irMono[i]));
    if (peak > 1.0e-8f)
    {
        const float inv = 1.0f / peak;
        for (int i = 0; i < irN; ++i)
            irMono[i] *= inv;
    }

    const int testN = juce::jmax(4096, irN * 2);
    juce::HeapBlock<float> dry((size_t) testN, true);
    juce::HeapBlock<float> wet((size_t) testN, true);

    float b0 = 0, b1 = 0, b2 = 0, b3 = 0, b4 = 0, b5 = 0, b6 = 0;
    juce::Random rng(0xCABB1Eu);
    for (int i = 0; i < testN; ++i)
    {
        const float w = rng.nextFloat() * 2.0f - 1.0f;
        b0 = 0.99886f * b0 + w * 0.0555179f;
        b1 = 0.99332f * b1 + w * 0.0750759f;
        b2 = 0.96900f * b2 + w * 0.1538520f;
        b3 = 0.86650f * b3 + w * 0.3104856f;
        b4 = 0.55000f * b4 + w * 0.5329522f;
        b5 = -0.7616f * b5 - w * 0.0168980f;
        b6 = w * 0.115926f;
        dry[i] = (b0 + b1 + b2 + b3 + b4 + b5 + b6 + w * 0.5362f) * 0.11f;
    }

    for (int i = 0; i < testN; ++i)
    {
        const int kMax = juce::jmin(irN, i + 1);
        float acc = 0.0f;
        for (int k = 0; k < kMax; ++k)
            acc += dry[i - k] * irMono[k];
        wet[i] = acc;
    }

    const int start = juce::jmin(irN / 4, testN / 4);
    double dryE = 0.0, wetE = 0.0;
    const int count = testN - start;
    for (int i = start; i < testN; ++i)
    {
        dryE += (double) dry[i] * (double) dry[i];
        wetE += (double) wet[i] * (double) wet[i];
    }
    dryE = std::sqrt(dryE / (double) juce::jmax(1, count));
    wetE = std::sqrt(wetE / (double) juce::jmax(1, count));
    if (wetE < 1.0e-8)
        return 1.0f;

    return juce::jlimit(1.0f, juce::Decibels::decibelsToGain(24.0f), (float) (dryE / wetE));
}

bool CabinetSim::loadImpulseFromFile(const juce::File& file)
{
    if (! file.existsAsFile())
    {
        clearImpulse();
        return false;
    }

    juce::AudioFormatManager formats;
    formats.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(file));
    if (reader == nullptr || reader->lengthInSamples < 16)
    {
        clearImpulse();
        return false;
    }

    juce::AudioBuffer<float> ir((int) juce::jmax((juce::int64) 1, (juce::int64) reader->numChannels),
                                (int) juce::jmin(reader->lengthInSamples, (juce::int64) 1 << 20));
    reader->read(&ir, 0, ir.getNumSamples(), 0, true, true);

    const float peak = ir.getMagnitude(0, ir.getNumSamples());
    if (peak > 1.0e-8f)
        ir.applyGain(1.0f / peak);

    makeupGain.store(computeMakeupGain(ir));
    convolution.loadImpulseResponse(std::move(ir),
                                    reader->sampleRate,
                                    juce::dsp::Convolution::Stereo::no,
                                    juce::dsp::Convolution::Trim::no,
                                    juce::dsp::Convolution::Normalise::no);

    const auto folder = getIrFolder();
    auto relative = file.getRelativePathFrom(folder);
    loadedIrName = relative.isNotEmpty() ? relative : file.getFileName();
    pendingIrName = loadedIrName;
    usingIr.store(true);
    updateTargetGain();
    return true;
}

void CabinetSim::clearImpulse()
{
    usingIr.store(false);
    loadedIrName.clear();
    pendingIrName.clear();
    makeupGain.store(1.0f);
    updateTargetGain();
}

void CabinetSim::setIrFolder(const juce::File& folder)
{
    irFolder = folder;
    irFolder.createDirectory();
}

juce::File CabinetSim::getIrFolder() const
{
    return irFolder.getFullPathName().isEmpty() ? defaultIrFolder() : irFolder;
}

juce::StringArray CabinetSim::listImpulseFiles() const
{
    juce::StringArray names;
    const auto folder = getIrFolder();
    auto files = folder.findChildFiles(juce::File::findFiles, true, "*.wav");
    files.sort();
    for (const auto& f : files)
        names.add(f.getRelativePathFrom(folder));
    names.sort(true);
    return names;
}

void CabinetSim::setBody(float zeroToOne)
{
    body.store(juce::jlimit(0.0f, 1.0f, zeroToOne));
}

void CabinetSim::setAir(float zeroToOne)
{
    air.store(juce::jlimit(0.0f, 1.0f, zeroToOne));
}

void CabinetSim::setLevel(float zeroToOne)
{
    level.store(juce::jlimit(0.0f, 1.0f, zeroToOne));
    updateTargetGain();
}

void CabinetSim::restoreDefaults()
{
    enabled.store(true);
    body.store(0.55f);
    air.store(0.45f);
    level.store(0.5f);
    updateTargetGain();
}

juce::var CabinetSim::toJson() const
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty("enabled", isEnabled());
    obj->setProperty("body", body.load());
    obj->setProperty("air", air.load());
    obj->setProperty("level", level.load());
    obj->setProperty("irName", loadedIrName);
    obj->setProperty("irFolder", getIrFolder().getFullPathName());
    return juce::var(obj);
}

void CabinetSim::fromJson(const juce::var& json)
{
    if (auto* obj = json.getDynamicObject())
    {
        if (obj->hasProperty("enabled"))
            setEnabled((bool) obj->getProperty("enabled"));
        if (obj->hasProperty("body"))
            setBody((float) (double) obj->getProperty("body"));
        if (obj->hasProperty("air"))
            setAir((float) (double) obj->getProperty("air"));
        if (obj->hasProperty("level"))
            setLevel((float) (double) obj->getProperty("level"));
        if (obj->hasProperty("irFolder"))
            setIrFolder(juce::File(obj->getProperty("irFolder").toString()));
        pendingIrName = obj->getProperty("irName").toString();
        if (pendingIrName.isNotEmpty())
            loadImpulseFromFile(getIrFolder().getChildFile(pendingIrName));
        else
            clearImpulse();
    }
}
