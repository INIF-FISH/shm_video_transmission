#include "./SHM_Video_Transmission/shm_video_transmission.h"
#include <thread>
#include <iomanip> // For std::fixed and std::setprecision

int main(int argc, char *argv[])
{
    // Check if all required arguments are provided
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <channelName>\n";
        return 1;
    }

    // Extract command line arguments
    std::string channelName = argv[1];

    // Create a VideoReceiver object
    shm_video_trans::VideoReceiver receiver(channelName);

    while (!receiver.init())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Create a window to display the received video
    cv::namedWindow("Received Video", cv::WINDOW_AUTOSIZE);

    // Create a FrameBag object to hold received frames
    shm_video_trans::FrameBag receivedFrame;

    auto start_time = std::chrono::steady_clock::now();
    auto end_time = std::chrono::steady_clock::now();
    double FPS = 0.0; // Change to double for precision
    int frame_count = 0;
    unsigned long transmission_latency = 0;
    unsigned long system_latency = 0;
    unsigned long avg_transmission_latency = 0;
    unsigned long avg_system_latency = 0;
    cv::namedWindow("Received Video", cv::WindowFlags::WINDOW_NORMAL);

    while (true)
    {
        // Receive a frame
        if (receiver.receive())
        {
            auto now_time = std::chrono::steady_clock::now();
            receiver.lock();
            receivedFrame = receiver.toCvShare();
            auto transmission_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(now_time - receivedFrame.write_time);
            auto system_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(now_time - receivedFrame.time_stamp);
            frame_count++;
            transmission_latency += transmission_duration.count();
            system_latency += system_duration.count();
            end_time = std::chrono::steady_clock::now();
            auto elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
            // FPS && Latency
            if (elapsed_milliseconds >= 1000)
            {
                FPS = static_cast<double>(frame_count * 1000) / static_cast<double>(elapsed_milliseconds);
                avg_transmission_latency = transmission_latency / frame_count;
                avg_system_latency = system_latency / frame_count;
                frame_count = 0;
                transmission_latency = 0;
                system_latency = 0;
                start_time = std::chrono::steady_clock::now();
            }
            cv::putText(receivedFrame.frame, "FPS              : " + std::to_string(FPS), cv::Point(10, 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 255), 1);
            cv::putText(receivedFrame.frame, "TransLatency(ns) : " + std::to_string(avg_transmission_latency), cv::Point(10, 40), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 255), 1);
            cv::putText(receivedFrame.frame, "SysLatency(ns)   : " + std::to_string(avg_system_latency), cv::Point(10, 60), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 255), 1);
            // Display the received frame
            cv::imshow("Received Video", receivedFrame.frame);
            receiver.unlock();

            // Press 'ESC' to quit
            if (cv::waitKey(1) == 27)
                break;
        }
    }

    return 0;
}
