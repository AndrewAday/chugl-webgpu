/*
name: ckfxr 
desc: ChucK/ChuGL port of DrPetter's sfxr (https://www.drpetter.se/project_sfxr.html)
author: Andrew Zhu Aday (https://ccrma.stanford.edu/~azaday/)
date: June 2024
*/

/*
Audio Graph
Design so that the ckfxr synth is reusable, *without* UI controls

[sin|saw|square|noise] -> LP Filter --> HP Filter --> Phasor --> ADSR
*/

// Delay-based effects including flange, chorus, doubling
// https://github.com/ccrma/music220a/blob/main/07-time-and-space/delay-based-efx/script.js
// https://ccrma.stanford.edu/~dattorro/EffectDesignPart2.pdf
class DelayFX extends Chugraph
{
    // audio signal
    inlet => DelayL delay => outlet;
    delay => Gain feedback => delay;

    // control signal
    SinOsc lfo_delay_mod => Range range_delay_mod => Patch patch_delay_mod(delay, "delay") => blackhole;

    // default to flanger
    flange();  

    // params
    // float feedback_gain 
    // delay_base (center delay duration)
    // delay_mod_freq (lfo to modulate delay length)
    // delay_mod_depth (delay_base is modulated between [delay_base*(1 - depth), delay_base * (1 + depth)])

    // public API ---------------------------------------------

    // Defaults
    // TODO: add vibrato, echo presets

    fun void flange() {
        // typical range: 1~10ms
        delayBase(5::ms);
        feedbackGain(.85);
        delayModFreq(.1);
        delayModDepth(.80);  // 80% depth. oscillate between (1ms, 9ms)
    }

    fun void chorus() {
        // typical range: 5~30ms
        delayBase(12::ms);
        feedbackGain(.55);
        delayModFreq(.2);
        delayModDepth(.50);  // 50% depth. oscillate between (6ms, 18ms)
    }

    fun void double() {
        // typical range: 20~100ms
        delayBase(60::ms);
        feedbackGain(.45);
        delayModFreq(.1);
        delayModDepth(.2);  // 20% depth. oscillate between (48ms, 72ms)
    }

    // Setters
    fun void feedbackGain(float f) { f => this.feedback.gain; }
    fun void delayBase(dur d) {
        delay.delay(d);
        // update output range
        d / samp => float delay_samps;
        range_delay_mod.outRadius(delay_samps , delay_samps * .999);
    }
    fun void delayModFreq(float f) { lfo_delay_mod.freq(f); }
    fun void delayModDepth(float f) { lfo_delay_mod.gain(f); }

    // Getters 
    fun float feedbackGain() { return feedback.gain(); }
    fun dur delayBase() { return delay.delay(); }
    fun float delayModFreq() { return lfo_delay_mod.freq(); }
    fun float delayModDepth() { return lfo_delay_mod.gain(); }
}

/*
keyboard controls
- use UI library only (not chuck HID) to test
- <space> and <return> play sound
- z for undo
*/
class CKFXR extends Chugraph
{
    LPF lpf => HPF hpf => DelayFX delayfx => ADSR adsr => outlet;
    PulseOsc square; SawOsc saw; SinOsc sin; Noise noise;
    [square, saw, sin, noise] @=> UGen waveforms[];

    // modulators
    SinOsc lfo_vibrato => blackhole;
    TriOsc lfo_pwm(0.0) => Range range_pwm(0.001, 0.999) => Patch patch_pwm(square, "width") => blackhole;

    // WaveType enum
    0 => static int WaveType_SQUARE;
    1 => static int WaveType_SAW;
    2 => static int WaveType_SIN;
    3 => static int WaveType_NOISE;

    // Synth params parameters are on [0,1] unless noted SIGNED & thus
    // on [-1,1]
    0 => int p_wave_type;

    // env
    dur p_attack_dur;
    dur p_release_dur; 
    dur p_sustain_dur;  // noteOn - noteOff
    float p_sustain_level;

