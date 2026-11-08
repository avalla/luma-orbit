#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
namespace orbit {
// Disk writes run on a background thread. The callback never waits for file I/O.
class Recorder {
public:
    Recorder()=default;
    ~Recorder(){stop();thread.stopThread(5000);}
    bool start(const juce::File& file,double rate,int channels){
        if(!std::isfinite(rate)||rate<=0||channels<1||channels>2)return false;
        stop();if(!thread.isThreadRunning()&&!thread.startThread())return false;juce::TemporaryFile temporary(file);
        std::unique_ptr<juce::OutputStream> stream=temporary.getFile().createOutputStream();
        if(!stream)return false;
        juce::WavAudioFormat format;
        auto formatWriter=format.createWriterFor(stream,juce::AudioFormatWriterOptions().withSampleRate(rate).withNumChannels(channels).withBitsPerSample(16));
        if(!formatWriter)return false;
        formatWriter.reset();
        if(!temporary.overwriteTargetFileWithTemporary())return false;
        // Reopen only after a valid destination has been established.
        auto output=file.createOutputStream();if(!output)return false;output->setPosition(0);output->truncate();
        stream=std::move(output);formatWriter=format.createWriterFor(stream,juce::AudioFormatWriterOptions().withSampleRate(rate).withNumChannels(channels).withBitsPerSample(16));
        if(!formatWriter)return false;
        auto next=std::make_unique<juce::AudioFormatWriter::ThreadedWriter>(formatWriter.release(),thread,65536);
        const juce::SpinLock::ScopedLockType lock(mutex);writer=std::move(next);remaining=int64_t(rate*300);sampleRate=rate;frames=0;failed=false;active=true;return true;
    }
    void stop(){
        std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> old;
        {const juce::SpinLock::ScopedLockType lock(mutex);active=false;old=std::move(writer);}
        old.reset(); // Flush outside the callback lock.
    }
    void push(const juce::AudioBuffer<float>& audio){
        if(!active.load())return;
        const juce::SpinLock::ScopedTryLockType lock(mutex);
        if(!lock.isLocked()){failed=true;active=false;return;}
        if(!writer||!active.load())return;
        const int size=int(std::min<int64_t>(remaining,audio.getNumSamples()));
        if(!writer->write(audio.getArrayOfReadPointers(),size)){failed=true;active=false;return;}
        frames+=size;remaining-=size;if(remaining<=0)active=false;
    }
    double seconds()const{return double(frames.load())/sampleRate.load();}
    std::atomic<bool> active{false},failed{false};
private:
    juce::TimeSliceThread thread{"Orbit WAV"};juce::SpinLock mutex;
    std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> writer;
    std::atomic<int64_t> frames{0};std::atomic<double> sampleRate{44100};int64_t remaining=0;
};
}
