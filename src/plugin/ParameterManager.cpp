#include "ParameterManager.h"
#include <sstream>
#include <locale>
namespace aurum {
namespace {constexpr const char* ids[]{"input","output","blend","wet","dry","linked","wetMute","dryMute","delta","bypass"};}
juce::AudioProcessorValueTreeState::ParameterLayout ParameterManager::layout(){
 juce::AudioProcessorValueTreeState::ParameterLayout p;
 auto number=[&](const char* id,const char* name,float lo,float hi,float def,const char* unit){p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id,1},name,juce::NormalisableRange<float>{lo,hi,.01f},def,juce::AudioParameterFloatAttributes().withLabel(unit).withStringFromValueFunction([](float x,int){return juce::String(x,1);})) );};
 number("input","Input Trim",-12,12,0,"dB");number("output","Output Trim",-12,12,0,"dB");number("blend","Linked Wet Blend",0,100,100,"%");number("wet","Independent Wet Level",-60,6,0,"dB");number("dry","Independent Dry Level",-60,6,-60,"dB");
 for(auto item:{std::pair{"linked","Link Wet/Dry"},std::pair{"wetMute","Mute Wet"},std::pair{"dryMute","Mute Dry"},std::pair{"delta","Delta Monitor"},std::pair{"bypass","Bypass"}})p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{item.first,1},item.second,juce::String(item.first)=="linked"));return p;
}
ParameterManager::ParameterManager(juce::AudioProcessor& owner):state(owner,nullptr,"AurumState",layout()){for(size_t i=0;i<values.size();++i)values[i]=state.getRawParameterValue(ids[i]);}
Parameters ParameterManager::snapshot() const noexcept {return validated({values[0]->load(),values[1]->load(),values[2]->load()/100.0,values[3]->load(),values[4]->load(),values[5]->load()>.5f,values[6]->load()>.5f,values[7]->load()>.5f,values[8]->load()>.5f,values[9]->load()>.5f});}
void ParameterManager::save(juce::MemoryBlock& bytes,bool compact,int width){auto tree=state.copyState();tree.setProperty("schema",1,nullptr);tree.setProperty("variant",compact?"track":"master",nullptr);tree.setProperty("editorWidth",width,nullptr);auto xml=tree.createXml();juce::AudioProcessor::copyXmlToBinary(*xml,bytes);}
bool ParameterManager::restore(const void* bytes,int size,bool compact,int& width){
 if(size<=0||size>1024*1024)return false;
 auto xml=juce::AudioProcessor::getXmlFromBinary(bytes,size);if(!xml||!xml->hasTagName("AurumState"))return false;
 auto tree=juce::ValueTree::fromXml(*xml);if(int(tree.getProperty("schema",0))!=1||tree.getProperty("variant").toString()!=(compact?"track":"master"))return false;
 // Validate the complete transaction before touching any live parameter.
 std::array<float,10> restored{};std::array<bool,10> found{};
 for(auto child:tree){const auto id=child.getProperty("id").toString();for(size_t i=0;i<values.size();++i)if(id==ids[i]){
  if(found[i]||!child.hasProperty("value"))return false;
  const auto value=child.getProperty("value");const auto text=value.toString().toStdString();float v=0;std::istringstream parser(text);parser.imbue(std::locale::classic());parser>>std::noskipws>>v;
  if(!parser||!parser.eof()||!std::isfinite(v))return false;
  auto range=state.getParameter(ids[i])->getNormalisableRange();restored[i]=juce::jlimit(range.start,range.end,v);found[i]=true;
 }}
 for(bool valid:found)if(!valid)return false;
 for(size_t i=0;i<values.size();++i){auto* p=state.getParameter(ids[i]);p->setValueNotifyingHost(p->convertTo0to1(restored[i]));}
 const int base=compact?520:1200;width=juce::jlimit(int(base*.8),int(base*1.75),int(tree.getProperty("editorWidth",base)));return true;
}
}
