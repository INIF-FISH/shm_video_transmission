#include "./SHM_Video_Transmission/shm_video_transmission.h"
#include <thread>
#include <csignal>
#include <chrono>

bool terminate_flag = false;

void signalHandler(int signum)
{
    switch (signum)
    {
    case SIGINT:
        terminate_flag = true;
        break;
    default:
        break;
    }
}

int main(int argc, char *argv[])
{
    signal(SIGINT, signalHandler);
    // Check if all required arguments are provided
    if (argc < 6)
    {
        std::cerr << "Usage: " << argv[0] << " <channelName> <width> <height> <videoFilePath> <realFrameRate>[true|false]\n";
        return 1;
    }

    // Extract command line arguments
    std::string channelName = argv[1];
    int width = std::stoi(argv[2]);
    int height = std::stoi(argv[3]);
    std::string videoFile = argv[4];
    bool realFrameRate = false;
    if (strcmp(argv[5], "true") == 0)
    {
        realFrameRate = true;
    }
    else if (strcmp(argv[5], "false") != 0)
    {
        printf("Invalid argument: %s\n", argv[1]);
        return 1;
    }

    // Create a VideoSender object
    shm_video_trans::VideoSender sender(channelName, width, height);

    // Open the video file
    cv::VideoCapture cap(videoFile);
    if (!cap.isOpened())
    {
        std::cerr << "Error: Unable to open video file\n";
        return 1;
    }

    double fps = cap.get(cv::CAP_PROP_FPS);
    int wait_nanoseconds = std::round(1000000000.f / fps);

    cv::Mat frame;
    while (!terminate_flag)
    {
        // Loop back to the beginning of the video file when reaching the end
        if (cap.get(cv::CAP_PROP_POS_FRAMES) == cap.get(cv::CAP_PROP_FRAME_COUNT))
            cap.set(cv::CAP_PROP_POS_FRAMES, 0);

        std::chrono::system_clock::time_point start_time = std::chrono::system_clock::now();
        // Capture a frame from the video file
        cap >> frame;

        if (frame.empty())
            continue;

        // Send the frame
        sender.send(frame, start_time);

        // Calculate time taken for this iteration
        auto end_time = std::chrono::system_clock::now();
        auto elapsed_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();

        // Calculate the remaining time to wait
        long long remaining_time = wait_nanoseconds - elapsed_time;
        if (remaining_time > 0 && realFrameRate)
            std::this_thread::sleep_for(std::chrono::nanoseconds(remaining_time));
    }

    return 0;
}
