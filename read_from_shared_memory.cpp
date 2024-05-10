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
    const int video_width = 640;
    const int video_height = 480;
    const int video_size = video_width * video_height * 3; // Assuming RGB format

    // Create shared memory object
    shared_memory_object shm_obj(open_or_create, "video_memory", read_write);

    // Map shared memory to current process address space
    mapped_region region(shm_obj, read_write);

    // Create window for displaying received frames
    namedWindow("Received Video", WINDOW_AUTOSIZE);

    // Variables for frame count and timing
    
    auto start_time = high_resolution_clock::now();
    auto end_time = high_resolution_clock::now();

    // Main loop: Read frames from shared memory and display
    Mat frame(video_height, video_width, CV_8UC3);
    int FPS = 0;
    int frame_count = 0;
    unsigned long  transmission_latency = 0;
    unsigned long avg_transmission_latency = 0;
    while (true) {
        auto read_time = high_resolution_clock::now(); // Time frame is read from shared memory
        FrameMetadata *metadata = static_cast<FrameMetadata *>(region.get_address() + video_size);
        auto write_time = metadata->write_time;

        if(metadata->is_consumed)
            continue;

        // Copy data from shared memory to OpenCV Mat
        std::memcpy(frame.data, region.get_address(), video_size);

        metadata->is_consumed = true;

        auto duration = duration_cast<nanoseconds>(read_time - write_time); // Calculate transmission latency

        // Print transmission latency
        std::cout << "Transmission latency: " << duration.count() << " ns" << std::endl;

        // Increment frame count
        frame_count++;
        transmission_latency += duration.count();

        // Check if 1 second has passed
        end_time = high_resolution_clock::now();
        auto elapsed_seconds = duration_cast<seconds>(end_time - start_time).count();
        if (elapsed_seconds >= 1) {
            // Print frame count
            FPS = frame_count;
            avg_transmission_latency = transmission_latency / frame_count;
            // Reset frame count and timing
            frame_count = 0;
            transmission_latency = 0;
            start_time = high_resolution_clock::now();
        }
        putText(frame, "FPS: " + std::to_string(FPS), Point(10, 30), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 255, 0), 2);
        putText(frame, "Latency(ns): " + std::to_string(avg_transmission_latency), Point(10, 60), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 255, 0), 2);
        // Display frame
        imshow("Received Video", frame);

        // Check for key press to exit
        if (waitKey(1) == 27) // ESC key
            break;
    }

    // Destroy window
    destroyAllWindows();

    return 0;
}
