#pragma once

#include <opencv2/core.hpp>
#include <opencv2/dnn.hpp>
#include <opencv2/imgproc.hpp>
#include "MediaPipeAnchors.inc"
#include <algorithm>
#include <cmath>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace MediaPipePose
{

inline constexpr double kPi = 3.141592653589793238L;

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

struct PoseResult
{
    cv::Mat bbox;
    cv::Mat landmarks;
    cv::Mat worldLandmarks;
    cv::Mat mask;
    float confidence = 0.f;
    bool valid = false;
};

class PersonDetector
{
public:
    PersonDetector(const std::string& modelPath,
                   float nmsThreshold = 0.3f,
                   float scoreThreshold = 0.5f,
                   int topK = 5000,
                   bool preferCuda = true)
        : modelPath_(modelPath)
        , nmsThreshold_(nmsThreshold)
        , scoreThreshold_(scoreThreshold)
        , topK_(topK)
        , inputSize_(224, 224)
        , preferCuda_(preferCuda)
    {
        net_ = cv::dnn::readNet(modelPath_);
        applyDnnBackend(net_, preferCuda_, usingCuda_);
        anchors_ = getMediapipeAnchor();
    }

    bool isLoaded() const { return !net_.empty(); }
    bool usingCuda() const { return usingCuda_; }
    void disableCuda() { fallbackDnnNetToCpu(net_, usingCuda_); }

    cv::Mat infer(const cv::Mat& srcImage)
    {
        if (srcImage.empty() || !isLoaded()) {
            return {};
        }

        cv::Mat inputBlob;
        cv::Size padBias;
        preprocess(srcImage, inputBlob, padBias);

        std::vector<cv::Mat> outputs;
        dnnForward(net_, usingCuda_, inputBlob, outputs);
        return postprocess(outputs, cv::Size(srcImage.cols, srcImage.rows), padBias);
    }

private:
    void preprocess(const cv::Mat& img, cv::Mat& blob, cv::Size& padBias)
    {
        cv::dnn::Image2BlobParams params;
        params.datalayout = cv::dnn::DNN_LAYOUT_NCHW;
        params.ddepth = CV_32F;
        params.mean = cv::Scalar::all(127.5);
        params.scalefactor = cv::Scalar::all(1.0 / 127.5);
        params.size = inputSize_;
        params.swapRB = true;
        params.paddingmode = cv::dnn::DNN_PMODE_LETTERBOX;

        const double ratio = std::min(
            inputSize_.height / static_cast<double>(img.rows),
            inputSize_.width / static_cast<double>(img.cols));
        padBias = cv::Size(0, 0);
        if (img.rows != inputSize_.height || img.cols != inputSize_.width) {
            const cv::Size ratioSize(
                static_cast<int>(img.cols * ratio),
                static_cast<int>(img.rows * ratio));
            const int padH = inputSize_.height - ratioSize.height;
            const int padW = inputSize_.width - ratioSize.width;
            padBias.width = padW / 2;
            padBias.height = padH / 2;
        }

        blob = cv::dnn::blobFromImageWithParams(img, params);
        padBias.width = static_cast<int>(padBias.width / ratio);
        padBias.height = static_cast<int>(padBias.height / ratio);
    }

    cv::Mat postprocess(const std::vector<cv::Mat>& outputs,
                        const cv::Size& orgSize,
                        const cv::Size& padBias)
    {
        if (outputs.size() < 2) {
            return {};
        }

        cv::Mat boxLandDelta = outputs[0].reshape(outputs[0].size[0], outputs[0].size[1]);
        if (boxLandDelta.type() != CV_32F) {
            boxLandDelta.convertTo(boxLandDelta, CV_32F);
        }
        cv::Mat boxDelta = boxLandDelta.colRange(0, 4);
        cv::Mat landmarkDelta = boxLandDelta.colRange(4, boxLandDelta.cols);

        const int numAnchors = boxLandDelta.rows;
        cv::Mat score = outputs[1].reshape(1, numAnchors);
        if (score.type() != CV_32F) {
            score.convertTo(score, CV_32F);
        }

        const float scale = static_cast<float>(std::max(orgSize.height, orgSize.width));

        cv::Mat maskLow = score < -100.f;
        score.setTo(-100.f, maskLow);
        cv::Mat maskHigh = score > 100.f;
        score.setTo(100.f, maskHigh);

        cv::Mat deno;
        cv::exp(-score, deno);
        cv::divide(1.0, 1.0 + deno, score);

        boxDelta.col(0) = boxDelta.col(0) / inputSize_.width;
        boxDelta.col(1) = boxDelta.col(1) / inputSize_.height;
        boxDelta.col(2) = boxDelta.col(2) / inputSize_.width;
        boxDelta.col(3) = boxDelta.col(3) / inputSize_.height;

        cv::Mat xy1 = (boxDelta.colRange(0, 2) - boxDelta.colRange(2, 4) / 2 + anchors_) * scale;
        cv::Mat xy2 = (boxDelta.colRange(0, 2) + boxDelta.colRange(2, 4) / 2 + anchors_) * scale;
        cv::Mat boxes;
        cv::hconcat(xy1, xy2, boxes);

        std::vector<cv::Rect2d> rotBoxes(static_cast<size_t>(boxes.rows));
        boxes.col(0) = boxes.col(0) - padBias.width;
        boxes.col(1) = boxes.col(1) - padBias.height;
        boxes.col(2) = boxes.col(2) - padBias.width;
        boxes.col(3) = boxes.col(3) - padBias.height;

        for (int i = 0; i < boxes.rows; ++i) {
            rotBoxes[static_cast<size_t>(i)] = cv::Rect2d(
                cv::Point2d(boxes.at<float>(i, 0), boxes.at<float>(i, 1)),
                cv::Point2d(boxes.at<float>(i, 2), boxes.at<float>(i, 3)));
        }

        std::vector<int> keep;
        cv::dnn::NMSBoxes(rotBoxes, score, scoreThreshold_, nmsThreshold_, keep, 1.0f, topK_);
        if (keep.empty()) {
            return {};
        }

        const int nbCols = landmarkDelta.cols + boxes.cols + 1;
        cv::Mat candidates(static_cast<int>(keep.size()), nbCols, CV_32FC1);
        int row = 0;
        for (const int idx : keep) {
            candidates.at<float>(row, nbCols - 1) = score.at<float>(idx, 0);
            boxes.row(idx).copyTo(candidates.row(row).colRange(0, 4));
            for (int k = 0; k < 4; ++k) {
                const int lx = k * 2;
                const int ly = lx + 1;
                candidates.at<float>(row, 4 + lx) =
                    (landmarkDelta.at<float>(idx, lx) / inputSize_.width + anchors_.at<float>(idx, 0)) * scale
                    - padBias.width;
                candidates.at<float>(row, 4 + ly) =
                    (landmarkDelta.at<float>(idx, ly) / inputSize_.height + anchors_.at<float>(idx, 1)) * scale
                    - padBias.height;
            }
            ++row;
        }
        return candidates;
    }

    std::string modelPath_;
    float nmsThreshold_;
    float scoreThreshold_;
    int topK_;
    cv::Size inputSize_;
    bool preferCuda_ = true;
    bool usingCuda_ = false;
    cv::dnn::Net net_;
    cv::Mat anchors_;
};

class PoseEstimator
{
public:
    PoseEstimator(const std::string& modelPath, float confThreshold = 0.5f, bool preferCuda = true)
        : modelPath_(modelPath)
        , confThreshold_(confThreshold)
        , inputSize_(256, 256)
        , personBoxPreEnlargeFactor_(1.f)
        , personBoxEnlargeFactor_(1.25f)
        , preferCuda_(preferCuda)
    {
        net_ = cv::dnn::readNet(modelPath_);
        applyDnnBackend(net_, preferCuda_, usingCuda_);
    }

    bool isLoaded() const { return !net_.empty(); }
    bool usingCuda() const { return usingCuda_; }
    void disableCuda() { fallbackDnnNetToCpu(net_, usingCuda_); }

    std::optional<PoseResult> infer(const cv::Mat& image, const cv::Mat& person)
    {
        if (image.empty() || person.empty() || !isLoaded()) {
            return std::nullopt;
        }

        cv::Mat inputBlob;
        cv::Mat rotatedPersonBox;
        float angle = 0.f;
        cv::Mat rotationMatrix;
        cv::Size padBias;
        if (!preprocess(image, person, inputBlob, rotatedPersonBox, angle, rotationMatrix, padBias)) {
            return std::nullopt;
        }

        std::vector<cv::Mat> outputBlob;
        dnnForward(net_, usingCuda_, inputBlob, outputBlob);

        return postprocess(
            outputBlob,
            rotatedPersonBox,
            angle,
            rotationMatrix,
            padBias,
            cv::Size(image.cols, image.rows));
    }

private:
    bool preprocess(const cv::Mat& image,
                    const cv::Mat& person,
                    cv::Mat& blob,
                    cv::Mat& rotatedPersonBox,
                    float& angle,
                    cv::Mat& rotationMatrix,
                    cv::Size& padBias)
    {
        padBias = cv::Size(0, 0);
        cv::Mat personKeypoints = person.colRange(4, 12).reshape(0, 4);
        cv::Point2f midHipPoint(personKeypoints.at<float>(0, 0), personKeypoints.at<float>(0, 1));
        cv::Point2f fullBodyPoint(personKeypoints.at<float>(1, 0), personKeypoints.at<float>(1, 1));

        const double fullDist = cv::norm(midHipPoint - fullBodyPoint);
        if (fullDist < 1.0) {
            return false;
        }
        cv::Mat fullBoxf;
        {
            std::vector<cv::Mat> vmat = {
                cv::Mat(midHipPoint) - fullDist,
                cv::Mat(midHipPoint) + fullDist};
            cv::hconcat(vmat, fullBoxf);
        }

        cv::Mat centerBox;
        cv::reduce(fullBoxf, centerBox, 1, cv::REDUCE_AVG, CV_32F);
        cv::Mat whBox = fullBoxf.col(1) - fullBoxf.col(0);
        cv::Mat newHalfSize = whBox * personBoxPreEnlargeFactor_ / 2.f;

        cv::Mat fullBox;
        {
            std::vector<cv::Mat> vmat = {centerBox - newHalfSize, centerBox + newHalfSize};
            cv::hconcat(vmat, fullBox);
        }

        cv::Mat personBox;
        fullBox.convertTo(personBox, CV_32S);
        personBox.at<int>(0, 0) = std::clamp(personBox.at<int>(0, 0), 0, image.cols);
        personBox.at<int>(0, 1) = std::clamp(personBox.at<int>(0, 1), 0, image.cols);
        personBox.at<int>(1, 0) = std::clamp(personBox.at<int>(1, 0), 0, image.rows);
        personBox.at<int>(1, 1) = std::clamp(personBox.at<int>(1, 1), 0, image.rows);

        const int cropW = personBox.at<int>(0, 1) - personBox.at<int>(0, 0);
        const int cropH = personBox.at<int>(1, 1) - personBox.at<int>(1, 0);
        if (cropW <= 0 || cropH <= 0) {
            return false;
        }

        cv::Mat cropped = image(cv::Rect(
            personBox.at<int>(0, 0),
            personBox.at<int>(1, 0),
            cropW,
            cropH)).clone();

        int top = personBox.at<int>(1, 0) - static_cast<int>(std::floor(fullBox.at<float>(1, 0)));
        int left = personBox.at<int>(0, 0) - static_cast<int>(std::floor(fullBox.at<float>(0, 0)));
        int bottom = static_cast<int>(std::ceil(fullBox.at<float>(1, 1))) - personBox.at<int>(1, 1);
        int right = static_cast<int>(std::ceil(fullBox.at<float>(0, 1))) - personBox.at<int>(0, 1);
        top = std::max(0, top);
        bottom = std::max(0, bottom);
        left = std::max(0, left);
        right = std::max(0, right);
        cv::copyMakeBorder(cropped, cropped, top, bottom, left, right, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
        padBias.width += personBox.at<int>(0, 0) - left;
        padBias.height += personBox.at<int>(1, 0) - top;

        midHipPoint -= cv::Point2f(static_cast<float>(padBias.width), static_cast<float>(padBias.height));
        fullBodyPoint -= cv::Point2f(static_cast<float>(padBias.width), static_cast<float>(padBias.height));

        float radians = static_cast<float>(
            kPi / 2.0 - std::atan2(-(fullBodyPoint.y - midHipPoint.y), fullBodyPoint.x - midHipPoint.x));
        radians = radians - 2.f * static_cast<float>(kPi)
            * static_cast<float>(static_cast<int>((radians + static_cast<float>(kPi)) / (2.f * static_cast<float>(kPi))));
        angle = radians * 180.f / static_cast<float>(kPi);

        rotationMatrix = cv::getRotationMatrix2D(midHipPoint, angle, 1.0);
        cv::Mat rotatedImage;
        cv::warpAffine(cropped, rotatedImage, rotationMatrix, cv::Size(cropped.cols, cropped.rows));

        cv::dnn::Image2BlobParams params;
        params.datalayout = cv::dnn::DNN_LAYOUT_NHWC;
        params.ddepth = CV_32F;
        params.mean = cv::Scalar::all(0);
        params.scalefactor = cv::Scalar::all(1.0 / 255.0);
        params.size = inputSize_;
        params.swapRB = true;
        params.paddingmode = cv::dnn::DNN_PMODE_NULL;
        blob = cv::dnn::blobFromImageWithParams(rotatedImage, params);

        if (cropped.empty()) {
            return false;
        }

        rotatedPersonBox = (cv::Mat_<float>(2, 2) << 0.f, 0.f,
                            static_cast<float>(cropped.cols),
                            static_cast<float>(cropped.rows));
        return true;
    }

    std::optional<PoseResult> postprocess(const std::vector<cv::Mat>& blob,
                                          const cv::Mat& rotatedPersonBox,
                                          float angle,
                                          const cv::Mat& rotationMatrix,
                                          const cv::Size& padBias,
                                          const cv::Size& imgSize)
    {
        (void)imgSize;
        if (blob.size() < 5) {
            return std::nullopt;
        }

        const float valConf = blob[1].at<float>(0);
        if (valConf < confThreshold_) {
            return std::nullopt;
        }

        cv::Mat landmarks = blob[0].reshape(0, 39);
        cv::Mat heatmap = blob[3];
        cv::Mat landmarksWorld = blob[4].reshape(0, 39);
        (void)heatmap;

        if (landmarks.type() != CV_32F) {
            landmarks.convertTo(landmarks, CV_32F);
        }
        if (landmarksWorld.type() != CV_32F) {
            landmarksWorld.convertTo(landmarksWorld, CV_32F);
        }

        cv::Mat deno;
        cv::exp(-landmarks.colRange(3, landmarks.cols), deno);
        cv::divide(1.0, 1.0 + deno, landmarks.colRange(3, landmarks.cols));

        cv::Mat whRotatedPersonBox = rotatedPersonBox.row(1) - rotatedPersonBox.row(0);
        cv::Mat scaleFactor = whRotatedPersonBox.clone();
        scaleFactor.col(0) /= static_cast<float>(inputSize_.width);
        scaleFactor.col(1) /= static_cast<float>(inputSize_.height);

        landmarks.col(0) = (landmarks.col(0) - inputSize_.width / 2.f) * scaleFactor.at<float>(0);
        landmarks.col(1) = (landmarks.col(1) - inputSize_.height / 2.f) * scaleFactor.at<float>(1);
        landmarks.col(2) = landmarks.col(2) * std::max(scaleFactor.at<float>(1), scaleFactor.at<float>(0));

        cv::Mat coordsRotationMatrix;
        cv::getRotationMatrix2D(cv::Point2f(0.f, 0.f), angle, 1.0).convertTo(coordsRotationMatrix, CV_32F);
        cv::Mat rotatedLandmarks = landmarks.colRange(0, 2) * coordsRotationMatrix.colRange(0, 2);
        cv::hconcat(rotatedLandmarks, landmarks.colRange(2, landmarks.cols), rotatedLandmarks);

        cv::Mat rotatedLandmarksWorld = landmarksWorld.colRange(0, 2) * coordsRotationMatrix.colRange(0, 2);
        cv::hconcat(rotatedLandmarksWorld, landmarksWorld.col(2), rotatedLandmarksWorld);

        cv::Mat rotationMatrix32;
        rotationMatrix.convertTo(rotationMatrix32, CV_32F);
        cv::Mat rotationComponent = (cv::Mat_<float>(2, 2) << rotationMatrix32.at<float>(0, 0),
                                     rotationMatrix32.at<float>(1, 0),
                                     rotationMatrix32.at<float>(0, 1),
                                     rotationMatrix32.at<float>(1, 1));
        cv::Mat translationComponent = rotationMatrix32(cv::Rect(2, 0, 1, 2)).clone();
        cv::Mat invertedTranslation = -rotationComponent * translationComponent;
        cv::Mat inverseRotationMatrix;
        cv::hconcat(rotationComponent, invertedTranslation, inverseRotationMatrix);

        cv::Mat rc;
        cv::reduce(rotatedPersonBox, rc, 0, cv::REDUCE_AVG, CV_32F);
        cv::Mat center = (cv::Mat_<float>(1, 3) << rc.at<float>(0, 0), rc.at<float>(0, 1), 1.f);

        cv::Mat originalCenter(2, 1, CV_32FC1);
        originalCenter.at<float>(0) = center.dot(inverseRotationMatrix.row(0));
        originalCenter.at<float>(1) = center.dot(inverseRotationMatrix.row(1));

        for (int idxRow = 0; idxRow < rotatedLandmarks.rows; ++idxRow) {
            landmarks.at<float>(idxRow, 0) = rotatedLandmarks.at<float>(idxRow, 0)
                + originalCenter.at<float>(0) + static_cast<float>(padBias.width);
            landmarks.at<float>(idxRow, 1) = rotatedLandmarks.at<float>(idxRow, 1)
                + originalCenter.at<float>(1) + static_cast<float>(padBias.height);
        }

        double vmin0 = 0.0;
        double vmax0 = 0.0;
        double vmin1 = 0.0;
        double vmax1 = 0.0;
        cv::minMaxLoc(landmarks.col(0), &vmin0, &vmax0);
        cv::minMaxLoc(landmarks.col(1), &vmin1, &vmax1);
        cv::Mat bbox = (cv::Mat_<float>(2, 2) << static_cast<float>(vmin0), static_cast<float>(vmin1),
                        static_cast<float>(vmax0), static_cast<float>(vmax1));

        cv::Mat centerBox;
        cv::reduce(bbox, centerBox, 0, cv::REDUCE_AVG, CV_32F);
        cv::Mat whBox = bbox.row(1) - bbox.row(0);
        cv::Mat newHalfSize = whBox * personBoxEnlargeFactor_ / 2.f;
        {
            std::vector<cv::Mat> vmat = {centerBox - newHalfSize, centerBox + newHalfSize};
            cv::vconcat(vmat, bbox);
        }

        PoseResult result;
        result.bbox = bbox;
        result.landmarks = landmarks;
        result.worldLandmarks = rotatedLandmarksWorld;
        result.confidence = valConf;
        result.valid = true;
        return result;
    }

    std::string modelPath_;
    float confThreshold_;
    cv::Size inputSize_;
    float personBoxPreEnlargeFactor_;
    float personBoxEnlargeFactor_;
    bool preferCuda_ = true;
    bool usingCuda_ = false;
    cv::dnn::Net net_;
};

inline void drawSkeletonLine(cv::Mat& image,
                             const cv::Mat& landmarks,
                             const cv::Mat& keepLandmarks,
                             int idx1,
                             int idx2,
                             int thickness = 2)
{
    if (keepLandmarks.at<uchar>(idx1) && keepLandmarks.at<uchar>(idx2)) {
        cv::line(
            image,
            cv::Point(static_cast<int>(landmarks.at<float>(idx1, 0)),
                      static_cast<int>(landmarks.at<float>(idx1, 1))),
            cv::Point(static_cast<int>(landmarks.at<float>(idx2, 0)),
                      static_cast<int>(landmarks.at<float>(idx2, 1))),
            cv::Scalar(255, 255, 255),
            thickness);
    }
}

inline void drawPoseOverlay(cv::Mat& image, const PoseResult& pose, float presenceThreshold = 0.8f)
{
    if (!pose.valid || pose.landmarks.empty()) {
        return;
    }

    cv::Mat landmarksScreen = pose.landmarks.rowRange(0, pose.landmarks.rows - 6);
    cv::Mat keepLandmarks = landmarksScreen.col(4) > presenceThreshold;

    static const std::vector<std::pair<int, int>> segments = {
        {0, 1}, {1, 2}, {2, 3}, {3, 7}, {0, 4}, {4, 5}, {5, 6}, {6, 8}, {9, 10},
        {12, 14}, {14, 16}, {16, 22}, {16, 18}, {16, 20}, {18, 20},
        {11, 13}, {13, 15}, {15, 21}, {15, 19}, {15, 17}, {17, 19},
        {11, 12}, {11, 23}, {23, 24}, {24, 12},
        {24, 26}, {26, 28}, {28, 30}, {28, 32}, {30, 32},
        {23, 25}, {25, 27}, {27, 31}, {27, 29}, {29, 31}};

    for (const auto& seg : segments) {
        drawSkeletonLine(image, landmarksScreen, keepLandmarks, seg.first, seg.second);
    }

    if (!pose.bbox.empty()) {
        cv::Mat box;
        pose.bbox.convertTo(box, CV_32S);
        cv::rectangle(
            image,
            cv::Point(box.at<int>(0, 0), box.at<int>(0, 1)),
            cv::Point(box.at<int>(1, 0), box.at<int>(1, 1)),
            cv::Scalar(0, 255, 0),
            2);
    }

    for (int i = 0; i < landmarksScreen.rows; ++i) {
        if (keepLandmarks.at<uchar>(i)) {
            cv::circle(
                image,
                cv::Point(static_cast<int>(landmarksScreen.at<float>(i, 0)),
                          static_cast<int>(landmarksScreen.at<float>(i, 1))),
                2,
                cv::Scalar(0, 0, 255),
                -1);
        }
    }
}

inline std::vector<PoseResult> detectPoses(PersonDetector& personDetector,
                                           PoseEstimator& poseEstimator,
                                           const cv::Mat& image)
{
    std::vector<PoseResult> results;
    const cv::Mat persons = personDetector.infer(image);
    if (persons.empty()) {
        return results;
    }

    for (int i = 0; i < persons.rows; ++i) {
        const cv::Mat person = persons.row(i);
        if (auto pose = poseEstimator.infer(image, person)) {
            results.push_back(*pose);
        }
    }
    return results;
}

} // namespace MediaPipePose
