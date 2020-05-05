#include "Audio.h"
#include "synth.h"

#include <string>
#include <sstream>
#include <algorithm> // std::max
#include <cmath>
#include <float.h> // std::max

//playback implemenation based on zetpo 8's
//https://github.com/samhocevar/zepto8/blob/master/src/pico8/sfx.cpp

Audio::Audio(){
    for(int i = 0; i < 4; i++) {
        _sfxChannels[i].sfxId = -1;
    }
}

void Audio::setMusic(std::string musicString){
    std::istringstream s(musicString);
    std::string line;
    char buf[3] = {0};
    int musicIdx = 0;
    
    while (std::getline(s, line)) {
        buf[0] = line[0];
        buf[1] = line[1];
        uint8_t flagByte = (uint8_t)strtol(buf, NULL, 16);

        buf[0] = line[3];
        buf[1] = line[4];
        uint8_t channel1byte = (uint8_t)strtol(buf, NULL, 16);

        buf[0] = line[5];
        buf[1] = line[6];
        uint8_t channel2byte = (uint8_t)strtol(buf, NULL, 16);

        buf[0] = line[7];
        buf[1] = line[8];
        uint8_t channel3byte = (uint8_t)strtol(buf, NULL, 16);

        buf[0] = line[9];
        buf[1] = line[10];
        uint8_t channel4byte = (uint8_t)strtol(buf, NULL, 16);

        _songs[musicIdx++] = {
            flagByte,
            channel1byte,
            channel2byte,
            channel3byte,
            channel4byte
        };
    }

}

void Audio::setSfx(std::string sfxString) {
    std::istringstream s(sfxString);
    std::string line;
    char buf[3] = {0};
    int sfxIdx = 0;
    
    while (std::getline(s, line)) {
        buf[0] = line[0];
        buf[1] = line[1];
        uint8_t editorMode = (uint8_t)strtol(buf, NULL, 16);

        buf[0] = line[2];
        buf[1] = line[3];
        uint8_t noteDuration = (uint8_t)strtol(buf, NULL, 16);

        buf[0] = line[4];
        buf[1] = line[5];
        uint8_t loopRangeStart = (uint8_t)strtol(buf, NULL, 16);

        buf[0] = line[6];
        buf[1] = line[7];
        uint8_t loopRangeEnd = (uint8_t)strtol(buf, NULL, 16);

        _sfx[sfxIdx].editorMode = editorMode;
        _sfx[sfxIdx].speed = noteDuration;
        _sfx[sfxIdx].loopRangeStart = loopRangeStart;
        _sfx[sfxIdx].loopRangeEnd = loopRangeEnd;

        //32 notes, 5 chars each
        int noteIdx = 0;
        for (int i = 8; i < 168; i+=5) {
            buf[0] = line[i];
            buf[1] = line[i + 1];
            uint8_t key = (uint8_t)strtol(buf, NULL, 16);

            buf[0] = '0';
            buf[1] = line[i + 2];
            uint8_t waveform = (uint8_t)strtol(buf, NULL, 16);

            buf[0] = '0';
            buf[1] = line[i + 3];
            uint8_t volume = (uint8_t)strtol(buf, NULL, 16);

            buf[0] = '0';
            buf[1] = line[i + 4];
            uint8_t effect = (uint8_t)strtol(buf, NULL, 16);

            _sfx[sfxIdx].notes[noteIdx++] = {
                key,
                waveform,
                volume,
                effect
            };
        }

        sfxIdx++;
    } 
}