    // freq (in midi for linear properties) 
    float p_freq_base_midi;    // Start frequency. doing midi from [12, 127] ~ [16.345hz, 12543.85hz]
    float p_freq_limit_midi;   // Min frequency cutoff, anything below is stopped 
    float p_freq_ramp;         // Slide in semitones / s (SIGNED)
    float p_freq_dramp;        // Delta slide semitones / s^2 (SIGNED)

    // Vibrato
    float p_vib_depth; // Vibrato depth [0, 1]
    float p_vib_freq;    // Vibrato speed

    // Tonal change
    float p_arp_mod_midi;      // Freq Change amount (SIGNED)
    dur p_arp_time;          // how long after attack to apply arp_mod

    // PWM
    float p_pwm_depth;         
    float p_pwm_freq;          

    // Flanger
    float p_feedback_gain; 
    dur p_delay_base_dur;
    float p_delay_mod_freq;
    float p_delay_mod_depth;

    // Low-pass filter
    float p_lpf_freq;      // Low-pass filter cutoff
    float p_lpf_ramp;      // Low-pass filter cutoff sweep (semitones / s)
    float p_lpf_resonance; // Low-pass filter resonance

    // High-pass filter
    float p_hpf_freq;     // High-pass filter cutoff
    float p_hpf_ramp;     // High-pass filter cutoff sweep (semitones / s)
    
    // initialize values
    resetParams();

    // public API
    fun void resetParams()
    {
        WaveType_SQUARE => p_wave_type;

        // env
        1::ms => p_attack_dur;
        1::ms => p_release_dur;
        1::second => p_sustain_dur;
        1.0 => p_sustain_level;

        // freq 
        60.0 => p_freq_base_midi;    
        0 => p_freq_limit_midi;
        0 => p_freq_ramp;    
        0 => p_freq_dramp;   

        // vibrato
        0 => p_vib_depth;
        0 => p_vib_freq; 

        // arp
        0 => p_arp_mod_midi;      
        0::ms => p_arp_time;      

        // pwm 
        0 => p_pwm_depth;         
        0 => p_pwm_freq;          

        // delay 
        0 => p_feedback_gain; 
        0::ms => p_delay_base_dur;
        0 => p_delay_mod_freq;
        0 => p_delay_mod_depth;

        // lpf
        20000 => p_lpf_freq;    
        0 => p_lpf_ramp;      
        1 => p_lpf_resonance; 

        // hpf
        20 => p_hpf_freq;    
        0 => p_hpf_ramp;    
    }

    fun void play()
    {   
        _play(++_play_count);
    }

    fun void pickupCoin()
    {
        resetParams();

        if (rnd(1)) WaveType_SAW => p_wave_type;
        else WaveType_SQUARE => p_wave_type;

        Math.random2f(60, 84) => p_freq_base_midi;

        frnd(0.05, 0.15)::second => p_sustain_dur;
        frnd(0.15, 0.3)::second => p_release_dur;
        frnd(.6, .9) => p_sustain_level;

        drnd(p_sustain_dur*.6, p_sustain_dur * 1.2) => p_arp_time;
        // p_sustain_dur  => p_arp_time;
        Math.random2f(2, 24) $ int => p_arp_mod_midi;
        
        play();
    }

    fun void shootLaser()
    {
        resetParams();

        rnd(2) => p_wave_type;
        
        frnd(-120, -12) => p_freq_ramp;
        frnd(-80, -12) => p_freq_dramp;
        frnd(60, 127) => p_freq_base_midi;

        frnd(0.05, 0.2)::second => p_sustain_dur;
        frnd(0.05, 0.3)::second => p_release_dur;
        frnd(.6, .9) => p_sustain_level;

        play();
    }

    // Internal -----------------------------------------------------------------
    // used to prevent overlapping play calls so that the synth can be 
    // retriggered before a previous shred ends. 
    // DO NOT MODIFY _play_count directly. Use play() instead.
    0 => int _play_count; 

