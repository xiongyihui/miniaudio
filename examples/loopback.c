/*
Demonstrates how to capture data from a microphone using the low-level API.

This example simply captures data from your default microphone until you press Enter. The output is saved to the file
specified on the command line.

Capturing works in a very similar way to playback. The only difference is the direction of data movement. Instead of
the application sending data to the device, the device will send data to the application. This example just writes the
data received by the microphone straight to a WAV file.
*/
#define MINIAUDIO_IMPLEMENTATION
#include "../miniaudio.h"

#include <stdlib.h>
#include <stdio.h>


#define PERIOD_TIME_MS      4

ma_encoder* pEncoder = NULL;
ma_decoder* pDecoder = NULL;

static uint64_t monotonic_ns()
{
  struct timespec monotime;
  clock_gettime(CLOCK_MONOTONIC_RAW, &monotime);
  return monotime.tv_sec * (uint64_t)1e9 + monotime.tv_nsec;
}

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    MA_ASSERT(pEncoder != NULL);

    if (pDecoder == NULL) {
        return;
    }

    ma_encoder_write_pcm_frames(pEncoder, pInput, frameCount, NULL);

    ma_decoder_read_pcm_frames(pDecoder, pOutput, frameCount, NULL);
}

int audio_setup(const char *rec, const char *out)
{
    ma_result result;
    ma_decoder decoder;
    ma_device_config deviceConfig;
    ma_device device;

    ma_encoder_config encoderConfig;
    ma_encoder encoder;

    pDecoder = &decoder;
    pEncoder = &encoder;

    result = ma_decoder_init_file(out, NULL, &decoder);
    if (result != MA_SUCCESS) {
        printf("Could not load file: %s\n", out);
        return -2;
    }

    encoderConfig = ma_encoder_config_init(ma_encoding_format_wav, decoder.outputFormat, decoder.outputChannels, decoder.outputSampleRate);

    if (ma_encoder_init_file(rec, &encoderConfig, &encoder) != MA_SUCCESS) {
        printf("Failed to initialize output file.\n");
        return -1;
    }

    deviceConfig = ma_device_config_init(ma_device_type_duplex);
    deviceConfig.playback.format   = decoder.outputFormat;
    deviceConfig.capture.format    = decoder.outputFormat;
    deviceConfig.playback.channels = decoder.outputChannels;
    deviceConfig.capture.channels  = decoder.outputChannels;
    deviceConfig.sampleRate        = decoder.outputSampleRate;
    deviceConfig.periods           = 1;
    deviceConfig.periodSizeInMilliseconds = PERIOD_TIME_MS;
    deviceConfig.noPreSilencedOutputBuffer = true;
    deviceConfig.dataCallback      = data_callback;
    deviceConfig.pUserData         = &decoder;

    if (ma_device_init(NULL, &deviceConfig, &device) != MA_SUCCESS) {
        printf("Failed to open playback device.\n");
        ma_decoder_uninit(&decoder);
        ma_encoder_uninit(&encoder);
        return -3;
    }

    if (ma_device_start(&device) != MA_SUCCESS) {
        printf("Failed to start playback device.\n");
        ma_device_uninit(&device);
        ma_decoder_uninit(&decoder);
        ma_encoder_uninit(&encoder);
        return -4;
    }

    printf("Press Enter to quit...");
    getchar();

    ma_device_uninit(&device);
    ma_decoder_uninit(&decoder);
    ma_encoder_uninit(&encoder);

    return 0;
}


int main(int argc, char** argv)
{
    if (argc != 3) {
        printf("Usage: %s rec.wav out.wav\n", argv[0]);
        return -1;
    }

    audio_setup(argv[1], argv[2]);

    return 0;
}
