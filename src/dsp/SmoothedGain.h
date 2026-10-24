#pragma once
#include <algorithm>
#include <cmath>
namespace aurum {
class SmoothedGain {
 double current=0,target=0,increment=0;int remaining=0,length=960;
public:
 void prepare(double sampleRate,double initial) noexcept {length=std::max(1,int(std::round(sampleRate*.02)));reset(initial);}
 void reset(double value) noexcept {current=target=value;increment=0;remaining=0;}
 void setTarget(double value) noexcept {if(value==target)return;target=value;remaining=length;increment=(target-current)/remaining;}
 double next() noexcept {if(remaining>0){current+=increment;if(--remaining==0)current=target;}return current;}
};
}
