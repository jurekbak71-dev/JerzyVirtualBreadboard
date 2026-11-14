#include "PluginEditor.h"
#if MF_GUI_ASSETS
#include <BinaryData.h>
#endif
#include <cmath>

namespace
{
juce::Image artwork (const juce::String& file)
{
#if MF_GUI_ASSETS
    int size = 0;
    if (auto* bytes = BinaryData::getNamedResource (file.toRawUTF8(), size))
        return juce::ImageFileFormat::loadFrom(bytes, size_t(size));
#else
    juce::ignoreUnused(file);
#endif
    return {};
}

class RackChoice final : public juce::ComboBox
{
public:
    void mouseDown(const juce::MouseEvent& event) override
    {
        const int count=getNumItems();
        if(count>0) {const int item=juce::jlimit(0,count-1,int(event.position.x*count/juce::jmax(1,getWidth())));if(isItemEnabled(getItemId(item)))setSelectedItemIndex(item,juce::sendNotificationSync);}
    }
};
class RackStyle final : public juce::LookAndFeel_V4
{
public:
    RackStyle()
    {
        const std::array<const char*, 7> names { "steel_large", "steel_warm", "black_anodized", "steel_bright", "chrome_small", "black_small", "brass_small" };
        for (size_t i = 0; i < names.size(); ++i)
        {
            bodies[i] = artwork(juce::String(names[i]) + "_body_png");
            indexes[i] = artwork(juce::String(names[i]) + "_index_128_png");
        }
        buttonOff=artwork("control_button_off_png");buttonOn=artwork("control_button_on_png");
        setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffeee9dc));
        setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff131515));
        setColour(juce::ComboBox::textColourId, juce::Colour(0xffeee9dc));
        setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff817d71));
    }
    void drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h,
                          float value, float start, float end, juce::Slider& slider) override
    {
        const auto i = size_t(juce::jlimit(0, 6, int(slider.getProperties()["knobStyle"])));
        const auto& body = bodies[i]; const auto& index = indexes[i];
        if (body.isValid() && index.getWidth() == 1536 && index.getHeight() == 768)
        {
            auto dest = juce::Rectangle<float>(float(x),float(y),float(w),float(h));
            auto side = juce::jmin(dest.getWidth(),dest.getHeight());
            dest = juce::Rectangle<float>(side,side).withCentre(dest.getCentre());
            g.drawImage(body,dest);
            const int frame = juce::jlimit(0,127,juce::roundToInt(juce::jlimit(0.0f,1.0f,value)*127));
            g.drawImage(index,int(dest.getX()),int(dest.getY()),int(dest.getWidth()),int(dest.getHeight()),(frame%16)*96,(frame/16)*96,96,96);
        }
        else LookAndFeel_V4::drawRotarySlider(g,x,y,w,h,value,start,end,slider);
        // Independent vector marker survives raster body, index and reflections.
        const float radius=0.5f*float(juce::jmin(w,h));
        const float angle=start+juce::jlimit(0.0f,1.0f,value)*(end-start);
        const auto cx=float(x)+0.5f*float(w),cy=float(y)+0.5f*float(h);
        const auto ux=std::sin(angle),uy=-std::cos(angle);
        juce::Point<float> inner(cx+radius*.37f*ux,cy+radius*.37f*uy);
        juce::Point<float> outer(cx+radius*.74f*ux,cy+radius*.74f*uy);
        g.setColour(juce::Colour(0xff220908));
        g.drawLine(juce::Line<float>(inner,outer),juce::jmax(2.5f,radius*.16f));
        g.setColour(juce::Colour(0xffeb2925));
        g.drawLine(juce::Line<float>(inner,outer),juce::jmax(1.5f,radius*.09f));
        g.fillEllipse(outer.x-radius*.055f,outer.y-radius*.055f,radius*.11f,radius*.11f);
    }
    void positionComboBoxText(juce::ComboBox&,juce::Label& label) override {label.setBounds(0,0,0,0);}
    void drawComboBox(juce::Graphics& g,int width,int height,bool,int,int,int,int,juce::ComboBox& box) override
    {
        const int count=juce::jmax(1,box.getNumItems());const float cell=float(width)/count;
        for(int i=0;i<count;++i)
        {
            auto r=juce::Rectangle<float>(i*cell,0,cell-2,float(height));
            const bool selected=box.getSelectedItemIndex()==i;
            if(box.getComponentID()=="type")
            {
                g.setColour(juce::Colour(0xff111514));g.fillRoundedRectangle(r.withHeight(height*.47f),3);
                g.setColour(selected?juce::Colour(0xffffa11a):juce::Colour(0xff44443a));g.fillEllipse(r.getCentreX()-3,4,6,6);
                r=r.withTrimmedTop(height*.46f);
            }
            else if(buttonOff.isValid())g.drawImage(selected&&buttonOn.isValid()?buttonOn:buttonOff,r);
            else {g.setColour(juce::Colour(selected?0xff603109:0xff151719));g.fillRoundedRectangle(r,3);}
            g.setOpacity(box.isItemEnabled(box.getItemId(i))?1.0f:.35f);g.setColour(selected?juce::Colour(0xffffd795):juce::Colour(0xffeee9dc));g.setFont(juce::FontOptions(juce::jlimit(9.0f,15.0f,height*.42f)));
            auto text=box.getItemText(i);if(box.getComponentID()=="roomMode")text=i==0?"USER":"FACTORY";
            g.drawFittedText(text,r.toNearestInt().reduced(1),juce::Justification::centred,1);g.setOpacity(1);
        }
        if(box.hasKeyboardFocus(true)){g.setColour(juce::Colours::orange);g.drawRect(box.getLocalBounds(),1);}
    }
    void drawButtonText(juce::Graphics& g,juce::TextButton& button,bool,bool) override
    {
        const auto text=button.getButtonText();const auto center=button.getLocalBounds().toFloat().getCentre();const float x=center.x,y=center.y;
        g.setColour(button.isEnabled()?juce::Colour(0xffeee9dc):juce::Colour(0xff66685f));
        if(text=="Save") {g.drawRect(x-7,y-8,14.0f,16.0f,1.5f);g.drawRect(x-4,y-7,7.0f,5.0f,1.0f);g.fillRect(x-4,y+1,8.0f,5.0f);}
        else if(text=="Copy") {g.drawRect(x-7,y-7,10.0f,11.0f,1.5f);g.drawRect(x-3,y-3,10.0f,11.0f,1.5f);}
        else if(text=="Undo"||text=="Redo")
        {
            juce::Path path;path.startNewSubPath(-6,0);path.cubicTo(-2,-7,7,-5,7,2);path.cubicTo(7,6,3,8,0,7);
            path.applyTransform(juce::AffineTransform::scale(text=="Redo"?-1.0f:1.0f,1).translated(x,y));g.strokePath(path,juce::PathStrokeType(1.7f));
            juce::Path arrow;arrow.addTriangle(-8,-4,-8,3,-1,2);arrow.applyTransform(juce::AffineTransform::scale(text=="Redo"?-1.0f:1.0f,1).translated(x,y));g.fillPath(arrow);
        }
        else {g.setFont(juce::FontOptions(juce::jlimit(10.0f,15.0f,button.getHeight()*.4f)));g.drawFittedText(text,button.getLocalBounds().reduced(2),juce::Justification::centred,1);}
    }
    void drawButtonBackground(juce::Graphics& g,juce::Button& button,const juce::Colour&,bool over,bool down) override
    {
        const auto bounds=button.getLocalBounds().toFloat();
        if(buttonOff.isValid())g.drawImage((button.getToggleState()||down)&&buttonOn.isValid()?buttonOn:buttonOff,bounds);
        else {g.setColour(juce::Colour(0xff202325));g.fillRoundedRectangle(bounds,3);}
        if(over||button.hasKeyboardFocus(true)){g.setColour(juce::Colour(0xffb58a47));g.drawRoundedRectangle(bounds.reduced(1),3,1);}
    }
    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button, bool over, bool down) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
        if(buttonOff.isValid() && buttonOn.isValid())
        {
            g.drawImage(button.getToggleState()?buttonOn:buttonOff,bounds);
            g.setColour(button.getToggleState()?juce::Colour(0xffffce76):juce::Colour(0xffeee9dc));
            g.setFont(juce::FontOptions(juce::jlimit(9.0f,17.0f,bounds.getHeight()*.4f)));
            g.drawFittedText(button.getButtonText(),button.getLocalBounds().reduced(3),juce::Justification::centred,1);
            if(button.hasKeyboardFocus(true)){g.setColour(juce::Colours::orange);g.drawRoundedRectangle(bounds,3,1);}
            return;
        }
        g.setColour(juce::Colour(0xff77736a));g.fillRoundedRectangle(bounds,3);
        g.setColour(button.getToggleState() ? juce::Colour(0xff603109) : juce::Colour(0xff151719));
        g.fillRoundedRectangle(bounds.reduced(2),2);
        g.setColour(button.getToggleState() ? juce::Colour(0xffffcc6e) : juce::Colour(0xffe5e2d7));
        if (!button.isEnabled()) g.setOpacity(.35f);
        g.setFont(juce::FontOptions(juce::jlimit(9.0f,17.0f,bounds.getHeight()*.4f)));
        g.drawFittedText(button.getButtonText(),button.getLocalBounds().reduced(3),juce::Justification::centred,1);
        if (over || down || button.hasKeyboardFocus(true)) {g.setColour(juce::Colours::orange);g.drawRoundedRectangle(bounds,3,1);}
    }
