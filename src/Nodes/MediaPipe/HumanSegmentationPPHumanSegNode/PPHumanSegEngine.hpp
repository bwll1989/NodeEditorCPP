#pragma once

#include <opencv2/core.hpp>
#include <opencv2/dnn.hpp>
#include <opencv2/imgproc.hpp>
#include <string>
#include <utility>
#include <vector>

namespace PPHumanSeg
{

inline void applyDnnBackend(cv::dnn::Net& net, bool preferCuda, bool& usingCuda)
{
    if (preferCuda) {
        net.setPreferableBackend(cv::dnn::DNN_BACKEND_CUDA);
        net.setPreferableTarget(cv::dnn::DNN_TARGET_CUDA);
        usingCuda = true;
        return;
    }
    net.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
    net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
    usingCuda = false;
}

inline void fallbackDnnNetToCpu(cv::dnn::Net& net, bool& usingCuda)
{
    if (!usingCuda) {
        return;
    }
    usingCuda = false;
    net.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
    net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
}

inline void dnnForward(cv::dnn::Net& net,
                       bool& usingCuda,
                       const cv::Mat& inputBlob,
                       std::vector<cv::Mat>& outputs)
{
    const std::vector<cv::String> outNames = net.getUnconnectedOutLayersNames();
    try {
        net.setInput(inputBlob);
        net.forward(outputs, outNames);
    } catch (const cv::Exception&) {
        if (!usingCuda) {
            throw;
        }
        fallbackDnnNetToCpu(net, usingCuda);
        net.setInput(inputBlob);
        net.forward(outputs, outNames);
    }
}

struct SegmentationResult
{
    cv::Mat foreground;
    cv::Mat background;
    cv::Mat mask;
    bool valid = false;
};

class PPHumanSegModel
{
public:
    explicit PPHumanSegModel(const std::string& modelPath, bool preferCuda = true)
        : modelPath_(modelPath)
        , inputSize_(192, 192)
        , preferCuda_(preferCuda)
    {
        net_ = cv::dnn::readNet(modelPath_);
        applyDnnBackend(net_, preferCuda_, usingCuda_);
    }

    bool isLoaded() const { return !net_.empty(); }
    bool usingCuda() const { return usingCuda_; }
    void disableCuda() { fallbackDnnNetToCpu(net_, usingCuda_); }

    cv::Mat inferMask(const cv::Mat& srcImage)
    {
        if (srcImage.empty() || !isLoaded()) {
            return {};
        }

        cv::Mat inputBlob;
        preprocess(srcImage, inputBlob);

        std::vector<cv::Mat> outputs;
        dnnForward(net_, usingCuda_, inputBlob, outputs);
        return postprocessMask(outputs, cv::Size(srcImage.cols, srcImage.rows));
    }

    static SegmentationResult splitForegroundBackground(const cv::Mat& srcImage, const cv::Mat& mask)
    {
        SegmentationResult result;
        if (srcImage.empty() || mask.empty()) {
            return result;
        }

        cv::Mat personMask;
        cv::compare(mask, 1, personMask, cv::CMP_EQ);

        result.foreground = cv::Mat::zeros(srcImage.size(), srcImage.type());
        srcImage.copyTo(result.foreground, personMask);

        cv::Mat bgMask;
        cv::bitwise_not(personMask, bgMask);
        result.background = cv::Mat::zeros(srcImage.size(), srcImage.type());
        srcImage.copyTo(result.background, bgMask);

        result.mask = std::move(personMask);
        result.valid = true;
        return result;
    }

    SegmentationResult infer(const cv::Mat& srcImage)
    {
        const cv::Mat mask = inferMask(srcImage);
        return splitForegroundBackground(srcImage, mask);
    }

private:
    void preprocess(const cv::Mat& img, cv::Mat& blob)
    {
        cv::Mat rgb;
        cv::cvtColor(img, rgb, cv::COLOR_BGR2RGB);
        cv::Mat resized;
        cv::resize(rgb, resized, inputSize_, 0, 0, cv::INTER_LINEAR);
        resized.convertTo(resized, CV_32F, 1.0 / 255.0);
        resized -= cv::Scalar(0.5, 0.5, 0.5);
        resized /= cv::Scalar(0.5, 0.5, 0.5);
        blob = cv::dnn::blobFromImage(resized);
    }

    static cv::Mat postprocessMask(const std::vector<cv::Mat>& outputs, const cv::Size& orgSize)
    {
        if (outputs.empty() || outputs[0].dims != 4) {
            return {};
        }

        const cv::Mat& output = outputs[0];
        const int numClasses = output.size[1];
        const int height = output.size[2];
        const int width = output.size[3];
        if (numClasses < 2 || height <= 0 || width <= 0) {
            return {};
        }

        const float* basePtr = output.ptr<float>();
        const int planeSize = height * width;
        std::vector<cv::Mat> resizedChannels(static_cast<size_t>(numClasses));
        for (int c = 0; c < numClasses; ++c) {
            cv::Mat channel(height, width, CV_32F, const_cast<float*>(basePtr + c * planeSize));
            cv::resize(channel, resizedChannels[static_cast<size_t>(c)], orgSize, 0, 0, cv::INTER_LINEAR);
        }

        cv::Mat mask(orgSize, CV_8UC1);
        for (int y = 0; y < orgSize.height; ++y) {
            auto* row = mask.ptr<uchar>(y);
            for (int x = 0; x < orgSize.width; ++x) {
                int bestClass = 0;
                float bestScore = resizedChannels[0].at<float>(y, x);
                for (int c = 1; c < numClasses; ++c) {
                    const float score = resizedChannels[static_cast<size_t>(c)].at<float>(y, x);
                    if (score > bestScore) {
                        bestScore = score;
                        bestClass = c;
                    }
                }
                row[x] = static_cast<uchar>(bestClass);
            }
        }
        return mask;
    }

    std::string modelPath_;
    cv::dnn::Net net_;
    cv::Size inputSize_;
    bool preferCuda_ = true;
    bool usingCuda_ = false;
};

} // namespace PPHumanSeg