    // map user params to synthesis 
    fun void _assign()
    {
        // waveform
        for (auto ugen : waveforms) ugen =< lpf;
        waveforms[p_wave_type] => lpf;
        <<< "assigning", p_wave_type >>>;

        // envelope
        adsr.set(p_attack_dur, 1::ms, p_sustain_level, p_release_dur);

        // frequency
        _setFreq(p_freq_base_midi);

        // bibrato
        lfo_vibrato.freq(p_vib_freq);
        lfo_vibrato.gain(p_vib_depth);

        // duty
        lfo_pwm.freq(p_pwm_freq);
        lfo_pwm.gain(p_pwm_depth);

        // delay effect
        delayfx.feedbackGain(p_feedback_gain);
        delayfx.delayBase(p_delay_base_dur);
        delayfx.delayModFreq(p_delay_mod_freq);
        delayfx.delayModDepth(p_delay_mod_depth);

        // Low-pass filter
        lpf.freq(p_lpf_freq);
        lpf.Q(p_lpf_resonance);

        // High-pass filter
        hpf.freq(p_hpf_freq);
    }

    fun void _play(int pc)
    {
        if (pc != _play_count) return;

        _assign();

        spork ~ _playArp(pc);
        spork ~_playMod(pc);

        <<< "keyon" >>>;
        adsr.keyOn();
        adsr.attackTime() + adsr.decayTime() + p_sustain_dur => now;

        if (pc != this._play_count) return;


        <<< "keyoff" >>>;
        this.adsr.keyOff();
        this.adsr.releaseTime() => now;

        // wait for env to finish. if, after release, pc is the same, bump pc again. 
        // this means no other incoming play requests were made
        // and bumping pc again will stop any inifinite loop sporks
        if (pc == _play_count) _play_count++;
    }

    fun int _setFreq(float freq_midi)
    {
        // only set if it's above the cutoff
        if (freq_midi < p_freq_limit_midi) return false;

        Std.mtof(freq_midi) => float freq_hz;

        freq_hz => sin.freq;
        freq_hz => saw.freq;
        freq_hz => square.freq;
        return true;
    }

    fun void _playArp(int pc)
    {
        p_arp_time => now;
        if (pc != _play_count) return;

        // adjust frequency
        <<< "playArp", p_arp_time / second, p_arp_mod_midi >>>;
        p_arp_mod_midi +=> p_freq_base_midi;
    }

    // frequency + vibrato mod + filter mod
    fun void _playMod(int pc)
    {
        now => time t;

        while (1::ms => now) {
            if (pc != _play_count) return;

            // dt
            (now - t) / second => float dt; // elapsed time in seconds
            now => t;

            // frequency modulation
            (dt * p_freq_ramp) +=> p_freq_base_midi;
            (dt * p_freq_dramp) +=> p_freq_ramp;

            // vibrato (remap from [-1, 1] to [1-depth, 1+depth] )
            (lfo_vibrato.last() + 1) * p_freq_base_midi => float freq_midi;

            // set frequency
            _setFreq(freq_midi);
            
            // lpf ramp
            (dt * p_lpf_ramp) + lpf.freq() => lpf.freq;

            // hpf ramp
            (dt * p_hpf_ramp) + hpf.freq() => hpf.freq;
        }
    }

    // return random float in range [0, range]
    fun float frnd(float range)
    {
        return range * Math.randomf();
    }

    fun float frnd(float l, float h)
    {
        return Math.random2f(l, h);
    }

    // return random duration
    fun dur drnd(dur a, dur b) 
    {
        return Math.random2f(a/samp, b/samp)::samp;
    }

    // return random int in range [0, n]
    fun int rnd(int n)
    {
        return Math.random() % (n + 1);
    }
}

CKFXR ckfxr => dac;

class CKFXR_Params
{
    0 => int waveform;

