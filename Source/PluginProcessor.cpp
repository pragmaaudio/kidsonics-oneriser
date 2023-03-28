#include "PluginProcessor.h"
#include "PluginEditor.h"

OneRiserProcessor::OneRiserProcessor()
 : AudioProcessor(
    BusesProperties()
     #if ! JucePlugin_IsMidiEffect
      #if ! JucePlugin_IsSynth
       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
      #endif
       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
     #endif
 ),
 // the parameters object is passed its arguments here
 parameters(*this, nullptr, "Parameters", createParameters()) {}

OneRiserProcessor::~OneRiserProcessor() = default;

const juce::String OneRiserProcessor::getName() const {
    return JucePlugin_Name;
}

bool OneRiserProcessor::acceptsMidi() const {
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool OneRiserProcessor::producesMidi() const {
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool OneRiserProcessor::isMidiEffect() const {
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double OneRiserProcessor::getTailLengthSeconds() const {
    return 0.0;
}

int OneRiserProcessor::getNumPrograms() {
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int OneRiserProcessor::getCurrentProgram() {
    return 0;
}

void OneRiserProcessor::setCurrentProgram(int index) {
    juce::ignoreUnused (index);
}

const juce::String OneRiserProcessor::getProgramName(int index) {
    juce::ignoreUnused (index);
    return {};
}

void OneRiserProcessor::changeProgramName(int index, const juce::String& newName) {
    juce::ignoreUnused (index, newName);
}

void OneRiserProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    juce::ignoreUnused(samplesPerBlock);

    riserProcessor.prepare(uint(sampleRate));
}

void OneRiserProcessor::releaseResources() {
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool OneRiserProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}

void OneRiserProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                     juce::MidiBuffer& midiMessages) {
    juce::ignoreUnused(midiMessages);

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // clears unused output channels if there are less input channels (avoids garbage data)
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    //              //             //               //              //

    float* leftData = buffer.getWritePointer(0),
         * rightData = buffer.getWritePointer(1);

    // for (int i = 0; i < buffer.getNumSamples(); i++) {
    //     leftData[i] = 0;
    //     rightData[i] = 0;
    // }

    riserProcessor.process(leftData, rightData, buffer.getNumSamples());
}

bool OneRiserProcessor::hasEditor() const {
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* OneRiserProcessor::createEditor() {
    return new OneRiserEditor(*this);
}

void OneRiserProcessor::getStateInformation(juce::MemoryBlock& destData) {
    // this allows the host to save the state of the device
    auto state = parameters.copyState();
    std::unique_ptr<XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void OneRiserProcessor::setStateInformation(const void* data, int sizeInBytes) {
    // this allows the host to load the state of the device
    std::unique_ptr<XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName(parameters.state.getType()))
        parameters.replaceState(ValueTree::fromXml(*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new OneRiserProcessor();
}

AudioProcessorValueTreeState::ParameterLayout OneRiserProcessor::createParameters() {
    vector<std::unique_ptr<RangedAudioParameter>> params;

    // range and resolution of the parameters, which are all the same in this case
    NormalisableRange<float> normRange(0.0f, 1.0f, 0.0001f);

    // the "AudioParameterFloat" objects
    // "ParameterID { "...", 1 }" is used to provide a version hint value (in this case 1) to each parameter
    params.push_back(std::make_unique<AudioParameterFloat>(ParameterID { "MAS_AMT", 1 }, "Master Amount",  normRange, 0.00f));
    params.push_back(std::make_unique<AudioParameterFloat>(ParameterID { "FLG_AMT", 1 }, "Flanger Amount", normRange, 0.65f));
    params.push_back(std::make_unique<AudioParameterFloat>(ParameterID { "FIL_AMT", 1 }, "Filter Amount",  normRange, 1.00f));
    params.push_back(std::make_unique<AudioParameterFloat>(ParameterID { "REV_AMT", 1 }, "Reverb Amount",  normRange, 0.70f));

    return { params.begin(), params.end() };
}