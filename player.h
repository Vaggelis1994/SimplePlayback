#pragma once

#include "networkReader.h"
#define BUFFER_SIZE 4096

/**
 * Implement the player.
 *
 * ATTENTION: API must be async. No function is supposed to block!
 *
 */
class Player {

private:

    //variables for mixing
    double networkLevel;
    double playerLevel;

    net::StopWatch stopWatch;
    net::NetworkReader nr;

    //output streams
    std::ofstream sink;
    std::ofstream stats;

    //buffers
    char *playerBuffer;
    char *networkBuffer;
    char *mix;

    //requested bytes for buffers
    size_t playerBytes;
    size_t networkBytes;

    //represents number of samples currently being streamed
    int writtenSamples;

    std::vector<int16_t> m_saw;
    size_t m_sawIndex;

    //variables for pause method
    std::chrono::milliseconds timePaused;
    int pausedSample;
    bool paused;

public:
    Player() : m_sawIndex(0){}
    virtual ~Player() {}

    /**
     * Open the player and prepare it so it can start playing whenever play() is called.
     *
     * @param networkUrl URL to the network stream
     * @param filename filename used as input for the filesource
     * @warning args are not used
     */
    void open(char *networkUrl, char *filename) {

        char sinkFile[] = "audio_output.raw";
        char statsFile[] = "realtime_stats.txt";

        sink.open(sinkFile, std::ios::binary | std::ios::trunc);
        stats.open(statsFile, std::ios::trunc);

        int16_t samples = 72; //good compromise ;-)

        //Network Buffer for streaming
        networkBytes = samples * sizeof(int16_t);
        networkBuffer = new char[networkBytes];

        //Player Buffer for streaming
        playerBytes = samples * sizeof(int16_t);
        playerBuffer = new char[playerBytes];

        init(); //read the data from filename

        setMixingLevel(0); //compromise for default value
    }

    /**
     * Close the player
     */
    void close() {

        sink.close(); //closing audio output stream
        if(!sink) std::cerr<<"An error occurred when closing the audio output file."<<std::endl;

        stats.close(); //closing realtime stats file
        if(!stats) std::cerr<<"An error occurred when closing the stats file."<<std::endl;

    }

    /**
     * Start or resume playback from position where pause() was called
     */
    void play() {

        //what is actually read
        size_t playerRead;
        size_t networkRead;

        paused = false;

        networkRead = nr.read(&(*networkBuffer), networkBytes); //stream from network
        playerRead = read(playerBuffer, playerBytes); //stream from player
        mix = new char[2* (networkRead + playerRead)]; //mixing buffer -stereo

        //number of samples currently streaming
        writtenSamples += (int16_t) ((playerRead/sizeof(int16_t)) + (networkRead/sizeof(int16_t)));

        //mixing process -case for different byte read from each source
        int pos = 0;
        for (int i = 0; i < networkRead; ++i) {
            auto bit = networkLevel * (double) networkBuffer[i];
            mix[pos] = (char)bit; pos++; //copy the bit
            mix[pos] = (char)bit; pos++; //make it stereo
        }
        pos = 0;
        for (int i = 0; i < playerRead; ++i) {
            auto bit = playerLevel * (double) playerBuffer[i];
            mix[pos] += (char)bit; pos++; //compute weighted sum
            mix[pos] += (char)bit; pos++; //make it stereo
        }

        //output stats
        stats << stopWatch.elapsed<std::chrono::milliseconds>().count() << ", " << writtenSamples << std::endl;
        stats.flush();

        //output stream
        sink.write(mix, 2*(networkRead + playerRead));
        sink.flush();

        //till all the data from sources have been streamed
        if (playerRead == 0 && networkRead == 0) return;

        if(!paused) play(); //resume

    }

    /**
     * Pause playback at the current position
     */
    void pause() {

        paused = true;

        //pinpoint the time and number of samples when stream was paused
        timePaused = stopWatch.elapsed<std::chrono::milliseconds>();
        pausedSample = writtenSamples;

    }

    /**
     * Sets the mixing level.
     * -1 means only network source
     * 0 means 50% network source, 50% filesource
     * 1 means only filesource
     * @param level mixing level with range [-1..1]
     */
    void setMixingLevel(double level) {

        //adjusting level value in case is out of the appropriate range
        level = (level <= -1.0) ? -1.0 : level;
        level = (level >= 1.0) ? 1.0 : level;

        //set mixing levels at their weighted sum
        networkLevel = (1.0 - level) / 2;
        playerLevel = (1.0 + level) / 2;

    }

private:

    void init () {

        std::ifstream is("audio1_s16le_mono_48k.raw", std::ios::binary);
        if (is) {

            // get length of file:
            is.seekg(0, is.end);
            int length = is.tellg();
            is.seekg(0, is.beg);

            m_saw.resize(length / sizeof(int16_t));
            is.read ((char*)m_saw.data(),length);

            if (is.fail()) std::cerr<<"error: only "<<is.gcount()<<" could be read"<<std::endl;

            is.close();
        }
        else {
            std::cerr<<"Player reader: Couldn't open input file!"<<std::endl;
            exit(1);
        }

    }

    /**
     * Stream from file -equivelant process of network stream
     * The sample format being read is 48kHz, S16LE, mono.
     *
     * @note This function will block until all requested bytes or the maximum available bytes have been read.
     *
     * @param buf destination buffer for the data
     * @param maxBytes size of the destination buffer
     * @return number of bytes actually read
     *
     * @credits not me!
     */
    size_t read (char* buf, size_t maxBytes) {

        const size_t blockSize = 8192 * 4;
        size_t samplesRemaining = m_saw.size() - m_sawIndex;

        if (samplesRemaining == 0) return 0; //EOS

        size_t maxReadSize = std::min(std::min(maxBytes, blockSize), samplesRemaining * sizeof(int16_t));
        auto nSamples = maxReadSize / sizeof(int16_t);
        std::copy_n(m_saw.begin() + m_sawIndex, nSamples, (int16_t*)buf);
        m_sawIndex += nSamples;

        return nSamples * sizeof(int16_t);

    }



};