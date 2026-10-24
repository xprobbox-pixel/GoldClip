#pragma once
#include <array>
#include <atomic>
#include <algorithm>
#include <cstdint>
#include <cmath>
namespace aurum {
static_assert(std::atomic<unsigned>::is_always_lock_free&&std::atomic<float>::is_always_lock_free);
struct Envelope {float inputMin=0,inputMax=0,outputMin=0,outputMax=0;};
// Single audio producer, single editor consumer. Overflow drops graphics, never audio.
class Telemetry {
 static constexpr unsigned capacity=1024;
 std::array<Envelope,capacity> frames{};
 alignas(64) std::atomic<unsigned> write{0};
 alignas(64) std::atomic<unsigned> read{0};
 std::array<std::atomic<float>,2> input{},output{};
 std::atomic<unsigned> dropped{0};
 std::atomic<bool> overload{false};
 Envelope pending{};unsigned count=0;
 static void max(std::atomic<float>& dest,float value) noexcept {auto old=dest.load(std::memory_order_relaxed);while(old<value&&!dest.compare_exchange_weak(old,value,std::memory_order_relaxed)){} }
public:
 void push(const double* in,const double* out,unsigned channels) noexcept {
  for(unsigned c=0;c<channels;++c){
   const auto a=float(std::clamp(in[c],-1.0e6,1.0e6)),b=float(std::clamp(out[c],-1.0e6,1.0e6));
   if(count==0&&c==0)pending={a,a,b,b};
   max(input[c],std::abs(a));max(output[c],std::abs(b));if(std::abs(b)>=1)overload.store(true,std::memory_order_relaxed);
   pending.inputMin=std::min(pending.inputMin,a);pending.inputMax=std::max(pending.inputMax,a);
   pending.outputMin=std::min(pending.outputMin,b);pending.outputMax=std::max(pending.outputMax,b);
  }
  if(++count<64)return;
  const auto w=write.load(std::memory_order_relaxed);
  if(w-read.load(std::memory_order_acquire)<capacity){frames[w&(capacity-1)]=pending;write.store(w+1,std::memory_order_release);}else dropped.fetch_add(1,std::memory_order_relaxed);
  pending={};count=0;
 }
 bool pop(Envelope& frame) noexcept {const auto r=read.load(std::memory_order_relaxed);if(r==write.load(std::memory_order_acquire))return false;frame=frames[r&(capacity-1)];read.store(r+1,std::memory_order_release);return true;}
 struct Peaks {std::array<float,2> input,output;bool overloaded;unsigned droppedFrames;};
 Peaks consumePeaks() noexcept {return {{input[0].exchange(0),input[1].exchange(0)},{output[0].exchange(0),output[1].exchange(0)},overload.exchange(false),dropped.load()};}
};
}
