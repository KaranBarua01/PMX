#include <array>
#include <filesystem>
#include <fstream>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include "recording/QuickRecorder.h"

namespace fs = std::filesystem;

static bool hasRiffHeader(const fs::path& path)
{
    std::ifstream in(path, std::ios::binary);
    char bytes[12]{};
    in.read(bytes, sizeof(bytes));
    return in.gcount()==12 && std::string(bytes,4)=="RIFF" && std::string(bytes+8,4)=="WAVE";
}

int main()
{
    const auto dir=fs::temp_directory_path()/"pmx_quick_recorder_test";
    fs::remove_all(dir); fs::create_directories(dir);
    const auto take=dir/"take.wav";

    pmx::recording::QuickRecorder recorder;
    recorder.prepare(1000.0, 2, 4.0); // 4k-frame nonblocking ring for test
    auto started=recorder.start(take);
    if(!started.ok) return 1;

    std::array<float,100> left{},right{};
    for(int i=0;i<100;++i){left[i]=0.1f;right[i]=-0.1f;}
    for(int block=0;block<20;++block)
        if(!recorder.push(left.data(),right.data(),static_cast<int>(left.size()))) return 2;
    if(recorder.framesAccepted()!=2000) return 3;

    auto stopped=recorder.stop();
    if(!stopped.ok || !fs::exists(take) || fs::file_size(take)<=44 || !hasRiffHeader(take)) return 4;
    if(recorder.isRecording()) return 5;

    const auto interrupted=dir/"interrupted.wav";
    if(!recorder.start(interrupted).ok) return 6;
    if(!recorder.push(left.data(),right.data(),100)) return 7;
    const auto recovery=recorder.interruptForRecovery();
    if(!recovery.ok || !fs::exists(interrupted) || !hasRiffHeader(interrupted) || fs::file_size(interrupted)<=44) return 8;

    const auto bad=recorder.start(dir/"missing-parent"/"x"/"take.wav");
    if(bad.ok || bad.error.empty()) return 9;

    // Stop may race an in-flight producer; every accepted sample must reach the WAV.
    recorder.prepare(48000,2,120);
    const auto racing=dir/"concurrent-stop.wav";
    if(!recorder.start(racing).ok) return 10;
    recorder.push(left.data(),right.data(),100);
    std::vector<float> longBlock(4000000,0.1f);
    std::atomic<bool> entered{false};
    std::thread producer([&]{entered.store(true);recorder.push(longBlock.data(),longBlock.data(),4000000);});
    while(!entered.load()) std::this_thread::yield();
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::microseconds(100);
    while(std::chrono::steady_clock::now()<deadline) {}
    recorder.stop();
    producer.join();
    if(fs::file_size(racing)!=44+recorder.framesAccepted()*6) return 11;
    fs::remove_all(dir);
    return 0;
}