private:
    std::array<juce::Image,7> bodies,indexes;
    juce::Image buttonOff,buttonOn;
};
}

void MicrophonicFuzzAudioProcessorEditor::addKnob(const char* id,const char* label,float x,float y,float radius,int knobStyle)
{
    auto* parameter = processor.parameters.getParameter(id);
    if (parameter == nullptr) return;
    Knob item;item.bounds={x-radius,y-radius,2*radius,2*radius};item.slider=std::make_unique<juce::Slider>();
    auto& slider=*item.slider;slider.setComponentID(id);slider.setName(label);slider.setTitle(label);slider.setDescription(juce::String(label)+"; drag or use arrow keys");
    slider.setLookAndFeel(style.get());slider.getProperties().set("knobStyle",knobStyle);
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);slider.setRotaryParameters(juce::MathConstants<float>::pi*1.25f,juce::MathConstants<float>::pi*2.75f,true);
    slider.setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);slider.setPopupDisplayEnabled(true,true,this);
    slider.setTooltip(label);slider.setWantsKeyboardFocus(true);addAndMakeVisible(slider);
    item.attachment=std::make_unique<SliderAttachment>(processor.parameters,id,slider);knobs.push_back(std::move(item));
}
void MicrophonicFuzzAudioProcessorEditor::addSwitch(const char* id,const char* label,juce::Rectangle<float> bounds)
{
    if (processor.parameters.getParameter(id)==nullptr)return;
    Switch item;item.bounds=bounds;item.button=std::make_unique<juce::ToggleButton>(label);
    item.button->setComponentID(id);item.button->setName(label);item.button->setTooltip(label);item.button->setLookAndFeel(style.get());item.button->setWantsKeyboardFocus(true);
    addAndMakeVisible(*item.button);item.attachment=std::make_unique<ButtonAttachment>(processor.parameters,id,*item.button);switches.push_back(std::move(item));
}
void MicrophonicFuzzAudioProcessorEditor::addChoice(const char* id,const char* label,juce::StringArray values,juce::Rectangle<float> bounds)
{
    if (processor.parameters.getParameter(id)==nullptr)return;
    Choice item;item.bounds=bounds;item.combo=std::make_unique<RackChoice>();item.combo->setComponentID(id);item.combo->setName(label);item.combo->setTooltip(label);item.combo->setLookAndFeel(style.get());
    item.combo->addItemList(values,1);addAndMakeVisible(*item.combo);
    item.attachment=std::make_unique<ComboAttachment>(processor.parameters,id,*item.combo);choices.push_back(std::move(item));
}
MicrophonicFuzzAudioProcessorEditor::MicrophonicFuzzAudioProcessorEditor(MicrophonicFuzzAudioProcessor& p)
 : AudioProcessorEditor(&p),processor(p),style(std::make_unique<RackStyle>())
{
    for (auto name : {"rack_base", "panel_header", "panel_tube", "panel_fuzz", "panel_virtual_mic", "panel_room_ir", "panel_feedback", "panel_filter1", "panel_filter2", "panel_modulation", "panel_analyzer", "panel_output", "panel_scope"})
        layers.push_back({name,artwork(juce::String(name)+"_png")});
    layers.push_back({"mic chamber",artwork("mic_chamber_background_png")});
    layers.push_back({"mic geometry",artwork("mic_capsule_geometry_png")});
    layers.push_back({"tube",artwork("tube_assembly_png")});
    layers.push_back({"labels",artwork("labels_overlay_png")});
    addKnob("gain","Fuzz gain",238,201,49,0);addKnob("bias","Bias",363,201,34,1);
    addKnob("tone","Tone",238,324,34,1);addKnob("gate","Gate",363,324,34,1);
    addKnob("micDistance","Microphone distance",493,204,28,3);addKnob("micPosition","Microphone position",493,320,28,3);
    addKnob("micMovement","Microphone movement",777,204,28,3);addKnob("micSensitivity","Microphone sensitivity",777,320,28,3);
    addKnob("irMix","IR mix",900,355,28,6);addKnob("early","Early reflections",994,355,28,6);addKnob("decay","IR decay",1095,355,28,6);
    addKnob("feedback","Feedback",1283,221,94,0);addKnob("resonance","Resonance",1228,374,34,1);addKnob("instability","Instability",1356,374,34,1);
    addKnob("filter1Freq","Filter 1 frequency",1487,178,29,3);addKnob("filter1Res","Filter 1 resonance",1577,178,25,3);
    addKnob("filter2Freq","Filter 2 frequency",1487,353,29,3);addKnob("filter2Res","Filter 2 resonance",1577,353,25,3);
    addKnob("tremoloRate","Tremolo rate",184,535,21,4);addKnob("tremoloDepth","Tremolo depth",240,535,21,4);
    addKnob("flangerRate","Flanger rate",382,535,21,4);addKnob("flangerDepth","Flanger depth",434,535,21,4);addKnob("flangerFeedback","Flanger feedback",484,535,21,4);
    addKnob("octLevel","Octafuzz amount",467,475,16,6);
    addKnob("mix","Dry/wet mix",1128,520,41,0);addKnob("output","Output gain",1257,520,41,0);
    addSwitch("bypass","BYPASS",{1309,19,88,33});addSwitch("limiter","ON",{1357,510,51,33});
    addSwitch("tremoloOn","~",{92,511,50,40});addSwitch("flangerOn","~",{300,511,50,40});
    addSwitch("octOn","OCT",{91,458,73,31});
    addChoice("type","Fuzz type",{"Si","Ge","LED","BJT"},{232,378,171,30});
    addChoice("octMode","Octafuzz octave",{"DOWN","UP","BOTH"},{182,458,220,31});
    addChoice("micMode","Virtual Mic mode",{"SUSTAIN","SCREAM","CHAOS"},{537,383,253,32});
    addChoice("roomMode","IR library",{"USER","FACTORY + USER"},{923,129,137,35});
    for (auto& item:choices) if(item.combo->getComponentID()=="roomMode") item.combo->setItemEnabled(2,processor.hasFactoryIR());
    loadIR.setName("Load user room impulse response WAV");loadIR.setTooltip("Load user IR into the feedback path");loadIR.setLookAndFeel(style.get());addAndMakeVisible(loadIR);
    loadIR.onClick=[this]
    {
        fileChooser=std::make_unique<juce::FileChooser>("Load room IR (WAV)",juce::File(),"*.wav");
        fileChooser->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,
          [safe=juce::Component::SafePointer<MicrophonicFuzzAudioProcessorEditor>(this)](const juce::FileChooser& chooser)
          {
              if(safe==nullptr || chooser.getResult()==juce::File())return;
              const auto result=safe->processor.loadUserIR(chooser.getResult());
              safe->irStatus=result.wasOk()?chooser.getResult().getFileName():result.getErrorMessage();safe->repaint();
          });
    };
    for(auto* button:{&previousIR,&nextIR,&savePreset,&copyPreset,&undo,&redo,&settings,&roomA,&roomB,&irMenu})
    {
        button->setLookAndFeel(style.get());button->setWantsKeyboardFocus(true);addAndMakeVisible(*button);
    }
    previousIR.setName("Previous factory IR");nextIR.setName("Next factory IR");settings.setName("Settings and analyzer view");irMenu.setName("Room IR source and loading");
    previousIR.setTooltip("Previous factory room preset");nextIR.setTooltip("Next factory room preset");savePreset.setTooltip("Save plugin preset");copyPreset.setTooltip("Copy preset state to clipboard");
    undo.setTooltip("Undo parameter edit");redo.setTooltip("Redo parameter edit");settings.setTooltip("Preset loading and analyzer display");roomA.setTooltip("USER: loaded WAV only");roomB.setTooltip("FACTORY: selected synthetic preset plus optional USER WAV");
    auto setRoom=[this](float value){if(auto* parameter=processor.parameters.getParameter("roomMode")){parameter->beginChangeGesture();parameter->setValueNotifyingHost(value);parameter->endChangeGesture();}};
    roomA.onClick=[setRoom]{setRoom(0);};roomB.onClick=[setRoom]{setRoom(1);};
    irMenu.setTooltip("Room IR source and loading");
    irMenu.onClick=[this,setRoom]
    {
        juce::PopupMenu menu;
        menu.addItem(1,"Load user WAV IR...");
        menu.addSeparator();
        menu.addItem(2,"USER - loaded WAV only",true,processor.parameters.getRawParameterValue("roomMode")->load()<.5f);
        menu.addItem(3,"FACTORY + optional USER WAV",true,processor.parameters.getRawParameterValue("roomMode")->load()>=.5f);
        menu.addSeparator();
        const int selected=juce::jlimit(0,JerzyFactoryIR::count-1,
            juce::roundToInt(processor.parameters.getRawParameterValue("factoryIR")->load()));
        for(int i=0;i<JerzyFactoryIR::count;++i)
            menu.addItem(10+i,JerzyFactoryIR::nameFor(i),true,i==selected);
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(irMenu),
          [safe=juce::Component::SafePointer<MicrophonicFuzzAudioProcessorEditor>(this),setRoom](int choice){
              if(safe==nullptr)return;
              if(choice==1)safe->loadIR.triggerClick();
              else if(choice==2)setRoom(0);
              else if(choice==3)setRoom(1);
              else if(choice>=10&&choice<10+JerzyFactoryIR::count){
                  if(auto* p=safe->processor.parameters.getParameter("factoryIR")){
                      p->beginChangeGesture();p->setValueNotifyingHost(p->convertTo0to1(float(choice-10)));p->endChangeGesture();
                  }
                  setRoom(1);
              }
          });
    };
    auto stepFactory=[this,setRoom](int offset){
        auto* p=processor.parameters.getParameter("factoryIR");if(!p)return;
        const int current=juce::jlimit(0,JerzyFactoryIR::count-1,
            juce::roundToInt(processor.parameters.getRawParameterValue("factoryIR")->load()));
        const int next=(current+offset+JerzyFactoryIR::count)%JerzyFactoryIR::count;
        p->beginChangeGesture();p->setValueNotifyingHost(p->convertTo0to1(float(next)));p->endChangeGesture();
        setRoom(1);
    };
    previousIR.onClick=[stepFactory]{stepFactory(-1);};
    nextIR.onClick=[stepFactory]{stepFactory(1);};
    savePreset.onClick=[this]{choosePreset(true);};
    copyPreset.onClick=[this]{juce::MemoryBlock data;processor.getStateInformation(data);juce::SystemClipboard::copyTextToClipboard(data.toBase64Encoding());};
    undo.onClick=[this]{processor.undoManager.undo();};redo.onClick=[this]{processor.undoManager.redo();};
    settings.onClick=[this]
    {
        juce::PopupMenu menu;menu.addItem(1,"Load plugin preset...");menu.addSeparator();menu.addItem(2,"Signal spectrum",true,!showVU);menu.addItem(3,"VU meter",true,showVU);
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(settings),[safe=juce::Component::SafePointer<MicrophonicFuzzAudioProcessorEditor>(this)](int choice){if(safe==nullptr)return;if(choice==1)safe->choosePreset(false);if(choice==2)safe->setAnalyzerMode(false);if(choice==3)safe->setAnalyzerMode(true);safe->repaint();});
    };
    ledImage=artwork("control_led_amber_png");vuFace=artwork("vu_face_png");vuNeedle=artwork("vu_needle_png");
    setResizable(true,true);setResizeLimits(836,300,2006,720);getConstrainer()->setFixedAspectRatio(1672.0/600.0);setSize(1254,450);startTimerHz(30);
}
MicrophonicFuzzAudioProcessorEditor::~MicrophonicFuzzAudioProcessorEditor()
{
    stopTimer();fileChooser.reset();loadIR.setLookAndFeel(nullptr);
    for(auto* button:{&previousIR,&nextIR,&savePreset,&copyPreset,&undo,&redo,&settings,&roomA,&roomB,&irMenu})button->setLookAndFeel(nullptr);
    for(auto& item:knobs)item.slider->setLookAndFeel(nullptr);
    for(auto& item:switches)item.button->setLookAndFeel(nullptr);
    for(auto& item:choices)item.combo->setLookAndFeel(nullptr);
}
void MicrophonicFuzzAudioProcessorEditor::resized()
{
    const float sx=getWidth()/1672.0f,sy=getHeight()/600.0f;
    auto scale=[sx,sy](juce::Rectangle<float> r){return juce::Rectangle<float>(r.getX()*sx,r.getY()*sy,r.getWidth()*sx,r.getHeight()*sy).toNearestInt();};
    for(auto& item:knobs)item.slider->setBounds(scale(item.bounds));
    for(auto& item:switches)item.button->setBounds(scale(item.bounds));
    for(auto& item:choices)item.combo->setBounds(scale(item.bounds));
    loadIR.setBounds(scale({920,401,70,27}));irMenu.setBounds(scale({999,401,62,27}));
    previousIR.setBounds(scale({655,18,37,38}));nextIR.setBounds(scale({985,18,37,38}));savePreset.setBounds(scale({1030,18,40,38}));copyPreset.setBounds(scale({1075,18,40,38}));
    roomA.setBounds(scale({1175,18,43,38}));roomB.setBounds(scale({1220,18,43,38}));undo.setBounds(scale({1438,18,40,38}));redo.setBounds(scale({1482,18,40,38}));settings.setBounds(scale({1566,18,47,38}));
}
void MicrophonicFuzzAudioProcessorEditor::choosePreset(bool save)
{
    fileChooser=std::make_unique<juce::FileChooser>(save?"Save Jerzy Microphonic Fuzz preset":"Load Jerzy Microphonic Fuzz preset",juce::File(),"*.mfpreset");
    fileChooser->launchAsync((save?juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::warnAboutOverwriting:juce::FileBrowserComponent::openMode)|juce::FileBrowserComponent::canSelectFiles,
      [safe=juce::Component::SafePointer<MicrophonicFuzzAudioProcessorEditor>(this),save](const juce::FileChooser& chooser)
      {
        if(safe==nullptr||chooser.getResult()==juce::File())return;
        if(save){juce::MemoryBlock data;safe->processor.getStateInformation(data);if(!chooser.getResult().withFileExtension("mfpreset").replaceWithData(data.getData(),data.getSize()))safe->irStatus="Preset save failed";}
        else {juce::MemoryBlock data;if(chooser.getResult().loadFileAsData(data))safe->processor.setStateInformation(data.getData(),int(data.getSize()));else safe->irStatus="Preset load failed";}
      });
}
void MicrophonicFuzzAudioProcessorEditor::mouseDoubleClick(const juce::MouseEvent& event)
{
    if(juce::Rectangle<float>(529,446,525,139).contains(event.position.x*1672.0f/getWidth(),event.position.y*600.0f/getHeight())) {showVU=!showVU;repaint();}
}
void MicrophonicFuzzAudioProcessorEditor::timerCallback()
{
    const bool useFactory=processor.parameters.getRawParameterValue("roomMode")->load()>.5f;
    roomA.setToggleState(!useFactory,juce::dontSendNotification);roomB.setToggleState(useFactory,juce::dontSendNotification);roomB.setEnabled(processor.hasFactoryIR());
    undo.setEnabled(processor.undoManager.canUndo());redo.setEnabled(processor.undoManager.canRedo());
    if(processor.isIRPreparing())irStatus="Preparing room IR...";
    else irStatus=processor.getIRStatus();
    processor.copyRoomIRPreview(irPreview.data(),int(irPreview.size()));
    vuRms+=(1.0f-std::exp(-1.0f/(30.0f*.3f)))*(processor.getOutputRms()-vuRms);
    peak=processor.getOutputPeak();processor.copyWaveform(waveform.data(),int(waveform.size()));
    fftData.fill(0);
    for(size_t i=0;i<waveform.size();++i) fftData[i]=waveform[i]*(.5f-.5f*std::cos(2.0f*juce::MathConstants<float>::pi*float(i)/511.0f));
    fft.performFrequencyOnlyForwardTransform(fftData.data());repaint();
}
void MicrophonicFuzzAudioProcessorEditor::paint(juce::Graphics& original)
{
    juce::Graphics::ScopedSaveState state(original);auto& g=original;
    g.addTransform(juce::AffineTransform::scale(getWidth()/1672.0f,getHeight()/600.0f));g.fillAll(juce::Colour(0xff191b1d));
    bool background=false;
    for(const auto& layer:layers)if(layer.image.isValid()){if(layer.name=="mic chamber")g.drawImage(layer.image,juce::Rectangle<float>(542,129,185,237));
        else if(layer.name=="mic geometry")
        {
            const float distance=processor.parameters.getRawParameterValue("micDistance")->load();
            const float position=processor.parameters.getRawParameterValue("micPosition")->load();
            const float scale=1.0f-.13f*distance;
            auto rectangle=juce::Rectangle<float>(542,129,185*scale,237*scale).withCentre({634.5f+(position-.5f)*20,247.5f+processor.getMicVisualOffset()*6});
            g.drawImage(layer.image,rectangle);
        }
        else if(layer.name=="tube")g.drawImage(layer.image,juce::Rectangle<float>(50,12,101,379));
        else g.drawImage(layer.image,juce::Rectangle<float>(0,0,1672,600));background=true;}
    if(!background){g.setColour(juce::Colours::ivory);g.setFont(juce::FontOptions(26.0f));g.drawText("JERZY MICROPHONIC FUZZ - fallback build",50,10,700,40,juce::Justification::centredLeft);}
    // Dedicated OCTAFUZZ strip inside the existing FX area. It is drawn over
    // the decorative rack atlas, but does not move or cover existing FX controls.
    g.setColour(juce::Colour(0xf0161b1b));g.fillRoundedRectangle(76.f,443.f,437.f,50.f,5.f);
    g.setColour(juce::Colour(0xff9b753b));g.drawRoundedRectangle(76.f,443.f,437.f,50.f,5.f,1.f);
    g.setColour(juce::Colour(0xffffd39b));g.setFont(juce::FontOptions(11.0f));
    g.drawText("OCTAFUZZ",85,444,115,15,juce::Justification::centredLeft);
    g.setColour(juce::Colour(0xffa9aaa2));g.drawText("LEVEL",445,446,55,13,juce::Justification::centred);
    const float feedback=processor.parameters.getParameter("feedback")->getValue();
    juce::Path feedbackArc;feedbackArc.addCentredArc(1283,221,93,93,0,juce::MathConstants<float>::pi*1.25f,juce::MathConstants<float>::pi*(1.25f+1.5f*feedback),true);
    g.setColour(juce::Colour(0xffff9d12).withAlpha(.16f));g.strokePath(feedbackArc,juce::PathStrokeType(10));
    g.setColour(juce::Colour(0xffffbe35));g.strokePath(feedbackArc,juce::PathStrokeType(3));
    // Separate signal-driven arc: the upper arc remains the feedback control.
    const float fbActivity=juce::jlimit(0.f,1.f,processor.getFeedbackActivity());
    juce::Path activityArc;activityArc.addCentredArc(1283,221,82,82,0,
        juce::MathConstants<float>::pi*1.25f,
        juce::MathConstants<float>::pi*(1.25f+1.5f*fbActivity),true);
    g.setColour(juce::Colour(0xffec4826));g.strokePath(activityArc,juce::PathStrokeType(2));
    g.setColour(juce::Colour(0xff101313));g.fillRoundedRectangle(698,18,283,38,3);g.setColour(juce::Colour(0xffeee3bd));g.setFont(juce::FontOptions(17.0f));
    g.drawText(processor.getIRStatus(),698,18,283,38,juce::Justification::centred);
    g.setColour(juce::Colour(0xff090d0d));g.fillRoundedRectangle(852,174,270,91,5);g.setColour(juce::Colour(0xff77705c));g.drawRoundedRectangle(852,174,270,91,5,1);
    juce::Path impulse;
    for(size_t i=0;i<irPreview.size();++i){const float x=860.0f+254.0f*float(i)/127.0f;const float y=246.0f-irPreview[i]*61.0f;if(i==0)impulse.startNewSubPath(x,y);else impulse.lineTo(x,y);}
    g.setColour(juce::Colour(0xffffb634));g.strokePath(impulse,juce::PathStrokeType(1.4f));
    if(processor.getRoomIRDuration()>0){g.setColour(juce::Colour(0xffaaa48e));g.setFont(juce::FontOptions(10.0f));g.drawText(juce::String(processor.getRoomIRDuration(),3)+" s",1050,248,63,13,juce::Justification::centredRight);}
    // Actual signal analyzers. No illustrative waveform/spectrum atlases are used.
    g.setColour(juce::Colour(0xff080c0c));g.fillRoundedRectangle(545,460,493,104,4);
    g.setColour(juce::Colour(0xff353025));
    for(int y=0;y<5;++y)g.drawHorizontalLine(465+y*23,551,1030);
    for(int x=0;x<12;++x)g.drawVerticalLine(551+x*43,465,560);
    juce::Path spectrum;
    const float rate=float(processor.getProcessingSampleRate());
    for(int x=0;x<=458;++x)
    {
        const float hz=20.0f*std::pow(1000.0f,float(x)/458.0f);
        const float fractionalBin=hz*512.0f/juce::jmax(8000.0f,rate);
        const int bin=juce::jlimit(0,255,int(fractionalBin));const float fraction=juce::jlimit(0.0f,1.0f,fractionalBin-bin);
        const float magnitude=hz<rate*.5f ? fftData[size_t(bin)]*(1-fraction)+fftData[size_t(juce::jmin(255,bin+1))]*fraction : 0.0f;
        const float db=juce::Decibels::gainToDecibels(magnitude/128.0f,-60.0f);
        const float y=557.0f-juce::jlimit(0.0f,1.0f,(db+60.0f)/60.0f)*91.0f;
        if(x==0)spectrum.startNewSubPath(575,y);else spectrum.lineTo(575.0f+x,y);
    }
    g.setColour(juce::Colour(0xffffb634));g.strokePath(spectrum,juce::PathStrokeType(1.5f));
    if(showVU && vuFace.isValid() && vuNeedle.isValid())
    {
        const juce::Rectangle<float> target(680,458,202,108);
        g.setColour(juce::Colour(0xff080c0c));g.fillRect(545,460,493,104);g.drawImage(vuFace,target);
        juce::Graphics::ScopedSaveState needleState(g);
        const auto pivot=juce::Point<float>(target.getCentreX(),target.getY()+target.getHeight()*.845f);
        const auto normalized=juce::jlimit(0.0f,1.0f,(juce::Decibels::gainToDecibels(vuRms,-60.0f)+38.0f)/23.0f);
        g.addTransform(juce::AffineTransform::rotation((normalized-.5f)*1.5f,pivot.x,pivot.y));g.drawImage(vuNeedle,target);
    }
    juce::Path wave;
    for(int i=0;i<80;++i){float y=518.0f-juce::jlimit(-1.0f,1.0f,waveform[size_t(i*6)])*35.0f;if(i==0)wave.startNewSubPath(1542,y);else wave.lineTo(1542.0f+i,y);}
    g.setColour(juce::Colour(0xffcbc7af));g.strokePath(wave,juce::PathStrokeType(1.2f));
    for(int channel=0;channel<2;++channel)
    {
        const float level=juce::jlimit(0.0f,1.0f,(juce::Decibels::gainToDecibels(processor.getOutputPeak(channel),-60.0f)+60.0f)/60.0f);
        for(int i=0;i<20;++i){g.setColour(i<float(20)*level?(i>16?juce::Colours::orangered:i>12?juce::Colours::orange:juce::Colours::yellowgreen):juce::Colour(0xff242516));g.fillRect(1443+channel*19,555-i*4,13,3);}
    }
    const auto& led = ledImage;
    if(led.isValid())
    {
        const bool active=processor.parameters.getRawParameterValue("bypass")->load()<.5f;
        for(auto centre: {juce::Point<float>(271,105),{550,105},{934,105},{1220,105},{1586,107},{1586,278},{88,472}})
        {
            juce::ColourGradient glow(juce::Colour(0xffff8b0d).withAlpha(active?.45f:.03f),centre.x,centre.y,juce::Colours::transparentBlack,centre.x+18,centre.y,true);
            g.setGradientFill(glow);g.fillEllipse(centre.x-18,centre.y-18,36.0f,36.0f);
            g.setOpacity(active ? 1.0f : .15f);g.drawImage(led,juce::Rectangle<float>(centre.x-10,centre.y-10,20,20));
        }
        g.setOpacity(processor.parameters.getRawParameterValue("limiter")->load()>.5f?1.0f:.15f);g.drawImage(led,juce::Rectangle<float>(1338,475,20,20));g.setOpacity(1);
    }
    g.setColour(juce::Colour(0xff111413));g.fillRect(852,270,270,27);
    g.setColour(juce::Colour(0xffe6e1cd));g.setFont(juce::FontOptions(13.0f));g.drawFittedText(irStatus,{853,270,268,27},juce::Justification::centredLeft,1);
}
