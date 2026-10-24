#pragma once
#include "Parameters.h"
namespace aurum {
struct MixGains {double wet,dry;};
class ParallelMixer {
public:
 static MixGains targets(const Parameters& p) noexcept {
  return {p.wetMute?0.0:(p.linked?p.blend:gain(p.wetDb)),p.dryMute?0.0:(p.linked?1-p.blend:gain(p.dryDb))};
 }
 static double combine(double wet,double dry,MixGains g,double delta) noexcept {
  const double normal=wet*g.wet+dry*g.dry;
  return normal+(wet-dry-normal)*delta;
 }
};
}
