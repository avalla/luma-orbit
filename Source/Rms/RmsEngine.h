#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace luma {
// A sample-accurate sliding RMS window. Only prepare() allocates memory.
class RmsEngine {
public:
    struct Reading {
        std::array<double,2> meanSquare{};
        std::array<float,2> peak{};
        double fill=0;
    };
    void prepare(double rate) {
        sampleRate=rate;
        for(auto& channel : history) channel.resize(size_t(std::ceil(rate)));
        setWindow(0.4); reset();
    }
    void setWindow(double seconds) {
        const auto next=std::clamp(size_t(std::llround(sampleRate*std::clamp(seconds,0.1,1.0))),size_t(1),history[0].size());
        if(next!=window) { window=next; reset(); }
    }
    void reset() noexcept { position=filled=0; sums={}; }
    Reading process(const float* const* input,int channels,int samples) noexcept {
        Reading result;
        if(channels<1 || channels>2 || window==0) return result;
        for(int i=0;i<samples;++i) {
            for(int channel=0;channel<channels;++channel) {
                const auto ch=size_t(channel);
                // Non-finite input is ignored by the measurement, never rewritten in the audio.
                const auto sample=std::isfinite(input[channel][i])?input[channel][i]:0.0f;
                const double square=double(sample)*double(sample);
                if(filled==window) sums[ch]-=history[ch][position];
                history[ch][position]=square; sums[ch]+=square;
                result.peak[ch]=std::max(result.peak[ch],std::abs(sample));
            }
            filled=std::min(filled+1,window);
            if(++position==window) position=0;
        }
        for(int ch=0;ch<channels;++ch) result.meanSquare[size_t(ch)]=filled>0?std::max(0.0,sums[size_t(ch)]/double(filled)):0;
        result.fill=double(filled)/double(window);
        return result;
    }
private:
    std::array<std::vector<double>,2> history;
    std::array<double,2> sums{};
    double sampleRate=44100;
    size_t position=0,filled=0,window=0;
};
} // namespace luma
