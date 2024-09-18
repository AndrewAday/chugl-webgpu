//-----------------------------------------------------------------------------
// name: soundbulb.ck
// desc: soundBulb in ChuGL!
// 
// author: Kunwoo Kim
//         Andrew Zhu Aday (https://ccrma.stanford.edu/~azaday/)
// date: Fall 2024
//-----------------------------------------------------------------------------

// window size
1024 => int WINDOW_SIZE;
// y position of waveform
0 => float WAVEFORM_Y;
// y position of spectrum
0 => float SPECTRUM_Y;
// width of waveform and spectrum display
1.35 => float DISPLAY_WIDTH;
// waterfall depth
128 => int WATERFALL_DEPTH;
// waveform interpolation
0.1 => float WAVEFORM_LERP;
// spectrum radius
1.75 => float SPECTRUM_RADIUS;
// spectrum width
0.01 => float WATERFALL_INCREMENT;

// window title
GWindow.title( "soundbulb" );
// uncomment to fullscreen
GWindow.fullscreen();

// // put camera on a dolly
// GG.camera() --> GGen dolly --> GG.scene();
// GG.camera().orthographic();
// // position
// GG.camera().posZ( 10 );
// // set clipping plane
// GG.camera().clip( .05, WATERFALL_DEPTH * 10 );
// // set bg color
// GG.scene().backgroundColor( @(0,0,0) );

// waveform renderer
GLines waveform --> GG.scene(); waveform.width(0.005);
// translate up
waveform.posY( WAVEFORM_Y );
// color0
waveform.color( @(1, 1, 1)*1.5 );

// make a waterfall
Waterfall waterfall --> GG.scene();
// translate down
waterfall.posY( SPECTRUM_Y );

// which input?
adc => Gain input;
// SinOsc sine => Gain input => dac; .15 => sine.gain;
// accumulate samples from mic
input => Flip accum => blackhole;
// take the FFT
input => PoleZero dcbloke => FFT fft => blackhole;
// set DC blocker
.95 => dcbloke.blockZero;
// set size of flip
WINDOW_SIZE => accum.size;
// set window type and size
Windowing.hann(WINDOW_SIZE) => fft.window;
// set FFT size (will automatically zero pad)
WINDOW_SIZE*2 => fft.size;
// get a reference for our window for visual tapering of the waveform
Windowing.hann(WINDOW_SIZE) @=> float window[];

// sample array
float samples[0];
// FFT response
complex response[0];
// a vector to hold positions
vec2 positions[WINDOW_SIZE];

// previous waveform array
float prev_samples[WINDOW_SIZE];
// current waveform array
float curr_samples[WINDOW_SIZE];

// map audio buffer to 3D positions
fun void map2waveform( float in[], vec2 out[] )
{
    if( in.size() != out.size() )
    {
        <<< "size mismatch in map2waveform()", "" >>>;
        return;
    }
    
    DISPLAY_WIDTH => float width;
    for( 0 => int i; i < in.size(); i++)
    {
        prev_samples[i] + (in[i] - prev_samples[i]) * WAVEFORM_LERP => curr_samples[i];
        
        // space evenly in X
        -width/2 + width/WINDOW_SIZE*i => out[i].x;
        // map y, using window function to taper the ends
        curr_samples[i] * Math.sqrt(window[i] * 5.0) => out[i].y;
        
        curr_samples[i] => prev_samples[i];
    }
}

// map FFT output to 3D positions
fun void map2spectrum( complex in[], vec2 out[] )
{
    vec2 currentPos[WINDOW_SIZE];
    if( in.size() != out.size() )
    {
        <<< "size mismatch in map2spectrum()", "" >>>;
        return;
    }
    
    // mapping to xyz coordinate
    DISPLAY_WIDTH => float width;
    for( 0 => int i; i < WINDOW_SIZE; i++ )
    {
        // space evenly in X
        -width/2 + width/WINDOW_SIZE*i => currentPos[i].x;
        // map frequency bin magnitide in Y        
        Math.sqrt( (in[i]$polar).mag ) => currentPos[i].y;
        
        // change to polar
        SPECTRUM_RADIUS * Math.cos(40 * Math.PI * i / (WINDOW_SIZE - 1)) => out[i].x;
        (SPECTRUM_RADIUS + currentPos[i].y) * Math.sin(40 * Math.PI * i / (WINDOW_SIZE - 1)) => out[i].y;
    }

    waterfall.latest( out );
}

// custom GGen to render waterfall
class Waterfall extends GGen
{
    // waterfall playhead
    0 => int playhead;
    // lines
    GLines wfl[WATERFALL_DEPTH];
    // color
    @(1, 1, 1) => vec3 color;

    // iterate over line GGens
    for( GLines w : wfl )
    {
        // aww yea, connect as a child of this GGen
        w --> this;
        // color
        w.color( @(1, 1, 1) );
        // width
        w.width( 0.005 );
    }

