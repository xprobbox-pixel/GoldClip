#include "PluginEditor.h"
namespace aurum {
using namespace juce;
namespace {
const Colour gold(0xfff0ca72),muted(0xffb6b8bf),blue(0xff4b91e8);
void text(Graphics& g,const String& s,Rectangle<float> r,float size,Colour colour=muted,int align=Justification::centredLeft){g.setColour(colour);g.setFont(Font(FontOptions(size,Font::bold)));g.drawText(s,r,align);}
String db(float x){return x>0?String(20*std::log10(x),1):String(CharPointer_UTF8("\xe2\x88\x92\xe2\x88\x9e"));}
void panel(Graphics& g,Rectangle<float> r){g.setColour(Colour(0xff101114));g.fillRoundedRectangle(r,12);g.setColour(Colour(0xff45464f));g.drawRoundedRectangle(r,12,1);}
}
CustomLookAndFeel::CustomLookAndFeel(){setColour(Slider::textBoxTextColourId,gold);setColour(Slider::textBoxOutlineColourId,Colours::transparentBlack);setColour(ComboBox::backgroundColourId,Colour(0xff222329));setColour(ComboBox::textColourId,gold);setColour(PopupMenu::backgroundColourId,Colour(0xff222329));setColour(PopupMenu::textColourId,gold);}
void CustomLookAndFeel::drawRotarySlider(Graphics& g,int x,int y,int w,int h,float pos,float start,float end,Slider& s){auto r=Rectangle<float>(float(x),float(y),float(w),float(h)).reduced(8);auto centre=r.getCentre();const float radius=jmin(r.getWidth(),r.getHeight())*.5f;
 g.setGradientFill(ColourGradient(Colour(0xff5b5d67),centre.x,centre.y-radius,Colour(0xff17181c),centre.x,centre.y+radius,false));g.fillEllipse(centre.x-radius,centre.y-radius,2*radius,2*radius);g.setColour(Colour(0xff777980));g.drawEllipse(centre.x-radius,centre.y-radius,2*radius,2*radius,1);
 Path arc;arc.addCentredArc(centre.x,centre.y,radius-3,radius-3,0,start,start+pos*(end-start),true);g.setColour(s.isEnabled()?gold:Colour(0xff676971));g.strokePath(arc,PathStrokeType(3));auto endPoint=centre+Point<float>(std::sin(start+pos*(end-start)),-std::cos(start+pos*(end-start)))*(radius*.68f);g.setColour(Colours::white);g.drawLine({centre,endPoint},2);
}
void CustomLookAndFeel::drawToggleButton(Graphics& g,ToggleButton& b,bool over,bool){auto r=b.getLocalBounds().toFloat().reduced(1);g.setColour(Colour(over?0xff35363d:0xff222329));g.fillRoundedRectangle(r,6);g.setColour(b.getToggleState()?gold:Colour(0xff61636d));g.drawRoundedRectangle(r,6,1);text(g,b.getButtonText(),r,12,b.getToggleState()?gold:muted,Justification::centred);}
void CustomLookAndFeel::drawCornerResizer(Graphics& g,int w,int h,bool,bool){g.setColour(gold);for(int i=4;i<17;i+=5)g.drawLine(float(w-i),float(h-2),float(w-2),float(h-i),2);}
PluginEditor::PluginEditor(PluginProcessor& p):AudioProcessorEditor(p),processor(p),baseWidth(p.isCompact()?520:1200){
 const int saved=jlimit(int(baseWidth*.8),int(baseWidth*1.75),p.editorWidth.load());setLookAndFeel(&look);canvas.setInterceptsMouseClicks(false,true);addAndMakeVisible(canvas);
 const char* ids[]{"input","output","blend","wet","dry"};const char* names[]{"INPUT TRIM","OUTPUT TRIM","WET BLEND","WET LEVEL","DRY LEVEL"};const double defaults[]{0,0,100,0,-60};
 const char* descriptions[]{"Wet path input gain; the original dry path stays independent. Double-click resets to 0 dB.","Final combined output gain, including Delta. Can exceed 0 dBFS; there is no limiter.","Linked linear blend: wet plus dry gains sum to one for identical paths.","Unlinked processed-path gain in dB; inactive while Link Wet/Dry is on.","Unlinked original-path gain in dB; inactive while Link Wet/Dry is on."};
 for(size_t i=0;i<sliders.size();++i){auto& s=sliders[i];s.setName(names[i]);s.setTooltip(descriptions[i]);s.setSliderStyle(Slider::RotaryVerticalDrag);s.setTextBoxStyle(Slider::TextBoxBelow,false,90,24);s.setTextValueSuffix(i==2?" %":" dB");s.setNumDecimalPlacesToDisplay(1);s.setDoubleClickReturnValue(true,defaults[i]);canvas.addAndMakeVisible(s);labels[i].setText(names[i],dontSendNotification);labels[i].setJustificationType(Justification::centred);labels[i].setFont(Font(FontOptions(12,Font::bold)));canvas.addAndMakeVisible(labels[i]);sliderLinks[i]=std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(p.parameters.state,ids[i],s);}
 const char* boolIds[]{"linked","wetMute","dryMute","delta","bypass"};const char* boolNames[]{"LINK WET/DRY","MUTE WET","MUTE DRY","DELTA","BYPASS"};
 for(size_t i=0;i<toggles.size();++i){auto& b=toggles[i];b.setButtonText(boolNames[i]);b.setName(boolNames[i]);b.setTooltip(i==0?"Select constant-amplitude linked blend or independent dB gains.":i==3?"Output the aligned wet-minus-original difference before final Output Trim; ignores normal mixer levels/mutes. This affects renders.":i==4?"Crossfade to unity dry pass-through over 20 ms. Host bypass can provide immediate dry pass-through.":"Mute this path in the normal mixer. Delta monitoring remains independent.");canvas.addAndMakeVisible(b);toggleLinks[i]=std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(p.parameters.state,boolIds[i],b);}
 scale.setName("Window scale");scale.setTooltip("Resize the complete interface. 80% to 175%; drag the gold corner for intermediate sizes.");for(int percent:{80,100,125,150,175})scale.addItem(String(percent)+"%",percent);scale.onChange=[this]{if(auto percent=scale.getSelectedId();percent>0)setSize(roundToInt(baseWidth*percent/100.0),roundToInt(baseHeight*percent/100.0));};canvas.addAndMakeVisible(scale);
 reset.onClick=[this]{inputHold=outputHold=0;overloaded=false;};reset.setTooltip("Reset displayed peak holds and overload indication. Audio is unaffected.");canvas.addAndMakeVisible(reset);
 setResizable(true,true);setResizeLimits(int(baseWidth*.8),480,int(baseWidth*1.75),1050);getConstrainer()->setFixedAspectRatio(double(baseWidth)/baseHeight);setSize(saved,roundToInt(double(saved)*baseHeight/baseWidth));timerCallback();startTimerHz(30);
}
PluginEditor::~PluginEditor(){stopTimer();setLookAndFeel(nullptr);}
void PluginEditor::resized(){canvas.setBounds(0,0,baseWidth,baseHeight);canvas.setTransform(AffineTransform::scale(getWidth()/float(baseWidth),getHeight()/float(baseHeight)));processor.editorWidth=getWidth();
 const bool compact=processor.isCompact();
 for(size_t i=0;i<sliders.size();++i){const int x=compact?15+int(i)*100:180+int(i)*174;sliders[i].setBounds(x,compact?375:393,compact?90:130,100);labels[i].setBounds(x,compact?354:370,compact?90:130,22);}
 for(size_t i=0;i<toggles.size();++i)toggles[i].setBounds(15+int(i)*(compact?99:168),compact?500:510,compact?94:156,32);
 scale.setBounds(baseWidth-115,18,95,28);reset.setBounds(20,553,130,27);
 const double percent=100.0*getWidth()/baseWidth;bool preset=false;for(int v:{80,100,125,150,175})if(getWidth()==roundToInt(baseWidth*v/100.0)){scale.setSelectedId(v,dontSendNotification);preset=true;break;}if(!preset)scale.setText(String(percent,1)+"%",dontSendNotification);
}
void PluginEditor::timerCallback(){Envelope frame;unsigned drained=0;while(drained++<1024&&processor.telemetry.pop(frame)){history[head]=frame;head=(head+1)%unsigned(history.size());}peaks=processor.telemetry.consumePeaks();inputHold=jmax(inputHold,peaks.input[0],peaks.input[1]);outputHold=jmax(outputHold,peaks.output[0],peaks.output[1]);overloaded=overloaded||peaks.overloaded;const bool linked=processor.parameters.snapshot().linked;sliders[2].setEnabled(linked);sliders[3].setEnabled(!linked);sliders[4].setEnabled(!linked);repaint();}
void PluginEditor::drawWaveform(Graphics& g,Rectangle<float> r){panel(g,r);g.saveState();g.reduceClipRegion(r.reduced(2).toNearestInt());g.setColour(Colour(0xff252932));for(int row=1;row<4;++row)g.drawHorizontalLine(roundToInt(r.getY()+r.getHeight()*row/4),r.getX(),r.getRight());
 for(unsigned i=0;i<history.size();++i){const auto& v=history[(head+i)%unsigned(history.size())];const float x=r.getX()+r.getWidth()*i/float(history.size());auto y=[&](float a){return r.getCentreY()-jlimit(-1.2f,1.2f,a)*r.getHeight()*.4f;};g.setColour(Colour(0xff686b74));g.drawVerticalLine(roundToInt(x),y(v.inputMax),y(v.inputMin));g.setColour(gold);g.drawVerticalLine(roundToInt(x),y(v.outputMax),y(v.outputMin));}g.restoreState();
}
void PluginEditor::paint(Graphics& original){Graphics::ScopedSaveState state(original);original.addTransform(AffineTransform::scale(getWidth()/float(baseWidth),getHeight()/float(baseHeight)));auto& g=original;
 g.fillAll(Colour(0xff17181c));g.setColour(Colour(0xff45464f));g.drawRoundedRectangle(Rectangle<float>(2,2,float(baseWidth-4),596),14,2);g.setColour(gold);g.fillRect(20,64,baseWidth-40,2);
 text(g,"AURUM CLIP",{20,15,230,32},25,gold);text(g,processor.isCompact()?"TRACK / PHASE 1":"MASTER / PHASE 1",{processor.isCompact()?280.f:310.f,processor.isCompact()?78.f:18.f,220,28},14);text(g,"ROUTING FOUNDATION",{20,78,float(baseWidth-40),22},16);
 drawWaveform(g,{20,116,float(baseWidth-40),processor.isCompact()?150.f:190.f});text(g,"ORIGINAL INPUT / PROCESSED OUTPUT",{32,120,400,20},11);const float y=processor.isCompact()?280.f:320.f;
 text(g,"INPUT HOLD  "+db(inputHold)+" dBFS",{20,y,240,26},14);text(g,"OUTPUT HOLD  "+db(outputHold)+" dBFS",{processor.isCompact()?268.f:700.f,y,processor.isCompact()?232.f:350.f,26},processor.isCompact()?12.f:14.f,overloaded?Colour(0xfff47a70):gold);
 if(overloaded)text(g,"OUTPUT OVERLOAD",{20,y+25,300,22},12,Colour(0xfff47a70));
 text(g,"No clipping / Gold / Alchemy / oversampling in Phase 1",{processor.isCompact()?20.f:450.f,processor.isCompact()?326.f:552.f,float(processor.isCompact()?480:700),22},processor.isCompact()?11.f:13.f);
 text(g,String(JucePlugin_VersionString)+"  |  ZERO LATENCY  |  MONO / STEREO",{165,556,float(baseWidth-180),20},processor.isCompact()?10.f:12.f);text(g,"Waveform: fixed 25,600-sample history; peak-preserving 64-sample envelopes",{20,582,float(baseWidth-40),14},processor.isCompact()?8.f:11.f,blue);
}
}