void Audio::api_sfx(uint8_t sfx, int channel, int offset){

    if (sfx < -2 || sfx > 63 || channel < -1 || channel > 3 || offset > 31) {
        return;
    }

    if (sfx == -1)
    {
        // Stop playing the current channel
        if (channel != -1) {
            _sfxChannels[channel].sfxId = -1;
        }
    }
    else if (sfx == -2)
    {
        // Stop looping the current channel
        if (channel != -1) {
            _sfxChannels[channel].can_loop = false;
        }
    }
    else
    {
        // Find the first available channel: either a channel that plays
        // nothing, or a channel that is already playing this sample (in
        // this case PICO-8 decides to forcibly reuse that channel, which
        // is reasonable)
        if (channel == -1)
        {
            for (int i = 0; i < 4; ++i)
                if (_sfxChannels[i].sfxId == -1 ||
                    _sfxChannels[i].sfxId == sfx)
                {
                    channel = i;
                    break;
                }
        }

        // If still no channel found, the PICO-8 strategy seems to be to
        // stop the sample with the lowest ID currently playing
        if (channel == -1)
        {
            for (int i = 0; i < 4; ++i) {
               if (channel == -1 || _sfxChannels[i].sfxId < _sfxChannels[channel].sfxId) {
                   channel = i;
               }
            }
        }

        // Stop any channel playing the same sfx
        for (int i = 0; i < 4; ++i) {
            if (_sfxChannels[i].sfxId == sfx) {
                _sfxChannels[i].sfxId = -1;
            }
        }

        // Play this sound!
        _sfxChannels[channel].sfxId = sfx;
        _sfxChannels[channel].offset = std::max(0.f, (float)offset);
        _sfxChannels[channel].phi = 0.f;
        _sfxChannels[channel].can_loop = true;
        _sfxChannels[channel].is_music = false;
        // Playing an instrument starting with the note C-2 and the
        // slide effect causes no noticeable pitch variation in PICO-8,
        // so I assume this is the default value for “previous key”.
        _sfxChannels[channel].prev_key = 24;
        // There is no default value for “previous volume”.
        _sfxChannels[channel].prev_vol = 0.f;
    }      
}

void Audio::api_music(uint8_t pattern, int16_t fade_len, int16_t mask){
    if (pattern < 0 || pattern > 63) {
        return;
    }

    if (pattern == -1)
    {
        // Music will stop when fade out is finished
        _musicChannel.volume_step = fade_len <= 0 ? -FLT_MAX
                                  : -_musicChannel.volume * (1000.f / fade_len);
        return;
    }

    _musicChannel.count = 0;
    _musicChannel.mask = mask ? mask & 0xf : 0xf;

    _musicChannel.volume = 1.f;
    _musicChannel.volume_step = 0.f;
    if (fade_len > 0)
    {
        _musicChannel.volume = 0.f;
        _musicChannel.volume_step = 1000.f / fade_len;
    }

    _musicChannel.pattern = pattern;
    _musicChannel.offset = 0;

    // Find music speed; it’s the speed of the fastest sfx
    _musicChannel.master = _musicChannel.speed = -1;
    uint8_t channels[] = {
        _songs[pattern].channel1,
        _songs[pattern].channel2,
        _songs[pattern].channel3,
        _songs[pattern].channel4,
    };

    for (int i = 0; i < 4; ++i)
    {
        //last bit is a flag not used here
        uint8_t n = channels[i] & 0x0F;

        auto &sfx = _sfx[n & 0x3f];
        if (_musicChannel.master == -1 || _musicChannel.speed > sfx.speed)
        {
            _musicChannel.master = i;
            _musicChannel.speed = std::max(1, (int)sfx.speed);
        }
    }

    // Play music sfx on active channels
    for (int i = 0; i < 4; ++i)
    {
        if (((1 << i) & _musicChannel.mask) == 0)
            continue;

        //last bit is a flag not used here
        uint8_t n = channels[i] & 0x0F;;

        _sfxChannels[i].sfxId = n;
        _sfxChannels[i].offset = 0.f;
        _sfxChannels[i].phi = 0.f;
        _sfxChannels[i].can_loop = false;
        _sfxChannels[i].is_music = true;
        _sfxChannels[i].prev_key = 24;
        _sfxChannels[i].prev_vol = 0.f;
    }
}

