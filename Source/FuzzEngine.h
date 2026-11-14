#pragma once
#include <array>
#include <atomic>
#include <cmath>
#include <algorithm>

// Bounded base-rate reference engine. No heap allocation in prepare/process.
class FuzzEngine
{
public:
    static constexpr int maxIR = 2048;
    struct Parameters {
        float gain=6, feedback=.2f, mix=1, output=.8f, bias=0, tone=.5f, gate=0, type=0;
        float micDistance=.3f,micPosition=.5f,micMovement=0,micSensitivity=1,micMode=0;
        float resonance=.3f,instability=0,filter1Freq=5000,filter1Res=0,filter2Freq=80,filter2Res=0;
        float tremoloRate=4,tremoloDepth=0,tremoloOn=0,flangerRate=.3f,flangerDepth=0,flangerFeedback=0,flangerOn=0;
        float limiter=1,bypass=0,roomMode=0,irMix=.5f,early=.3f,decay=.4f;
    };
    void prepare(double rate) { sr=std::clamp(std::isfinite(rate)?rate:44100.0,8000.0,384000.0); reset(); }
    void reset() { channels={}; phase=0; current=Parameters{}; feedbackActivity=0; micVisualOffset=0; }
    // Call only while processing is stopped, e.g. prepareToPlay.
    void clearIR() { pending.store(-1); active.store(0); lengths.fill(0); loaded.store(false); swapping=false;irGain=1; }
    // Caller only writes an unowned slot; publication happens once at block boundary.
    bool stageIR(const float* data,int count) {
        if(data==nullptr || count<1 || count>maxIR || pending.load(std::memory_order_acquire)>=0) return false;
        const int a=active.load(std::memory_order_acquire); int slot=(a+1)%3;
        auto& target=irs[static_cast<size_t>(slot)]; target.fill(0); double norm=0;
        for(int i=0;i<count;++i) { if(!std::isfinite(data[i])) return false; norm+=std::abs(static_cast<double>(data[i])); }
        if(norm<1e-10f) return false;
        for(int i=0;i<count;++i) target[static_cast<size_t>(i)]=static_cast<float>(static_cast<double>(data[i])/std::max(norm,1.0));
        lengths[static_cast<size_t>(slot)]=count; pending.store(slot,std::memory_order_release); return true;
    }
    float getMicVisualOffset() const {return micVisualOffset;}
    // Read from the audio callback only; publishing to the GUI is done by the processor.
    float getFeedbackActivity() const noexcept {return feedbackActivity;}
    bool hasIR() const { return loaded.load(); }
    bool isIRSwapPending() const { return pending.load(std::memory_order_acquire)>=0; }
    bool stageClearIR() {
        if(isIRSwapPending()) return false;
        const int slot=(active.load(std::memory_order_acquire)+1)%3;
        lengths[static_cast<size_t>(slot)]=0; pending.store(slot,std::memory_order_release);return true;
    }
    void beginBlock() { if(pending.load(std::memory_order_acquire)>=0) swapping=true; }
    void process(float& l,float& r,const Parameters& target,bool stereo=true,const std::array<float,2>* externalRoom=nullptr,std::array<float,2>* pickupOutput=nullptr) {
        // Per-sample smoothing and explicit domain sanitisation; a bad host value
        // must neither poison state nor cause out-of-range delay/feedback indexes.
        const float s=static_cast<float>(1-std::exp(-1/(.01*sr)));
#define SM(x,lo,hi) do {const float v=std::isfinite(target.x)?std::clamp(target.x,lo,hi):current.x; current.x+=s*(v-current.x);} while(false)
        SM(gain,1.f,30.f);SM(feedback,0.f,.85f);SM(mix,0.f,1.f);SM(output,0.f,2.f);
        SM(bias,-1.f,1.f);SM(tone,0.f,1.f);SM(gate,0.f,1.f);SM(type,0.f,3.f);
        SM(micDistance,0.f,1.f);SM(micPosition,0.f,1.f);SM(micMovement,0.f,1.f);
        SM(micSensitivity,0.f,2.f);SM(micMode,0.f,1.f);SM(resonance,0.f,1.f);SM(instability,0.f,1.f);
        SM(filter1Freq,20.f,20000.f);SM(filter1Res,0.f,1.f);SM(filter2Freq,20.f,20000.f);SM(filter2Res,0.f,1.f);
        SM(tremoloRate,.1f,20.f);SM(tremoloDepth,0.f,1.f);SM(tremoloOn,0.f,1.f);
        SM(flangerRate,.05f,5.f);SM(flangerDepth,0.f,1.f);SM(flangerFeedback,0.f,1.f);SM(flangerOn,0.f,1.f);
        SM(limiter,0.f,1.f);SM(bypass,0.f,1.f);SM(roomMode,0.f,1.f);
        SM(irMix,0.f,1.f);SM(early,0.f,1.f);SM(decay,0.f,1.f);
#undef SM
        if(swapping) { irGain=std::max(0.f,irGain-1.f/256); if(irGain==0) { int p=pending.load(std::memory_order_acquire); if(p>=0) { active.store(p,std::memory_order_release); loaded.store(lengths[static_cast<size_t>(p)]>0); pending.store(-1,std::memory_order_release); } swapping=false; } }
        else irGain=std::min(1.f,irGain+1.f/256);
        phase+=1/sr; if(phase>10000)phase=0;
        l=sample(l,channels[0],externalRoom?&(*externalRoom)[0]:nullptr,pickupOutput?&(*pickupOutput)[0]:nullptr);
        if(stereo)r=sample(r,channels[1],externalRoom?&(*externalRoom)[1]:nullptr,pickupOutput?&(*pickupOutput)[1]:nullptr);
        else {r=l;if(pickupOutput)(*pickupOutput)[1]=(*pickupOutput)[0];}
        feedbackActivity=std::clamp(std::max(channels[0].fbLevel,stereo?channels[1].fbLevel:0.f)
                                    *current.feedback,0.f,1.f);
    }
private:
    struct Channel { std::array<float,32768> delay{};std::array<float,maxIR> history{};int cursor=0,head=0;float dcIn=0,dcOut=0,low=0,hp=0,mic=0,fb=0,env=0,flange=0,postIn=0,postOut=0,toneState=0,fbLevel=0,fbSafety=1; };
    std::array<Channel,2> channels{};
    std::array<std::array<float,maxIR>,3> irs{};
    std::array<int,3> lengths{};
    std::atomic<int> active{0},pending{-1};std::atomic<bool> loaded{false};
    Parameters current;double sr=44100,phase=0;float irGain=1;bool swapping=false;float micVisualOffset=0,feedbackActivity=0;
    float coeff(float hz)const {return static_cast<float>(1-std::exp(-6.283185307*std::clamp(hz,10.f,static_cast<float>(sr*.45))/sr));}
    static float safe(float x){return std::isfinite(x)?std::clamp(x,-8.f,8.f):0.f;}
    // Four intentionally distinct *software* voicings, not measured diode/BJT circuits.
    // BIAS moves the clipping centre and changes positive/negative headroom.
    static float voiceCurve(float x,float bias,int voice) noexcept {
        const float b=bias*(voice==1?.95f:voice==2?.42f:voice==3?.72f:.67f);
        const auto curve=[voice,bias](float z) {
            // An asymmetric transfer function models unequal positive/negative conduction.
            const float w=z*(1.f+(.18f*bias)*(z>=0?-1.f:1.f));
            switch(voice){
            case 0: return .85f*std::tanh(w*1.75f)+.15f*std::tanh(w*3.1f); // Si: harder knee
            case 1: return .76f*std::tanh(w*.9f)+.24f*std::tanh(w*1.8f); // Ge: softer knee
            case 2: return 1.02f*std::tanh(w*.52f)+.10f*std::tanh(w*1.15f); // LED: larger headroom
            default: {
                const float first=std::tanh(w*2.5f);
                return .78f*std::tanh(1.8f*first+.22f*w)+.18f*first; // BJT: cascaded asymmetry
            }
            }
        };
        return curve(x+b)-curve(b);
    }
    float sample(float input,Channel& c,const float* externalRoom=nullptr,float* pickupOutput=nullptr) {
        const auto& p=current;const float dry=safe(input);
        float x=dry-c.dcIn+.995f*c.dcOut;c.dcIn=dry;c.dcOut=x;
        c.env+=coeff(15)*(std::abs(x)-c.env);
        if(c.env<p.gate*.1f) x*=c.env/(p.gate*.1f+1e-8f);
        const float move=std::sin(static_cast<float>(phase*6.283185307*(.2+p.instability*3)));
        const float distanceMetres=.1f+3.9f*p.micDistance;
        // Effective motion adds at most +/-0.5m; acoustic path never falls below 0.05m.
        const float pathMetres=std::clamp(distanceMetres+.5f*p.micMovement*move,.05f,4.5f);
        micVisualOffset=pathMetres-distanceMetres;
        const float fractionalDelay=std::clamp(static_cast<float>(pathMetres/343.f*sr),1.f,30000.f);
        const int delay=static_cast<int>(fractionalDelay);const float fraction=fractionalDelay-static_cast<float>(delay);
        float pickup=c.delay[static_cast<size_t>((c.cursor-delay+32768)%32768)]*(1-fraction)
                    +c.delay[static_cast<size_t>((c.cursor-delay-1+32768)%32768)]*fraction;
        c.mic+=coeff(1200+15000*(1-p.micPosition))*(pickup-c.mic);
        pickup=(c.mic*(1-p.micMode)+ (pickup-c.mic)*p.micMode)*p.micSensitivity/(1+distanceMetres);
        if(pickupOutput)*pickupOutput=pickup;
        c.history[static_cast<size_t>(c.head)]=pickup;
        float room=pickup;const int a=active.load(std::memory_order_relaxed);
        if(lengths[static_cast<size_t>(a)]>0) {room=0;for(int k=0;k<lengths[static_cast<size_t>(a)];++k)room+=irs[static_cast<size_t>(a)][static_cast<size_t>(k)]*c.history[static_cast<size_t>((c.head-k+maxIR)%maxIR)];}
        // Independent bounded early reflections; roomMode B is unavailable without a licensed library.
        float reflection=0;for(int j=1;j<=4;++j){int d=std::min(32000,static_cast<int>(sr*(.007*j+.011*j*j)));reflection+=std::pow(.15f+.65f*p.decay,static_cast<float>(j))*c.delay[static_cast<size_t>((c.cursor-d+32768)%32768)]*.25f;}
        if(externalRoom)room=safe(*externalRoom);
        room += p.early * reflection; // Early reflections are a separate room control; no factory library is supplied.
        room=pickup+(room-pickup)*p.irMix*irGain;
        c.head=(c.head+1)%maxIR;
        c.fb+=coeff(300+11000*(1-p.resonance))*(room-c.fb);
        // Follow the true feedback-loop level with a slower gain stabiliser.
        // This retains growing feedback at low levels but progressively reduces
        // injection near self-oscillation; the stored state is channel-local.
        c.fbLevel+=coeff(16.f)*(std::abs(c.fb)-c.fbLevel);
        const float protectiveGain=std::clamp(.62f/(.18f+c.fbLevel),.22f,1.f);
        c.fbSafety+=coeff(18.f)*(protectiveGain-c.fbSafety);
        const float injection=std::tanh(c.fb*(1.f+p.resonance*2.f))
            *p.feedback*c.fbSafety*(1.f+p.instability*.08f*move);
        x+=injection;
        const float drive=p.gain/(1.f+.3f*c.env);
        const float driven=std::clamp(drive*x,-96.f,96.f);
        const int voice=std::clamp(static_cast<int>(p.type),0,3);
        const float voiceFraction=std::clamp(p.type-static_cast<float>(voice),0.f,1.f);
        // Crossfade adjacent software models while the type parameter ramps.
        float fuzz=voiceCurve(driven,p.bias,voice);
        if(voice<3 && voiceFraction>0.f)fuzz+=(voiceCurve(driven,p.bias,voice+1)-fuzz)*voiceFraction;
        c.low+=coeff(p.filter1Freq)*(fuzz-c.low);fuzz=c.low+p.filter1Res*.5f*(fuzz-c.low);
        c.hp+=coeff(p.filter2Freq)*(fuzz-c.hp);fuzz=(fuzz-c.hp)*(1+p.filter2Res*.4f);
        c.mic=safe(c.mic);c.toneState+=coeff(450+15500*p.tone*p.tone)*(fuzz-c.toneState);fuzz=c.toneState;
        c.delay[static_cast<size_t>(c.cursor)]=std::tanh(fuzz);
        const int fd=std::clamp(static_cast<int>(sr*(.001+.004*(1+std::sin(phase*6.283185307*p.flangerRate))*p.flangerDepth)),1,32000);
        float fl=c.delay[static_cast<size_t>((c.cursor-fd+32768)%32768)];
        c.flange=std::tanh(fl+c.flange*p.flangerFeedback*.85f);
        fuzz=(fuzz+c.flange*p.flangerDepth*p.flangerOn)*(.999f);
        c.cursor=(c.cursor+1)%32768;
        fuzz*=1-p.tremoloOn*p.tremoloDepth*.5f*(1+std::sin(static_cast<float>(phase*6.283185307*p.tremoloRate)));
        float dc=fuzz-c.postIn+.995f*c.postOut;c.postIn=fuzz;c.postOut=dc;
        float result=p.output*((1-p.mix)*dry+p.mix*dc);
        float limited=std::clamp(result,-.98f,.98f);result=result+(limited-result)*p.limiter;
        // Safety ceiling is active even when the optional limiter is switched
        // off or the effect is bypassed; it cannot be defeated by automation.
        return std::clamp(safe(result+(dry-result)*p.bypass),-.995f,.995f);
    }
};
