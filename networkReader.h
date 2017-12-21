#pragma once

#include <cmath>
#include <chrono>
#include <random>
#include <algorithm>
#include <fstream>
#include <thread>
#include <iostream>

namespace net {

    /**
     * Convenience class for time measurement
     */
    class StopWatch {
    public:
        StopWatch() : m_start(Clock::now()) {}
        template <class TimeUnit>
        TimeUnit elapsed() const {
            auto now = Clock::now();
            return std::chrono::duration_cast<TimeUnit>(now - m_start);
        }
    private:
        using Clock = std::chrono::high_resolution_clock;
        std::chrono::high_resolution_clock::time_point m_start;
    };

    class NetworkReader {
    public:
        /**
         * Network read simulator
         * On construction, a transfer rate profile is randomly generated.
         * Normally there is no need to specify a seed value other than for debugging purposes
         *
         * @note You are not supposed to modify this file unless you find a bug! ;-)
         *
         * @param seed optional PRNG seed to reproduce transfer speed profile curves
         */
        NetworkReader(int64_t seed = -1) : m_gen(m_rd()), m_sawIndex(0) {
            if (seed >= 0) {
                m_gen.seed(seed);
            }
            m_maxTime = std::chrono::seconds(100);
            initProfileCurve(m_maxTime);
            initWaveform();
        }

        /**
         * Reads a chunk of data from a simulated network.
         * Chunksize is limited to 327768 bytes.
         * The sample format being read is 48kHz, S16LE, mono.
         *
         * @note This function will block until all requested bytes or the maximum available bytes have been read.
         *
         * @param buf destination buffer for the data
         * @param maxBytes size of the destination buffer
         * @return number of bytes actually read
         */
        size_t read(char *buf, size_t maxBytes) {
            const size_t blockSize = 8192 * 4;
            auto t = m_clock.elapsed<std::chrono::milliseconds>();
            auto bps = getProfileValueAt(t);

            size_t samplesRemaining = m_saw.size() - m_sawIndex;
            if (samplesRemaining == 0) { // EOS
                return 0;
            }

            size_t maxReadSize = std::min(std::min(maxBytes, blockSize), samplesRemaining * sizeof(int16_t));
            int64_t dt = (int64_t)(1000 * maxReadSize / bps);

            std::this_thread::sleep_for(std::chrono::milliseconds(dt));

            auto nSamples = maxReadSize / sizeof(int16_t);
            std::copy_n(m_saw.begin() + m_sawIndex, nSamples, (int16_t*)buf);
            m_sawIndex += nSamples;
            return nSamples * sizeof(int16_t);
        }

    private:
        void initProfileCurve(std::chrono::milliseconds maxTime) {
            auto meanRate = 48000 * sizeof(int16_t); // 96kB/s -> 768kbps
            std::uniform_real_distribution<> dis(meanRate * 0.7, meanRate * 1.4);
            m_profile.resize(maxTime.count() / 1000);
            std::generate_n(m_profile.begin(), maxTime.count() / 1000, [&]{ return dis(m_gen); });
        }

        void initWaveform() {
            std::ifstream is("audio2_s16le_mono_48k.raw", std::ios::binary);
            if (is) {
                // get length of file:
                is.seekg(0, is.end);
                int length = is.tellg();
                is.seekg(0, is.beg);

                m_saw.resize(length / sizeof(int16_t));
                is.read ((char*)m_saw.data(),length);

                if (is.fail())
                    std::cout << "error: only " << is.gcount() << " could be read\n";

                is.close();
            } else {
                std::cout << "NetworkReader: Couldn't open input file!\n";
                exit(1);
            }
        }

        void dumpProfile(const std::string& filename, std::chrono::milliseconds dt) {
            std::ofstream out(filename, std::ios::trunc);
            std::chrono::milliseconds t{0};
            int32_t val = 0;
            out << "millisecond, bytesPerSecond\n";
            while(t < m_maxTime) {
                val = getProfileValueAt(t);
                out << t.count() << ',' << val << '\n';
                t += dt;
            }
            out.flush();
        }
        void dumpWaveform(const std::string& filename) {
            std::ofstream out(filename, std::ios::trunc);
            int64_t t = 0;
            for(auto val : m_saw) {
                out << t++ << ',' << val << '\n';
            }
            out.flush();
        }

        double getProfileValueAt(std::chrono::milliseconds time) {
            auto t = std::chrono::milliseconds(time.count() % m_maxTime.count());
            auto sec = std::chrono::duration_cast<std::chrono::seconds>(t).count();
            if (sec < 0) {
                return 0;
            }

            using FpSeconds = std::chrono::duration<double, std::chrono::seconds::period>;
            auto dx = std::chrono::duration_cast<FpSeconds>(time).count() - sec;
            return cosineInterpolate(m_profile[sec], m_profile[sec + 1], dx);
        }

        double cosineInterpolate(double y1, double y2, double mu) {
            double mu2 = (1 - cos(mu * M_PI)) / 2;
            return (y1 * (1 - mu2) + y2 * mu2);
        }

        std::default_random_engine generator;
        std::vector<double> m_profile;
        std::chrono::milliseconds m_maxTime;
        StopWatch m_clock;
        std::random_device m_rd;
        std::mt19937 m_gen;
        int64_t m_samplingRate;
        std::vector<int16_t> m_saw;
        size_t m_sawIndex;
    };
}