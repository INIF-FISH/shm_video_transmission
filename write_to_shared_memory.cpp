#include <iostream>
#include <opencv2/opencv.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <chrono>

using namespace boost::interprocess;
using namespace cv;
using namespace std::chrono;

struct FrameMetadata {
    high_resolution_clock::time_point write_time;
    bool is_consumed = false;
};

int main() {
    const std::string video_file = "1.mp4";
    const int video_width = 640;
    const int video_height = 480;
    const int video_size = video_width * video_height * 3; // Assuming RGB format

    // Open video file
    VideoCapture cap(video_file);
    if (!cap.isOpened()) {
        std::cerr << "Error: Cannot open video file" << std::endl;
        return 1;
    }

    // Create shared memory object
    shared_memory_object shm_obj(open_or_create, "video_memory", read_write);

    // Set shared memory size
    shm_obj.truncate(video_size + sizeof(FrameMetadata));

    // Map shared memory to current process address space
    mapped_region region(shm_obj, read_write);

    // Main loop: Read frames from video file and write to shared memory
    Mat frame;
    while (true) {
        // Time frame is written to shared memory
        cap >> frame;
        if (frame.empty())
            break;

        // Resize frame if necessary
        resize(frame, frame, Size(video_width, video_height));

        auto write_time = high_resolution_clock::now();
        // Copy frame data to shared memory
        std::memcpy(region.get_address(), frame.data, video_size);

        // Write write_time to shared memory
        FrameMetadata *metadata = static_cast<FrameMetadata *>(region.get_address() + video_size);
        metadata->write_time = write_time;
        metadata->is_consumed = false;
    }

    return 0;
}
