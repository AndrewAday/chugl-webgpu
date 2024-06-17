/*
name: ckfxr 
desc: ChucK/ChuGL port of DrPetter's sfxr (https://www.drpetter.se/project_sfxr.html)
author: Andrew Zhu Aday (https://ccrma.stanford.edu/~azaday/)
date: June 2024
*/

/*
*/

/*
Audio Graph
Design so that the ckfxr synth is reusable, *without* UI controls

[sin|saw|square|noise] -> LP Filter --> HP Filter --> Phaser --> ADSR



*/

// UGen for delay-based effects including flange, chorus, doubling
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

// while (1::second => now)
// {
//     // spork ~ ckfxr.pickupCoin();
//     spork ~ ckfxr.shootLaser();
// }

fun void ui()
{
    while (1) {
        GG.nextFrame() => now;
        if (UI.begin("CKFXR", null, 0)) {

        }
        UI.end();
    }
} spork ~ ui();

1::eon => now;