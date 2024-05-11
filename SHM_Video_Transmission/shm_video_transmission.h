#ifndef _SHM_VIDEO_TRANSMISSION_H
#define _SHM_VIDEO_TRANSMISSION_H

#include <iostream>
#include <opencv2/opencv.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/interprocess_sharable_mutex.hpp>
#include <boost/interprocess/sync/sharable_lock.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <chrono>

namespace shm_video_trans
{
    using namespace boost::interprocess;
    using namespace cv;
    using namespace std::chrono;

    struct FrameMetadata
    {
        interprocess_sharable_mutex mutex;
        high_resolution_clock::time_point time_stamp;
        high_resolution_clock::time_point write_time;
        int width = 0, height = 0;
        FrameMetadata() {}
        FrameMetadata(int width_, int height_) : width(width_), height(height_) {}
    };

    struct FrameBag
    {
        high_resolution_clock::time_point time_stamp;
        high_resolution_clock::time_point write_time;
        Mat frame;
        FrameBag() {}
        FrameBag(high_resolution_clock::time_point time_stamp_, high_resolution_clock::time_point write_time_)
            : time_stamp(time_stamp_), write_time(write_time_) {}
        FrameBag(high_resolution_clock::time_point time_stamp_, high_resolution_clock::time_point write_time_, Mat frame_)
            : time_stamp(time_stamp_), write_time(write_time_), frame(frame_) {}
    };

    class VideoReceiver
    {
    private:
        std::string channel_name_;
        std::shared_ptr<shared_memory_object> shm_obj;
        std::shared_ptr<mapped_region> region;
        high_resolution_clock::time_point last_receive_time = high_resolution_clock::time_point();
        FrameBag frame;
        FrameMetadata *metadata;

    public:
        VideoReceiver(std::string channel_name)
        {
            channel_name_ = channel_name;
            init();
        }

        ~VideoReceiver()
        {
        }

        bool init()
        {
            try
            {
                shm_obj = std::make_shared<shared_memory_object>(open_only, channel_name_.c_str(), read_write);
            }
            catch (const std::exception &e)
            {
                shm_obj = nullptr;
                return false;
            }
            region = std::make_shared<mapped_region>(*shm_obj, read_write);
            metadata = reinterpret_cast<FrameMetadata *>(region->get_address());
            frame.frame = cv::Mat(metadata->height, metadata->width, CV_8UC3, static_cast<unsigned char *>(region->get_address()) + sizeof(FrameMetadata));
            return true;
        }

        bool receive()
        {
            if (!(shm_obj && region))
                return false;
            sharable_lock<interprocess_sharable_mutex> lock(metadata->mutex);
            if (!(metadata->write_time > last_receive_time))
                return false;
            frame.time_stamp = metadata->time_stamp;
            frame.write_time = metadata->write_time;
            last_receive_time = metadata->write_time;
            return true;
        }

        FrameBag toCvCopy()
        {
            FrameBag out(frame.time_stamp, frame.write_time, frame.frame.clone());
            return out;
        }

        FrameBag &toCvShare()
        {
            return frame;
        }

        void lock()
        {
            metadata->mutex.lock_sharable();
        }

        void unlock()
        {
            metadata->mutex.unlock_sharable();
        }
    };

    class VideoSender
    {
    private:
        int video_width_, video_height_, video_size_;
        std::string channel_name_;
        std::shared_ptr<shared_memory_object> shm_obj;
        std::shared_ptr<mapped_region> region;
        FrameMetadata *metadata;
        int timeout_period = 1000;
        bool auto_remove = true;

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
            new (region->get_address()) FrameMetadata(video_width_, video_height_);
            metadata = reinterpret_cast<FrameMetadata *>(region->get_address());
        }

        ~VideoSender()
        {
            if (auto_remove)
                release_channel();
        }

        void disableAutoRemove()
        {
            auto_remove = false;
        }

        void enableAutoRemove()
        {
            auto_remove = true;
        }

        void setTimeoutPeriod(int t)
        {
            timeout_period = t;
        }

        void release_channel()
        {
            shm_obj->remove(channel_name_.c_str());
        }

        void send(cv::Mat &frame, std::chrono::high_resolution_clock::time_point time_stamp = std::chrono::high_resolution_clock::time_point())
        {
            if (frame.cols != video_width_ || frame.rows != video_height_)
                cv::resize(frame, frame, cv::Size(video_width_, video_height_));
            if (!metadata->mutex.timed_lock(boost::posix_time::microsec_clock::universal_time() + boost::posix_time::millisec(timeout_period)))
            {
                new (region->get_address()) FrameMetadata(video_width_, video_height_);
                return;
            }
            std::memcpy(static_cast<unsigned char *>(region->get_address()) + sizeof(FrameMetadata), frame.data, video_size_);
            metadata->time_stamp = time_stamp;
            metadata->write_time = high_resolution_clock::now();
            metadata->mutex.unlock();
        }

        void sendInUnsafeMode(cv::Mat &frame, std::chrono::high_resolution_clock::time_point time_stamp = std::chrono::high_resolution_clock::time_point())
        {
            if (frame.cols != video_width_ || frame.rows != video_height_)
                cv::resize(frame, frame, cv::Size(video_width_, video_height_));
            std::memcpy(static_cast<unsigned char *>(region->get_address()) + sizeof(FrameMetadata), frame.data, video_size_);
            metadata->time_stamp = time_stamp;
            metadata->write_time = high_resolution_clock::now();
        }

        void sendInReceiverFirstMode(cv::Mat &frame, std::chrono::high_resolution_clock::time_point time_stamp = std::chrono::high_resolution_clock::time_point())
        {
            if (frame.cols != video_width_ || frame.rows != video_height_)
                cv::resize(frame, frame, cv::Size(video_width_, video_height_));
            if (!metadata->mutex.try_lock())
                return;
            std::memcpy(static_cast<unsigned char *>(region->get_address()) + sizeof(FrameMetadata), frame.data, video_size_);
            metadata->time_stamp = time_stamp;
            metadata->write_time = high_resolution_clock::now();
            metadata->mutex.unlock();
        }
    };
} // namespace shm_video_trans

#endif // _SHM_VIDEO_TRANSMISSION_H