    // copy
    fun void latest( vec2 positions[] )
    {
        // set into
        positions => wfl[playhead].positions;
        // advance playhead
        playhead++;
        // wrap it
        WATERFALL_DEPTH %=> playhead;
    }

    fun void update( float dt )
    {
        // position
        playhead => int pos;
        // so good
        for( int i; i < wfl.size(); i++ )
        {
            // start with playhead-1 and go backwards
            pos--; if( pos < 0 ) WATERFALL_DEPTH-1 => pos;
            // offset X and Y
            wfl[pos].scaX(1.0 + i * WATERFALL_INCREMENT);
            wfl[pos].scaY(1.0 + i * WATERFALL_INCREMENT);
            
            wfl[pos].color(Math.pow(1.0 * (wfl.size() - i)/wfl.size(), 1) * color);
        }
    }
}

// do audio stuff
fun void doAudio()
{
    while( true )
    {
        // upchuck to process accum
        accum.upchuck();
        // get the last window size samples (waveform)
        accum.output( samples );
        // upchuck to take FFT, get magnitude reposne
        fft.upchuck();
        // get spectrum (as complex values)
        fft.spectrum( response );
        // jump by samples
        WINDOW_SIZE::samp/2 => now;
    }
}
spork ~ doAudio();

// **************** GRAPHICAL THINGS **************** //

// Segments per line
200 => int numSegments;
// ------- LightBulb Glass Outline ------- //
3 => int numOutline;
1.0 => float outlineRadius;
GLines glassOutline[numOutline];
vec2 glassOutlinePos[numSegments];
for (auto s : glassOutline)
{
    s --> GG.scene(); s.width(0.005);
}

fun void createOutline()
{
    for (0 => int j; j < numOutline; j++)
    {
        for (0 => int i; i < numSegments; i++)
        {
            1.65 * Math.PI * ((1.0 * i) / (numSegments - 1)) + 0.675 * Math.PI => float angle;
            outlineRadius * Math.cos(angle - Math.PI) + (0.01 * (1 - j)) => glassOutlinePos[i].x;
            1.1 * outlineRadius * Math.sin(angle - Math.PI + (0.02 * (1 - j))) => glassOutlinePos[i].y;
        }
        glassOutline[j].positions(glassOutlinePos);
    }
}
spork ~createOutline();

// ------- LightBulb Filament Holder ------- //
2 => int numHolder;
0.62 => float FH_WIDTH_RATIO;
200 => int numFHSegments;
vec2 FH_startPos[numHolder];
vec2 FH_endPos[numHolder];

GLines filamentHolder[numHolder];
vec2 FH_Pos[numFHSegments];
for (auto s : filamentHolder)
{
    s --> GG.scene(); s.width(0.005);
}

fun void createHolder()
{
    for (0 => int j; j < numHolder; j++)
    {
        -DISPLAY_WIDTH / 2.0 + DISPLAY_WIDTH * j => FH_startPos[j].x;
        WAVEFORM_Y => FH_startPos[j].y;
        
        FH_WIDTH_RATIO * FH_startPos[j].x => FH_endPos[j].x;
        WAVEFORM_Y - (DISPLAY_WIDTH) => FH_endPos[j].y;
        
        for (0 => int i; i < numFHSegments; i++)
        {
            FH_startPos[j].x + (FH_endPos[j].x - FH_startPos[j].x) * (1.0 * i / (numSegments - 1)) => FH_Pos[i].x;
            FH_startPos[j].y + (FH_endPos[j].y - FH_startPos[j].y) * (1.0 * i / (numSegments - 1)) => FH_Pos[i].y;
        }
        filamentHolder[j].positions(FH_Pos);
    }
}
spork ~createHolder();

// ------- LightBulb Base ------- //
GLines lightBulbBase --> GG.scene(); lightBulbBase.width(0.001);
150 => int numBaseSegments;
vec2 base_Pos[numBaseSegments];
vec2 base_startPos;
FH_WIDTH_RATIO * DISPLAY_WIDTH => float BASE_WIDTH;

100 => float baseFreq;
-0.002 => float baseYOffset;

fun void createBase()
{
    (FH_endPos[0].x + FH_endPos[1].x) / 2.0 => base_startPos.x;
    FH_endPos[0].y => base_startPos.y;
    for (0 => int i; i < numBaseSegments; i++)
    {
        base_startPos.x + (BASE_WIDTH * (1.0 - i * 0.002)) / 2.0 * Math.cos(Math.PI / 180.0 * baseFreq * i) => base_Pos[i].x;
        FH_endPos[0].y + baseYOffset * i => base_Pos[i].y;
    }
    lightBulbBase.positions(base_Pos);
}
spork ~createBase();


// ***************** graphics render loop ***************** //
while( true )
{
    // map to interleaved format
    map2waveform( samples, positions );
    // set the mesh position
    waveform.positions( positions );
    // map to spectrum display
    map2spectrum( response, positions );
    // next graphics frame
    GG.nextFrame() => now;
}