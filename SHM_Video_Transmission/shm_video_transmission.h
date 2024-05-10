#ifndef _SHM_VIDEO_TRANSMISSION_H
#define _SHM_VIDEO_TRANSMISSION_H

#include <iostream>
#include <opencv2/opencv.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <chrono>

namespace shm_video_trans
{
    using namespace boost::interprocess;
    using namespace cv;
    using namespace std::chrono;

    struct FrameMetadata
    {
        std::chrono::high_resolution_clock::time_point write_time;
        bool is_consumed = false;
    };

    struct FrameBag
    {
        FrameMetadata meta;
        Mat frame;
    };

    class VideoReceiver
    {
    private:
        int video_width_, video_height_, video_size_;
        std::string channel_name_;
        std::shared_ptr<shared_memory_object> shm_obj;
        std::shared_ptr<mapped_region> region;

    public:
        VideoReceiver(std::string channel_name, const int video_width, const int video_height)
        {
            video_width_ = video_width;
            video_height_ = video_height;
            video_size_ = video_width * video_height * 3;
            channel_name_ = channel_name;
            shm_obj = std::make_shared<shared_memory_object>(open_or_create, channel_name_.c_str(), read_write);
            region = std::make_shared<mapped_region>(*shm_obj, read_write);
        }

        ~VideoReceiver()
        {
            shm_obj->remove(channel_name_.c_str());
        }

        bool receive(FrameBag &out)
        {
            FrameMetadata *metadata = static_cast<FrameMetadata *>(region->get_address() + video_size_);
            if (metadata->is_consumed)
                return false;
            auto write_time = metadata->write_time;
            out.frame = Mat(video_height_, video_width_, CV_8UC3);
            std::memcpy(out.frame.data, region->get_address(), video_size_);
            out.meta = *metadata;
            metadata->is_consumed = true;
            return true;
        }
    };

    class VideoSender
    {
    private:
        int video_width_, video_height_, video_size_;
        std::string channel_name_;
        std::shared_ptr<shared_memory_object> shm_obj;
        std::shared_ptr<mapped_region> region;

    public:
        VideoSender(std::string channel_name, const int video_width, const int video_height)
        {
            video_width_ = video_width;
            video_height_ = video_height;
            video_size_ = video_width * video_height * 3;
            channel_name_ = channel_name;
            shm_obj = std::make_shared<shared_memory_object>(open_or_create, channel_name_.c_str(), read_write);
            shm_obj->truncate(video_size_ + sizeof(FrameMetadata));
            region = std::make_shared<mapped_region>(*shm_obj, read_write);
        }

        ~VideoSender()
        {
            shm_obj->remove(channel_name_.c_str());
        }

        void send(Mat &frame)
        {
            if (frame.cols != video_width_ || frame.rows != video_height_)
                resize(frame, frame, Size(video_width_, video_height_));
            std::memcpy(region->get_address(), frame.data, video_size_);
            FrameMetadata *metadata = static_cast<FrameMetadata *>(region->get_address() + video_size_);
            metadata->write_time = high_resolution_clock::now();
            metadata->is_consumed = false;
        }
    };
} // namespace shm_video_trans

#endif // _SHM_VIDEO_TRANSMISSION_H