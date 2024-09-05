#ifndef AUDIO_IO_HPP
#define AUDIO_IO_HPP

#include <portaudio.h>
#include "../utility/CircularQueue.h"
#include <list>
#include <thread>

typedef float SAMPLE;

#define PA_SAMPLE_TYPE paFloat32;

#ifndef SAMPLE_SILENCE
#define SAMPLE_SILENCE (0.0f)
#endif

#ifndef SAMPLE_RATE
#define SAMPLE_RATE (44100) // Sample rate of audio
#endif
#ifndef FRAMES_PER_BUFFER
#define FRAMES_PER_BUFFER (512) // Frames per buffer in PortAudio's stream callback
#endif
#ifndef NUM_CHANNELS
#define NUM_CHANNELS (2) // Number of channels used for recording and playback
#endif

#ifndef RING_BUFFER_TIME
#undef RING_BUFFER_LENGTH
#undef BLOCK_BUFFER_SIZE
#define RING_BUFFER_TIME (1)                                // Lenght of ring-buffer in seconds
#define RING_BUFFER_LENGTH (RING_BUFFER_TIME * SAMPLE_RATE) // Ring-buffer size
#define BLOCK_BUFFER_SIZE (RING_BUFFER_LENGTH * 5)          // Size of block buffer memory in heap memory
#endif

class AudioIO
{
public:
    // Stores the incomming stream data for both left and right channels
    typedef struct _StreamData
    {
        SAMPLE s1;
        SAMPLE s2;
    } StreamData;

    typedef struct _BlockData
    {
        unsigned long maxFrames = BLOCK_BUFFER_SIZE;
        unsigned long frameIndex = 0;
        SAMPLE *block;
    } BlockData;

    AudioIO();

    static int initialize();
    static int close();

    /**
     * Starts recording from default input device and saves the RAW audio to
     * a buffer list in memory.
     *
     * Returns
     *
     * `paNoError` on success and a non-zero value on failure.
     */
    static int startRecording();

    /**
     * Stops recording
     *
     * Returns
     *
     * `paNoError` on success and a non-zero value on failure.
     */
    static int stopRecording();

    /**
     * Writes the recording to a RAW audio file.
     *
     * Returns
     *
     * `0` on success and `-1` on failure.
     */
    static int writeRecording(const char *);

    /**
     * Clears the recording buffer.
     *
     * Returns
     *
     * `0` on success and `-1` on failure.
     */
    static int clearRecordingBuffer();

    static int playbackRecording();
    static int stopPlayback();

private:
    static PaStreamParameters _inputParameters,
        _outputParameters;
    static PaStream *_stream;
    static PaError _err;

    static CircularQueue<StreamData, RING_BUFFER_LENGTH> ringBuffer;

    static std::list<BlockData> blockData;

    static int recordCallback(const void *inputBuffer, void *outputBuffer,
                              unsigned long framesPerBuffer,
                              const PaStreamCallbackTimeInfo *timeInfo,
                              PaStreamCallbackFlags statusFlags,
                              void *userData);

    static int playCallback(const void *inputBuffer, void *outputBuffer,
                            unsigned long framesPerBuffer,
                            const PaStreamCallbackTimeInfo *timeInfo,
                            PaStreamCallbackFlags statusFlags,
                            void *userData);

    // Loads the recording from the ring-buffer into larger memory blocks
    static void storeRecording();

    static bool _isInitialized;

    static std::thread bufferThread;
};

#endif // AUDIO_IO_HPP