    UI_Float attack_dur_ms;
    UI_Float release_dur_ms;
    UI_Float sustain_dur_ms;
    UI_Float sustain_level;

    UI_Float freq_base_midi;    
    UI_Float freq_limit_midi;   
    UI_Float freq_ramp;  // semitones / sec
    UI_Float freq_dramp; // semitones / sec

    UI_Float vib_depth;
    UI_Float vib_freq; // hz

    UI_Float arp_mod_midi; 
    UI_Float arp_time_ms;       

    UI_Float pwm_depth;         
    UI_Float pwm_freq;          

    UI_Float feedback_gain; 
    UI_Float delay_base_dur_ms;
    UI_Float delay_mod_freq;
    UI_Float delay_mod_depth;

    UI_Float lpf_freq;     
    UI_Float lpf_ramp;     
    UI_Float lpf_resonance;

    UI_Float hpf_freq;     
    UI_Float hpf_ramp;

    UI_Float main_gain;
    UI_String export_wav_path;
}

fun void centerNext(float item_width)
{
    UI.getContentRegionAvail().x => float avail_width;
    UI.setCursorPosX(UI.getCursorPosX() + (avail_width - item_width) * .5);
}

fun void ui()
{

CKFXR_Params p;
["square", "saw", "sin", "noise"] @=> string waveforms[];

while (1) {
GG.nextFrame() => now;

UI.showDemoWindow(null);

if (UI.begin("CKFXR", null, 0)) {
    UI.getWindowSize() => vec2 size;
    // Left Pane ---------------------------------------
    UI.beginChild(
        "Left Generator",
        @(size.x * .2, 0), 
        UI_ChildFlags.Border,
        0
    );
    for (int i; i < 100; i++)
    {
        // FIXME: Good candidate to use ImGuiSelectableFlags_SelectOnNav
        // char label[128];
        // sprintf(label, "MyObject %d", i);
        // if (UI.Selectable(label, selected == i))
        //     selected = i;
    }
    UI.endChild(); // "Left Generator"

    UI.sameLine();

    // Middle Pane ---------------------------------------
    // UI.beginGroup();
    UI.beginChild(
        "item view", 
        @(size.x * .6, -UI.getFrameHeightWithSpacing()),
        // UI_ChildFlags.Border,
        0,
        0
    ); // Leave room for 1 line below us
    UI.text("Manual Settings");

    UI.getWindowWidth() => float middle_width;

    // waveform selection
    UI.separatorText("Waveform");
    for (int n; n < waveforms.size(); n++) {
        if (UI.selectable(waveforms[n], p.waveform == n, 0, 
            @(middle_width / waveforms.size(), 0))
        )
            n => p.waveform;
        if (n < waveforms.size() - 1) UI.sameLine();
    }

    UI.separatorText("Envelope");
    UI.itemTooltip("ctrl+click a slider to input a specific value");

    UI.slider("Attack (ms)", p.attack_dur_ms, 1, 1000);
    UI.slider("Release (ms)", p.release_dur_ms, 1, 1000);
    UI.slider("Sustain (ms)", p.sustain_dur_ms, 1, 1000);
    UI.slider("Sustain Level", p.sustain_level, 0, 1);

    UI.separatorText("Frequency");
    UI.slider("Base (MIDI)", p.freq_base_midi, 0, 127);
    UI.slider("Limit (MIDI)", p.freq_limit_midi, 0, 127);
    UI.slider("Ramp (semitones/s)", p.freq_ramp, -127, 127);
    UI.slider("Ramp Delta (semitones/s^2)", p.freq_dramp, -127, 127);

    UI.separatorText("Vibrato");
    UI.slider("Vibrato Depth", p.vib_depth, 0, 1);
    UI.slider("Vibrato Frequency (Hz)", p.vib_freq, 0, 20);

    UI.separatorText("Interval (changes base freq)");
    UI.slider("Size (MIDI)", p.arp_mod_midi, -127, 127);
    UI.slider("Onset (ms)", p.arp_time_ms, 0, 1000);

    UI.separatorText("Pulse-Width Modulation");
    UI.slider("PWM Depth", p.pwm_depth, 0, 1);
    UI.slider("PWM Frequency (Hz)", p.pwm_freq, 0, 20);

    UI.separatorText("Delay");
    UI.slider("Feedback Gain", p.feedback_gain, 0, .99);
    UI.slider("Base Delay (ms)", p.delay_base_dur_ms, 0, 100);
    UI.slider("Delay Mod Frequency (Hz)", p.delay_mod_freq, 0, 20);
    UI.slider("Delay Mod Depth", p.delay_mod_depth, 0, 1);

    UI.separatorText("Lowpass Filter");
    UI.slider("LPF Frequency (Hz)", p.lpf_freq, 0, 20000);
    UI.slider("LPF Ramp (semitones/s)", p.lpf_ramp, -127, 127);
    UI.slider("LPF Resonance", p.lpf_resonance, 0, 10);

    UI.separatorText("Highpass Filter");
    UI.slider("HPF Frequency (Hz)", p.hpf_freq, 0, 20000);
    UI.slider("HPF Ramp (semitones/s)", p.hpf_ramp, -127, 127);

    UI.separator();
    UI.endChild();

    // if (UI.button("Revert")) {}
    // UI.sameLine();
    // if (UI.button("Save")) {}
    // UI.endGroup();

    UI.sameLine();

    // Right Pane ---------------------------------------

    UI.pushStyleVar(UI_StyleVar.WindowPadding, @(0, 10));
    UI.beginChild(
        "right pane", @(0, 0), UI_ChildFlags.Border, 0
    );
    // UI.getWindowSize() => vec2 right_size;
    UI.getContentRegionAvail() => vec2 right_size;

    centerNext(UI.calcTextSize("Volume").x);
    UI.text("Volume");
    centerNext(right_size.x * .4);
    UI.vslider("##v", @(right_size.x * .4, right_size.y * .2), p.main_gain, 0, 1);

    centerNext(right_size.x * .8);
    UI.convertHSVtoRGB(@(2.0/7, 0.6, 0.6)) => vec3 g;
    UI.convertHSVtoRGB(@(2.0/7, 0.7, 0.7)) => vec3 g1;
    UI.convertHSVtoRGB(@(2.0/7, 0.8, 0.8)) => vec3 g2;
    UI.pushStyleColor(UI_Color.Button, @(g.x, g.y, g.z, 1));
    UI.pushStyleColor(UI_Color.ButtonHovered, @(g1.x, g1.y, g1.z, 1));
    UI.pushStyleColor(UI_Color.ButtonActive, @(g2.x, g2.y, g2.z, 1));
    UI.button("Play Sound", @(right_size.x * .8, right_size.y * .1));
    UI.popStyleColor(3);

    UI.inputText("##", p.export_wav_path);
    UI.button("Export WAV");




    UI.endChild();
    UI.popStyleVar();

}
UI.end(); // CKFXR
}
} spork ~ ui();

1::eon => now;

/*
Adjustable child windows

UI.BeginChild("left pane", ImVec2(150, 0), ImGuiChildFlags_Border | ImGuiChildFlags_ResizeX);
for (int i = 0; i < 100; i++)
{
    char label[128];
    sprintf(label, "MyObject %d", i);
    if (UI.Selectable(label, selected == i))
        selected = i;
}
UI.EndChild();
UI.SameLine();
....
UI.BeginChild("item view", ....);

*/

/*
fullscreen window

ImGuiViewport* viewport = UI.GetMainViewport();
UI.SetNextWindowPos(viewport->GetWorkPos());
UI.SetNextWindowSize(viewport->GetWorkSize());
UI.SetNextWindowViewport(viewport->ID);

UI.PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
UI.Begin(..., ..., ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize);

[...]

UI.End();
UI.PopStyleVar(2);
*/