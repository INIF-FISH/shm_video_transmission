#include "./SHM_Video_Transmission/shm_video_transmission.h"

int main(int argc, char *argv[])
{
    // Check if all required arguments are provided
    if (argc < 5)
    {
        std::cerr << "Usage: " << argv[0] << " <channelName> <width> <height> <videoFilePath>\n";
        return 1;
    }

    // Extract command line arguments
    std::string channelName = argv[1];
    int width = std::stoi(argv[2]);
    int height = std::stoi(argv[3]);
    std::string videoFile = argv[4];

    // Create a VideoSender object
    shm_video_trans::VideoSender sender(channelName, width, height);

    // Open the video file
    cv::VideoCapture cap(videoFile);
    if (!cap.isOpened())
    {
        std::cerr << "Error: Unable to open video file\n";
        return 1;
    }

    cv::Mat frame;
    while (true)
    {
        // Loop back to the beginning of the video file when reaching the end
        if (cap.get(cv::CAP_PROP_POS_FRAMES) == cap.get(cv::CAP_PROP_FRAME_COUNT))
            cap.set(cv::CAP_PROP_POS_FRAMES, 0);

        // Capture a frame from the video file
        cap >> frame;
        if (frame.empty())
            continue;

        // Send the frame
        sender.send(frame);
    }

    return 0;
}
