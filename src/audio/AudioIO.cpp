#include "AudioIO.h"

#include <stdio.h>

#define PA_ERROR_OUTPUT(err) (printf("PortAudio error: %s\n", Pa_GetErrorText(err)))

// Initialize static members
PaStream *AudioIO::_stream = nullptr;
PaError AudioIO::_err = paNoError;
bool AudioIO::_isInitialized = false;

PaStreamParameters AudioIO::_inputParameters = {};
PaStreamParameters AudioIO::_outputParameters = {};

std::thread AudioIO::bufferThread = std::thread();

std::list<AudioIO::BlockData> AudioIO::blockData = std::list<AudioIO::BlockData>();

CircularQueue<AudioIO::StreamData, RING_BUFFER_LENGTH> AudioIO::ringBuffer = CircularQueue<AudioIO::StreamData, RING_BUFFER_LENGTH>();

AudioIO::AudioIO()
{
    if (_isInitialized)
        return;
    _stream = nullptr;
    _err = paNoError;
    _isInitialized = false;
}

int AudioIO::recordCallback(const void *inputBuffer, void *outputBuffer,
                            unsigned long framesPerBuffer,
                            const PaStreamCallbackTimeInfo *timeInfo,
                            PaStreamCallbackFlags statusFlags,
                            void *userData)
{
    CircularQueue<StreamData, RING_BUFFER_LENGTH> *dataBuffer = (CircularQueue<StreamData, RING_BUFFER_LENGTH> *)userData;

    const SAMPLE *inputPtr = (const SAMPLE *)inputBuffer;

    // Record the sample to the array
    StreamData sData;
    if (inputBuffer == nullptr)
    {
        for (int i = 0; i < framesPerBuffer; i++)
        {
            sData.s1 = SAMPLE_SILENCE; // Left
            if (NUM_CHANNELS == 2)
                sData.s2 = SAMPLE_SILENCE; // Right
            dataBuffer->push(sData);
        }
    }
    else
    {
        for (int i = 0; i < framesPerBuffer; i++)
        {
            sData.s1 = *inputPtr++; /* left */
            if (NUM_CHANNELS == 2)
                sData.s2 = *inputPtr++; /* right */
            dataBuffer->push(sData);
        }
    }

    return paContinue;
}

int AudioIO::initialize()
{
    if (_isInitialized)
        return paNoError;
    _err = Pa_Initialize();
    if (_err != paNoError)
    {
        PA_ERROR_OUTPUT(_err);
        return _err;
    }

    // Initialize the input parameters
    _inputParameters.device = _err = Pa_GetDefaultInputDevice(); /* default input device */
    if (_err == paNoDevice)
    {
        PA_ERROR_OUTPUT(_err);
        return _err;
    }
    _inputParameters.channelCount = NUM_CHANNELS;
    _inputParameters.sampleFormat = PA_SAMPLE_TYPE;
    _inputParameters.suggestedLatency = Pa_GetDeviceInfo(_inputParameters.device)->defaultLowInputLatency;
    _inputParameters.hostApiSpecificStreamInfo = nullptr;

    // Initialize the output parameters
    _outputParameters.device = _err = Pa_GetDefaultOutputDevice(); /* default output device */
    if (_err == paNoDevice)
    {
        PA_ERROR_OUTPUT(_err);
        return _err;
    }
    _outputParameters.channelCount = NUM_CHANNELS;
    _outputParameters.sampleFormat = PA_SAMPLE_TYPE;
    _outputParameters.suggestedLatency = Pa_GetDeviceInfo(_outputParameters.device)->defaultLowOutputLatency;
    _outputParameters.hostApiSpecificStreamInfo = nullptr;

    blockData.clear();

    _isInitialized = true;
    return _err;
}

int AudioIO::close()
{
    if (!_isInitialized)
        return paNoError;
    if (_stream != nullptr)
    {
        _err = Pa_CloseStream(_stream);
        if (_err != paNoError)
        {
            PA_ERROR_OUTPUT(_err);
            return _err;
        }
    }

    _err = Pa_Terminate();
    if (_err != paNoError)
    {
        PA_ERROR_OUTPUT(_err);
        return _err;
    }

    _isInitialized = false;

    return _err;
}

int AudioIO::startRecording()
{
    _err = Pa_OpenStream(&_stream, &_inputParameters, nullptr, SAMPLE_RATE, FRAMES_PER_BUFFER, paClipOff, recordCallback, &ringBuffer);
    if (_err != paNoError)
    {
        PA_ERROR_OUTPUT(_err);
        return _err;
    }

    _err = Pa_StartStream(_stream);
    if (_err != paNoError)
    {
        PA_ERROR_OUTPUT(_err);
        return _err;
    }
    bufferThread = std::thread(storeRecording);
    return _err;
}

void AudioIO::storeRecording()
{
    std::cout << "Recording buffer into memory!\n";
    while (Pa_IsStreamActive(_stream))
    {
        StreamData data = {};
        // If there is no data in the ring buffer do nothing
        if (!ringBuffer.pop(data))
        {
            continue;
        }
        // Check if block is full and allocate more memory for it
        if (blockData.empty() || blockData.back().frameIndex == blockData.back().maxFrames)
        {
            blockData.push_back((BlockData){.maxFrames = BLOCK_BUFFER_SIZE, .frameIndex = 0});
        }
        BlockData &blockChunk = blockData.back();
        blockChunk.block[blockChunk.frameIndex*NUM_CHANNELS] = data.s1;
        if (NUM_CHANNELS == 2)
        {
            blockChunk.block[blockChunk.frameIndex*NUM_CHANNELS + 1] = data.s2;
        }
        blockChunk.frameIndex++;
    }
    std::cout << "Stream closed!\n";
    std::cout << "Data chunks used: " << blockData.size() << std::endl;
}

int AudioIO::stopRecording()
{
    _err = Pa_StopStream(_stream);
    if (_err != paNoError)
    {
        PA_ERROR_OUTPUT(_err);
        return _err;
    }
    bufferThread.join();
    return _err;
}

int AudioIO::writeRecording(const char *fname)
{
    FILE * f = fopen(fname, "wb");
    if (f == nullptr) {
        std::cerr << "Could not open file!\n";
        return -1;
    }
    for (BlockData chunk : blockData) {
        if (chunk.frameIndex == chunk.maxFrames) {
            fwrite(chunk.block, sizeof(SAMPLE)*NUM_CHANNELS, chunk.maxFrames, f);
        } else {
            fwrite(chunk.block, sizeof(SAMPLE)*NUM_CHANNELS, chunk.frameIndex, f);
        }
    }
    fclose(f);
    return 0;
}