void Audio::FillAudioBuffer(void *audioBuffer, size_t offset, size_t size){

    uint32_t *buffer = (uint32_t *)audioBuffer;

    for (size_t i = 0; i < size; ++i){
        int16_t sample = 0;

        for (int c = 0; c < 4; ++c) {
            sample += this->getSampleForChannel(c);
        }

        //buffer is stereo, so just send the mono sample to both channels
        buffer[i] = (sample<<16) | (sample & 0xffff);
    }
}

static float key_to_freq(float key)
{
    using std::exp2;
    return 440.f * exp2((key - 33.f) / 12.f);
}

//adapted from zepto8 sfx.cpp (wtfpl license)
int16_t Audio::getSampleForChannel(int channel){
    using std::fabs, std::fmod, std::floor, std::max;

    int const samples_per_second = 22050;

    int16_t sample = 0;

    const int index = _sfxChannels[channel].sfxId;

    if (index < 0 || index > 63) {
        //no (valid) sfx here. return silence
        return 0;
    }

    struct sfx const &sfx = _sfx[index];

    // Speed must be 1—255 otherwise the SFX is invalid
    int const speed = max(1, (int)sfx.speed);

    float const offset = _sfxChannels[channel].offset;
    float const phi = _sfxChannels[channel].phi;

    // PICO-8 exports instruments as 22050 Hz WAV files with 183 samples
    // per speed unit per note, so this is how much we should advance
    float const offset_per_second = 22050.f / (183.f * speed);
    float const offset_per_sample = offset_per_second / samples_per_second;
    float next_offset = offset + offset_per_sample;

    // Handle SFX loops. From the documentation: “Looping is turned
    // off when the start index >= end index”.
    float const loop_range = float(sfx.loopRangeEnd - sfx.loopRangeStart);
    if (loop_range > 0.f && next_offset >= sfx.loopRangeStart && _sfxChannels[channel].can_loop) {
        next_offset = fmod(next_offset - sfx.loopRangeStart, loop_range)
                    + sfx.loopRangeStart;
    }

    int const note_idx = (int)floor(offset);
    int const next_note_idx = (int)floor(next_offset);

    uint8_t key = sfx.notes[note_idx].key;
    float volume = sfx.notes[note_idx].volume / 7.f;
    float freq = key_to_freq(key);

    if (volume == 0.f){
        //volume all the way off. return silence, but make sure to set stuff
        _sfxChannels[channel].offset = next_offset;

        if (next_offset >= 32.f){
            _sfxChannels[channel].sfxId = -1;
        }
        else if (next_note_idx != note_idx){
            _sfxChannels[channel].prev_key = sfx.notes[note_idx].key;
            _sfxChannels[channel].prev_vol = sfx.notes[note_idx].volume / 7.f;
        }

        return 0;
    }
    
    //TODO: apply effects
    //int const fx = sfx.notes[note_id].effect;

    // Play note
    float waveform = z8::synth::waveform(sfx.notes[note_idx].waveform, phi);

    // Apply master music volume from fade in/out
    // FIXME: check whether this should be done after distortion
    //if (m_state.channels[chan].is_music) {
    //    volume *= m_state.music.volume;
    //}

    sample = (int16_t)(32767.99f * volume * waveform);

    // TODO: Apply hardware effects
    //if (m_ram.hw_state.distort & (1 << chan)) {
    //    sample = sample / 0x1000 * 0x1249;
    //}

    _sfxChannels[channel].phi = phi + freq / samples_per_second;

    _sfxChannels[channel].offset = next_offset;

    if (next_offset >= 32.f){
        _sfxChannels[channel].sfxId = -1;
    }
    else if (next_note_idx != note_idx){
        _sfxChannels[channel].prev_key = sfx.notes[note_idx].key;
        _sfxChannels[channel].prev_vol = sfx.notes[note_idx].volume / 7.f;
    }

    return sample;
}