#include "ProcessingEngine.h"
namespace aurum {
bool ProcessingEngine::prepare(double sampleRate,unsigned numChannels,Parameters p) noexcept {
 ready=std::isfinite(sampleRate)&&sampleRate>=8000&&sampleRate<=384000&&(numChannels==1||numChannels==2);
 if(!ready){channels=0;return false;}channels=numChannels;p=validated(p);auto mix=ParallelMixer::targets(p);
 input.prepare(sampleRate,gain(p.inputDb));output.prepare(sampleRate,gain(p.outputDb));wet.prepare(sampleRate,mix.wet);dry.prepare(sampleRate,mix.dry);delta.prepare(sampleRate,p.delta?1:0);bypass.prepare(sampleRate,p.bypass?1:0);return true;
}
void ProcessingEngine::reset(Parameters p) noexcept {p=validated(p);auto mix=ParallelMixer::targets(p);input.reset(gain(p.inputDb));output.reset(gain(p.outputDb));wet.reset(mix.wet);dry.reset(mix.dry);delta.reset(p.delta?1:0);bypass.reset(p.bypass?1:0);}
}
