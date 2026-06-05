#pragma once

#include <opencv2/core.hpp>
#include <opencv2/dnn.hpp>
#include <opencv2/imgproc.hpp>
#include "PalmAnchors.inc"
#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstring>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace MediaPipeHand
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

inline cv::Mat reshapeToRows(const cv::Mat& src, int featureCols)
{
    if (src.empty() || featureCols <= 0) {
        return {};
    }

    cv::Mat f32;
    if (src.type() != CV_32F) {
        src.convertTo(f32, CV_32F);
    } else {
        f32 = src;
    }

    const int total = static_cast<int>(f32.total());
    if (total % featureCols != 0) {
        return {};
    }
    const int rows = total / featureCols;

    if (f32.dims == 2 && f32.rows == rows && f32.cols == featureCols && f32.isContinuous()) {
        return f32;
    }

    cv::Mat out(rows, featureCols, CV_32F);
    cv::Mat flat = f32.isContinuous() ? f32 : f32.clone();
    if (static_cast<int>(flat.total()) != total) {
        return {};
    }
    std::memcpy(out.ptr<float>(), flat.ptr<float>(), static_cast<size_t>(total) * sizeof(float));
    return out;
}

inline float readScalar(const cv::Mat& mat)
{
    if (mat.empty()) {
        return 0.f;
    }
    if (mat.type() != CV_32F) {
        cv::Mat converted;
        mat.convertTo(converted, CV_32F);
        return converted.ptr<float>()[0];
    }
    return mat.ptr<float>()[0];
}

inline cv::Mat palmRowToBox(const cv::Mat& palm)
{
    cv::Mat box(2, 2, CV_32F);
    if (palm.cols < 4) {
        return {};
    }
    box.at<float>(0, 0) = palm.at<float>(0, 0);
    box.at<float>(0, 1) = palm.at<float>(0, 1);
    box.at<float>(1, 0) = palm.at<float>(0, 2);
    box.at<float>(1, 1) = palm.at<float>(0, 3);
    return box;
}

inline cv::Mat palmRowToLandmarks(const cv::Mat& palm)
{
    if (palm.cols < 18) {
        return {};
    }
    cv::Mat landmarks(7, 2, CV_32F);
    for (int i = 0; i < 7; ++i) {
        landmarks.at<float>(i, 0) = palm.at<float>(0, 4 + i * 2);
        landmarks.at<float>(i, 1) = palm.at<float>(0, 4 + i * 2 + 1);
    }
    return landmarks;
}

inline std::string toLowerAscii(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

inline int findOutputByNameAndSize(const std::vector<cv::String>& names,
                                   const std::vector<cv::Mat>& outputs,
                                   const std::vector<std::string>& keywords,
                                   int expectedTotal = -1)
{
    for (size_t i = 0; i < outputs.size(); ++i) {
        const std::string name = toLowerAscii(names[i]);
        bool matched = keywords.empty();
        for (const std::string& keyword : keywords) {
            if (name.find(keyword) != std::string::npos) {
                matched = true;
                break;
            }
        }
        if (!matched) {
            continue;
        }
        if (expectedTotal > 0 && static_cast<int>(outputs[i].total()) != expectedTotal) {
            continue;
        }
        return static_cast<int>(i);
    }
    return -1;
}

inline void resolvePalmOutputs(const std::vector<cv::Mat>& outputs, cv::Mat& boxes, cv::Mat& scores)
{
    if (outputs.size() < 2) {
        return;
    }

    // Official demo order: [0]=boxes, [1]=scores. Fall back to size when swapped.
    const cv::Mat& first = outputs[0];
    const cv::Mat& second = outputs[1];
    if (first.total() % 18 == 0 && second.total() % 18 != 0) {
        boxes = first;
        scores = second;
    } else if (second.total() % 18 == 0 && first.total() % 18 != 0) {
        boxes = second;
        scores = first;
    } else if (first.total() >= second.total()) {
        boxes = first;
        scores = second;
    } else {
        boxes = second;
        scores = first;
    }
}

inline float activateDetectionScore(float raw)
{
    raw = std::clamp(raw, -100.f, 100.f);
    if (raw >= 0.f && raw <= 1.f) {
        return raw;
    }
    return 1.f / (1.f + std::exp(-raw));
}

inline cv::Mat reshapePalmBoxes(const cv::Mat& tensor)
{
    return reshapeToRows(tensor, 18);
}

inline cv::Mat reshapePalmScores(const cv::Mat& tensor)
{
    return reshapeToRows(tensor, 1);
}

inline bool resolveHandPoseOutputs(const std::vector<cv::String>& names,
                                   const std::vector<cv::Mat>& outputs,
                                   cv::Mat& landmarks,
                                   cv::Mat& conf,
                                   cv::Mat& handedness,
                                   cv::Mat& worldLandmarks)
{
    if (outputs.empty()) {
        return false;
    }

    // Official mp_handpose.py order: landmarks, conf, handedness, world_landmarks
    if (outputs.size() == 4) {
        landmarks = outputs[0];
        conf = outputs[1];
        handedness = outputs[2];
        worldLandmarks = outputs[3];
        return true;
    }

    struct OutputItem
    {
        int index = -1;
        int total = 0;
        std::string name;
    };
    std::vector<OutputItem> items;
    items.reserve(outputs.size());
    for (size_t i = 0; i < outputs.size(); ++i) {
        items.push_back({static_cast<int>(i), static_cast<int>(outputs[i].total()), toLowerAscii(names[i])});
    }
    std::sort(items.begin(), items.end(), [](const OutputItem& a, const OutputItem& b) {
        return a.total < b.total;
    });

    auto pickByKeywords = [&](const std::vector<std::string>& keywords, int expectedTotal) -> int {
        return findOutputByNameAndSize(names, outputs, keywords, expectedTotal);
    };

    int idxLandmarks = pickByKeywords({"landmark", "screen", "xyz"}, 63);
    int idxWorld = pickByKeywords({"world"}, 63);
    int idxConf = pickByKeywords({"conf", "score", "presence"}, 1);
    int idxHandedness = pickByKeywords({"handed", "handness", "side"}, 1);

    std::vector<int> scalarOutputs;
    std::vector<int> landmarkOutputs;
    for (const OutputItem& item : items) {
        if (item.total == 1) {
            scalarOutputs.push_back(item.index);
        } else if (item.total == 63 || item.total == 21 * 3) {
            landmarkOutputs.push_back(item.index);
        }
    }

    if (idxLandmarks < 0 && !landmarkOutputs.empty()) {
        idxLandmarks = landmarkOutputs.front();
    }
    if (idxWorld < 0 && landmarkOutputs.size() >= 2) {
        idxWorld = landmarkOutputs[1];
    } else if (idxWorld < 0 && !landmarkOutputs.empty()) {
        idxWorld = landmarkOutputs.front();
    }
    if (idxConf < 0 && !scalarOutputs.empty()) {
        idxConf = scalarOutputs.front();
    }
    if (idxHandedness < 0 && scalarOutputs.size() >= 2) {
        idxHandedness = scalarOutputs[1];
    } else if (idxHandedness < 0 && !scalarOutputs.empty()) {
        idxHandedness = scalarOutputs.front();
    }

    if (idxLandmarks < 0 || idxConf < 0) {
        return false;
    }
    if (idxWorld < 0) {
        idxWorld = idxLandmarks;
    }
    if (idxHandedness < 0) {
        idxHandedness = idxConf;
    }

    landmarks = outputs[static_cast<size_t>(idxLandmarks)];
    conf = outputs[static_cast<size_t>(idxConf)];
    handedness = outputs[static_cast<size_t>(idxHandedness)];
    worldLandmarks = outputs[static_cast<size_t>(idxWorld)];
    return true;
}

inline cv::Mat addShiftToBoxRows(cv::Mat& box, const cv::Point2f& shift)
{
    const cv::Mat shiftMat = (cv::Mat_<float>(1, 2) << shift.x, shift.y);
    box.row(0) += shiftMat;
    box.row(1) += shiftMat;
    return box;
}

inline cv::Mat buildBoxFromCenter(const cv::Mat& centerBox, const cv::Mat& halfSize)
{
    cv::Mat expanded;
    const cv::Mat topLeft = centerBox - halfSize;
    const cv::Mat bottomRight = centerBox + halfSize;
    cv::vconcat(topLeft, bottomRight, expanded);
    return expanded;
}

struct HandResult
{
    cv::Mat bbox;
    cv::Mat landmarks;
    cv::Mat worldLandmarks;
    float handedness = 0.f;
    float confidence = 0.f;
    bool valid = false;
};

class PalmDetector
{
public:
    PalmDetector(const std::string& modelPath,
                 float nmsThreshold = 0.3f,
                 float scoreThreshold = 0.3f,
                 int topK = 5000,
                 bool preferCuda = true)
        : modelPath_(modelPath)
        , nmsThreshold_(nmsThreshold)
        , scoreThreshold_(scoreThreshold)
        , topK_(topK)
        , inputSize_(192, 192)
        , preferCuda_(preferCuda)
    {
        net_ = cv::dnn::readNet(modelPath_);
        applyDnnBackend(net_, preferCuda_, usingCuda_);
        anchors_ = getPalmAnchors();
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
        const double ratio = std::min(
            inputSize_.height / static_cast<double>(img.rows),
            inputSize_.width / static_cast<double>(img.cols));

        cv::Size ratioSize(
            static_cast<int>(img.cols * ratio),
            static_cast<int>(img.rows * ratio));
        cv::Mat resized;
        cv::resize(img, resized, ratioSize);

        padBias = cv::Size(0, 0);
        const int padH = inputSize_.height - ratioSize.height;
        const int padW = inputSize_.width - ratioSize.width;
        const int left = padW / 2;
        const int top = padH / 2;
        const int right = padW - left;
        const int bottom = padH - top;
        padBias.width = left;
        padBias.height = top;
        cv::copyMakeBorder(resized, resized, top, bottom, left, right, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
        padBias.width = static_cast<int>(padBias.width / ratio);
        padBias.height = static_cast<int>(padBias.height / ratio);

        cv::dnn::Image2BlobParams params;
        params.datalayout = cv::dnn::DNN_LAYOUT_NHWC;
        params.ddepth = CV_32F;
        params.mean = cv::Scalar::all(0);
        params.scalefactor = cv::Scalar::all(1.0 / 255.0);
        params.size = inputSize_;
        params.swapRB = true;
        params.paddingmode = cv::dnn::DNN_PMODE_NULL;
        blob = cv::dnn::blobFromImageWithParams(resized, params);
    }

    cv::Mat postprocess(const std::vector<cv::Mat>& outputs,
                        const cv::Size& orgSize,
                        const cv::Size& padBias)
    {
        if (outputs.size() < 2 || anchors_.rows <= 0) {
            return {};
        }

        cv::Mat boxesTensor;
        cv::Mat scoresTensor;
        resolvePalmOutputs(outputs, boxesTensor, scoresTensor);

        cv::Mat boxLandDelta = reshapePalmBoxes(boxesTensor);
        cv::Mat score = reshapePalmScores(scoresTensor);
        if (boxLandDelta.empty() || score.empty()) {
            return {};
        }
        if (boxLandDelta.type() != CV_32F) {
            boxLandDelta.convertTo(boxLandDelta, CV_32F);
        }
        if (score.type() != CV_32F) {
            score.convertTo(score, CV_32F);
        }
        if (score.cols > 1) {
            score = score.col(0);
        }

        const int numAnchors = std::min({boxLandDelta.rows, score.rows, anchors_.rows});
        if (numAnchors <= 0) {
            return {};
        }
        if (boxLandDelta.rows > numAnchors) {
            boxLandDelta = boxLandDelta.rowRange(0, numAnchors);
        }
        if (score.rows > numAnchors) {
            score = score.rowRange(0, numAnchors);
        }

        cv::Mat boxDelta = boxLandDelta.colRange(0, 4);
        cv::Mat landmarkDelta = boxLandDelta.colRange(4, boxLandDelta.cols);

        const float scale = static_cast<float>(std::max(orgSize.height, orgSize.width));

        boxDelta.col(0) = boxDelta.col(0) / inputSize_.width;
        boxDelta.col(1) = boxDelta.col(1) / inputSize_.height;
        boxDelta.col(2) = boxDelta.col(2) / inputSize_.width;
        boxDelta.col(3) = boxDelta.col(3) / inputSize_.height;

        cv::Mat anchorsUsed = anchors_.rowRange(0, numAnchors);
        cv::Mat xy1 = (boxDelta.colRange(0, 2) - boxDelta.colRange(2, 4) / 2 + anchorsUsed) * scale;
        cv::Mat xy2 = (boxDelta.colRange(0, 2) + boxDelta.colRange(2, 4) / 2 + anchorsUsed) * scale;
        cv::Mat boxes;
        cv::hconcat(xy1, xy2, boxes);
        boxes.col(0) = boxes.col(0) - padBias.width;
        boxes.col(1) = boxes.col(1) - padBias.height;
        boxes.col(2) = boxes.col(2) - padBias.width;
        boxes.col(3) = boxes.col(3) - padBias.height;

        std::vector<cv::Rect2d> rotBoxes(static_cast<size_t>(numAnchors));
        for (int i = 0; i < numAnchors; ++i) {
            const float x1 = std::min(boxes.at<float>(i, 0), boxes.at<float>(i, 2));
            const float y1 = std::min(boxes.at<float>(i, 1), boxes.at<float>(i, 3));
            const float x2 = std::max(boxes.at<float>(i, 0), boxes.at<float>(i, 2));
            const float y2 = std::max(boxes.at<float>(i, 1), boxes.at<float>(i, 3));
            rotBoxes[static_cast<size_t>(i)] = cv::Rect2d(
                cv::Point2d(x1, y1),
                cv::Point2d(x2, y2));
        }

        cv::Mat activatedScore(numAnchors, 1, CV_32F);
        for (int i = 0; i < numAnchors; ++i) {
            activatedScore.at<float>(i, 0) = activateDetectionScore(score.at<float>(i, 0));
        }

        std::vector<int> keep;
        cv::dnn::NMSBoxes(rotBoxes, activatedScore, scoreThreshold_, nmsThreshold_, keep, 1.0f, topK_);
        if (keep.empty()) {
            return {};
        }

        constexpr int nbCols = 4 + 14 + 1;
        cv::Mat candidates(static_cast<int>(keep.size()), nbCols, CV_32FC1);
        int row = 0;
        for (const int idx : keep) {
            candidates.at<float>(row, nbCols - 1) = activatedScore.at<float>(idx, 0);
            candidates.at<float>(row, 0) = static_cast<float>(rotBoxes[static_cast<size_t>(idx)].x);
            candidates.at<float>(row, 1) = static_cast<float>(rotBoxes[static_cast<size_t>(idx)].y);
            candidates.at<float>(row, 2) = static_cast<float>(rotBoxes[static_cast<size_t>(idx)].x + rotBoxes[static_cast<size_t>(idx)].width);
            candidates.at<float>(row, 3) = static_cast<float>(rotBoxes[static_cast<size_t>(idx)].y + rotBoxes[static_cast<size_t>(idx)].height);
            for (int k = 0; k < 7; ++k) {
                const int lx = k * 2;
                const int ly = lx + 1;
                candidates.at<float>(row, 4 + lx) =
                    (landmarkDelta.at<float>(idx, lx) / inputSize_.width + anchorsUsed.at<float>(idx, 0)) * scale
                    - padBias.width;
                candidates.at<float>(row, 4 + ly) =
                    (landmarkDelta.at<float>(idx, ly) / inputSize_.height + anchorsUsed.at<float>(idx, 1)) * scale
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

class HandPoseEstimator
{
public:
    HandPoseEstimator(const std::string& modelPath, float confThreshold = 0.3f, bool preferCuda = true)
        : modelPath_(modelPath)
        , confThreshold_(confThreshold)
        , inputSize_(224, 224)
        , preferCuda_(preferCuda)
    {
        net_ = cv::dnn::readNet(modelPath_);
        applyDnnBackend(net_, preferCuda_, usingCuda_);
        outputNames_ = net_.getUnconnectedOutLayersNames();
    }

    bool isLoaded() const { return !net_.empty(); }
    bool usingCuda() const { return usingCuda_; }
    void disableCuda() { fallbackDnnNetToCpu(net_, usingCuda_); }

    std::optional<HandResult> infer(const cv::Mat& image, const cv::Mat& palm)
    {
        if (image.empty() || palm.empty() || !isLoaded()) {
            return std::nullopt;
        }

        cv::Mat inputBlob;
        cv::Mat rotatedPalmBox;
        float angle = 0.f;
        cv::Mat rotationMatrix;
        cv::Size padBias;
        if (!preprocess(image, palm, inputBlob, rotatedPalmBox, angle, rotationMatrix, padBias)) {
            return std::nullopt;
        }

        std::vector<cv::Mat> outputBlob;
        dnnForward(net_, usingCuda_, inputBlob, outputBlob);
        return postprocess(outputBlob, rotatedPalmBox, angle, rotationMatrix, padBias);
    }

private:
    bool cropAndPadFromPalm(cv::Mat& image,
                            cv::Mat& palmBoxf,
                            cv::Point& bias,
                            bool forRotation)
    {
        cv::Mat wh = palmBoxf.row(1) - palmBoxf.row(0);
        cv::Point2f shiftVec(
            (forRotation ? palmBoxPreShift_.x : palmBoxShift_.x) * wh.at<float>(0, 0),
            (forRotation ? palmBoxPreShift_.y : palmBoxShift_.y) * wh.at<float>(0, 1));
        addShiftToBoxRows(palmBoxf, shiftVec);

        cv::Mat centerBox;
        cv::reduce(palmBoxf, centerBox, 0, cv::REDUCE_AVG, CV_32F);
        wh = palmBoxf.row(1) - palmBoxf.row(0);
        const float enlarge = forRotation ? palmBoxPreEnlargeFactor_ : palmBoxEnlargeFactor_;
        cv::Mat newHalfSize = wh * enlarge / 2.f;
        cv::Mat expanded = buildBoxFromCenter(centerBox, newHalfSize);

        cv::Mat palmBox;
        expanded.convertTo(palmBox, CV_32S);
        palmBox.at<int>(0, 0) = std::clamp(palmBox.at<int>(0, 0), 0, image.cols);
        palmBox.at<int>(0, 1) = std::clamp(palmBox.at<int>(0, 1), 0, image.rows);
        palmBox.at<int>(1, 0) = std::clamp(palmBox.at<int>(1, 0), 0, image.cols);
        palmBox.at<int>(1, 1) = std::clamp(palmBox.at<int>(1, 1), 0, image.rows);

        const int cropW = palmBox.at<int>(1, 0) - palmBox.at<int>(0, 0);
        const int cropH = palmBox.at<int>(1, 1) - palmBox.at<int>(0, 1);
        if (cropW <= 0 || cropH <= 0) {
            return false;
        }

        image = image(cv::Rect(
            palmBox.at<int>(0, 0),
            palmBox.at<int>(0, 1),
            cropW,
            cropH)).clone();

        int sideLen = 0;
        if (forRotation) {
            sideLen = static_cast<int>(std::lround(cv::norm(cv::Vec2f(
                static_cast<float>(image.cols),
                static_cast<float>(image.rows)))));
        } else {
            sideLen = std::max(image.cols, image.rows);
        }
        sideLen = std::max(1, sideLen);

        const int padH = sideLen - image.rows;
        const int padW = sideLen - image.cols;
        const int left = padW / 2;
        const int top = padH / 2;
        const int right = padW - left;
        const int bottom = padH - top;
        cv::copyMakeBorder(image, image, top, bottom, left, right, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));

        bias = cv::Point(palmBox.at<int>(0, 0) - left, palmBox.at<int>(0, 1) - top);
        expanded.copyTo(palmBoxf);
        return true;
    }

    bool preprocess(const cv::Mat& image,
                    const cv::Mat& palm,
                    cv::Mat& blob,
                    cv::Mat& rotatedPalmBox,
                    float& angle,
                    cv::Mat& rotationMatrix,
                    cv::Size& padBias)
    {
        padBias = cv::Size(0, 0);
        cv::Mat palmBoxf = palmRowToBox(palm);
        if (palmBoxf.empty()) {
            return false;
        }
        if (palmBoxf.type() != CV_32F) {
            palmBoxf.convertTo(palmBoxf, CV_32F);
        }

        cv::Mat work = image.clone();
        cv::Point bias(0, 0);
        if (!cropAndPadFromPalm(work, palmBoxf, bias, true)) {
            return false;
        }
        padBias.width += bias.x;
        padBias.height += bias.y;

        palmBoxf.at<float>(0, 0) -= static_cast<float>(padBias.width);
        palmBoxf.at<float>(0, 1) -= static_cast<float>(padBias.height);
        palmBoxf.at<float>(1, 0) -= static_cast<float>(padBias.width);
        palmBoxf.at<float>(1, 1) -= static_cast<float>(padBias.height);

        cv::Mat palmLandmarks = palmRowToLandmarks(palm);
        if (palmLandmarks.empty()) {
            return false;
        }
        if (palmLandmarks.type() != CV_32F) {
            palmLandmarks.convertTo(palmLandmarks, CV_32F);
        }
        for (int i = 0; i < palmLandmarks.rows; ++i) {
            palmLandmarks.at<float>(i, 0) -= static_cast<float>(padBias.width);
            palmLandmarks.at<float>(i, 1) -= static_cast<float>(padBias.height);
        }

        const cv::Point2f p1(palmLandmarks.at<float>(0, 0), palmLandmarks.at<float>(0, 1));
        const cv::Point2f p2(palmLandmarks.at<float>(2, 0), palmLandmarks.at<float>(2, 1));

        float radians = static_cast<float>(
            kPi / 2.0 - std::atan2(-(p2.y - p1.y), p2.x - p1.x));
        radians = radians - 2.f * static_cast<float>(kPi)
            * static_cast<float>(static_cast<int>((radians + static_cast<float>(kPi)) / (2.f * static_cast<float>(kPi))));
        angle = radians * 180.f / static_cast<float>(kPi);

        cv::Mat centerBox;
        cv::reduce(palmBoxf, centerBox, 0, cv::REDUCE_AVG, CV_32F);
        const cv::Point2f center(centerBox.at<float>(0, 0), centerBox.at<float>(0, 1));

        rotationMatrix = cv::getRotationMatrix2D(center, angle, 1.0);
        cv::Mat rotatedImage;
        cv::warpAffine(work, rotatedImage, rotationMatrix, cv::Size(work.cols, work.rows));

        cv::Mat homogeneous(7, 3, CV_32F);
        for (int i = 0; i < 7; ++i) {
            homogeneous.at<float>(i, 0) = palmLandmarks.at<float>(i, 0);
            homogeneous.at<float>(i, 1) = palmLandmarks.at<float>(i, 1);
            homogeneous.at<float>(i, 2) = 1.f;
        }

        cv::Mat rotMat32;
        rotationMatrix.convertTo(rotMat32, CV_32F);
        cv::Mat rotatedLm(2, 7, CV_32F);
        for (int i = 0; i < 7; ++i) {
            rotatedLm.at<float>(0, i) = homogeneous.at<float>(i, 0) * rotMat32.at<float>(0, 0)
                + homogeneous.at<float>(i, 1) * rotMat32.at<float>(0, 1)
                + rotMat32.at<float>(0, 2);
            rotatedLm.at<float>(1, i) = homogeneous.at<float>(i, 0) * rotMat32.at<float>(1, 0)
                + homogeneous.at<float>(i, 1) * rotMat32.at<float>(1, 1)
                + rotMat32.at<float>(1, 2);
        }

        double minX = 0.0;
        double maxX = 0.0;
        double minY = 0.0;
        double maxY = 0.0;
        cv::minMaxLoc(rotatedLm.row(0), &minX, &maxX);
        cv::minMaxLoc(rotatedLm.row(1), &minY, &maxY);
        cv::Mat rotatedPalmBoxf = (cv::Mat_<float>(2, 2) << static_cast<float>(minX), static_cast<float>(minY),
                                   static_cast<float>(maxX), static_cast<float>(maxY));

        cv::Point cropBias(0, 0);
        if (!cropAndPadFromPalm(rotatedImage, rotatedPalmBoxf, cropBias, false)) {
            return false;
        }

        cv::Mat resized;
        cv::resize(rotatedImage, resized, inputSize_, 0, 0, cv::INTER_AREA);

        cv::dnn::Image2BlobParams params;
        params.datalayout = cv::dnn::DNN_LAYOUT_NHWC;
        params.ddepth = CV_32F;
        params.mean = cv::Scalar::all(0);
        params.scalefactor = cv::Scalar::all(1.0 / 255.0);
        params.size = inputSize_;
        params.swapRB = true;
        params.paddingmode = cv::dnn::DNN_PMODE_NULL;
        blob = cv::dnn::blobFromImageWithParams(resized, params);

        rotatedPalmBox = rotatedPalmBoxf;
        return true;
    }

    std::optional<HandResult> postprocess(const std::vector<cv::Mat>& blob,
                                          const cv::Mat& rotatedPalmBox,
                                          float angle,
                                          const cv::Mat& rotationMatrix,
                                          const cv::Size& padBias)
    {
        if (blob.empty()) {
            return std::nullopt;
        }

        cv::Mat landmarksTensor;
        cv::Mat confTensor;
        cv::Mat handednessTensor;
        cv::Mat worldTensor;
        if (!resolveHandPoseOutputs(outputNames_, blob, landmarksTensor, confTensor, handednessTensor, worldTensor)) {
            return std::nullopt;
        }

        cv::Mat landmarks = reshapeToRows(landmarksTensor, 3);
        cv::Mat landmarksWorld = reshapeToRows(worldTensor, 3);
        cv::Mat confMat = reshapeToRows(confTensor, 1);
        cv::Mat handednessMat = reshapeToRows(handednessTensor, 1);
        if (landmarks.empty() || landmarksWorld.empty() || confMat.empty() || handednessMat.empty()) {
            return std::nullopt;
        }
        if (landmarks.rows != 21 || landmarksWorld.rows != 21) {
            return std::nullopt;
        }

        const float valConf = [&]() {
            float best = activateDetectionScore(readScalar(confMat));
            for (const cv::Mat& out : blob) {
                if (out.total() == 1) {
                    best = std::max(best, activateDetectionScore(readScalar(out)));
                }
            }
            return best;
        }();
        if (valConf < confThreshold_) {
            return std::nullopt;
        }
        const float handedness = readScalar(handednessMat);

        if (landmarks.type() != CV_32F) {
            landmarks.convertTo(landmarks, CV_32F);
        }
        if (landmarksWorld.type() != CV_32F) {
            landmarksWorld.convertTo(landmarksWorld, CV_32F);
        }

        cv::Mat whRotatedPalmBox = rotatedPalmBox.row(1) - rotatedPalmBox.row(0);
        const float scaleFactor = std::max(
            whRotatedPalmBox.at<float>(0, 0) / inputSize_.width,
            whRotatedPalmBox.at<float>(0, 1) / inputSize_.height);

        for (int i = 0; i < landmarks.rows; ++i) {
            landmarks.at<float>(i, 0) =
                (landmarks.at<float>(i, 0) - inputSize_.width / 2.f) * scaleFactor;
            landmarks.at<float>(i, 1) =
                (landmarks.at<float>(i, 1) - inputSize_.height / 2.f) * scaleFactor;
            landmarks.at<float>(i, 2) *= scaleFactor;
        }

        cv::Mat coordsRotationMatrix;
        cv::getRotationMatrix2D(cv::Point2f(0.f, 0.f), angle, 1.0).convertTo(coordsRotationMatrix, CV_32F);
        cv::Mat rotatedLm_xy = landmarks.colRange(0, 2) * coordsRotationMatrix.colRange(0, 2);
        cv::Mat rotatedLandmarks;
        cv::hconcat(rotatedLm_xy, landmarks.col(2), rotatedLandmarks);

        cv::Mat rotatedWorld_xy = landmarksWorld.colRange(0, 2) * coordsRotationMatrix.colRange(0, 2);
        cv::Mat rotatedLandmarksWorld;
        cv::hconcat(rotatedWorld_xy, landmarksWorld.col(2), rotatedLandmarksWorld);

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
        cv::reduce(rotatedPalmBox, rc, 0, cv::REDUCE_AVG, CV_32F);
        cv::Mat center = (cv::Mat_<float>(1, 3) << rc.at<float>(0, 0), rc.at<float>(0, 1), 1.f);
        const float originalCenterX = center.dot(inverseRotationMatrix.row(0));
        const float originalCenterY = center.dot(inverseRotationMatrix.row(1));

        for (int i = 0; i < rotatedLandmarks.rows; ++i) {
            landmarks.at<float>(i, 0) = rotatedLandmarks.at<float>(i, 0)
                + originalCenterX + static_cast<float>(padBias.width);
            landmarks.at<float>(i, 1) = rotatedLandmarks.at<float>(i, 1)
                + originalCenterY + static_cast<float>(padBias.height);
            landmarks.at<float>(i, 2) = rotatedLandmarks.at<float>(i, 2);
        }

        double vmin0 = 0.0;
        double vmax0 = 0.0;
        double vmin1 = 0.0;
        double vmax1 = 0.0;
        cv::minMaxLoc(landmarks.col(0), &vmin0, &vmax0);
        cv::minMaxLoc(landmarks.col(1), &vmin1, &vmax1);
        cv::Mat bbox = (cv::Mat_<float>(2, 2) << static_cast<float>(vmin0), static_cast<float>(vmin1),
                        static_cast<float>(vmax0), static_cast<float>(vmax1));

        cv::Mat whBox = bbox.row(1) - bbox.row(0);
        const cv::Mat handShiftMat = (cv::Mat_<float>(1, 2) << handBoxShift_.x * whBox.at<float>(0, 0),
                                    handBoxShift_.y * whBox.at<float>(0, 1));
        bbox.row(0) += handShiftMat;
        bbox.row(1) += handShiftMat;

        cv::Mat centerBox;
        cv::reduce(bbox, centerBox, 0, cv::REDUCE_AVG, CV_32F);
        whBox = bbox.row(1) - bbox.row(0);
        cv::Mat newHalfSize = whBox * handBoxEnlargeFactor_ / 2.f;
        bbox = buildBoxFromCenter(centerBox, newHalfSize);

        HandResult result;
        result.bbox = bbox;
        result.landmarks = landmarks;
        result.worldLandmarks = rotatedLandmarksWorld;
        result.handedness = handedness;
        result.confidence = valConf;
        result.valid = true;
        return result;
    }

    std::string modelPath_;
    float confThreshold_;
    cv::Size inputSize_;
    bool preferCuda_ = true;
    bool usingCuda_ = false;
    cv::Point2f palmBoxPreShift_{0.f, 0.f};
    cv::Point2f palmBoxShift_{0.f, -0.4f};
    float palmBoxPreEnlargeFactor_ = 4.f;
    float palmBoxEnlargeFactor_ = 3.f;
    cv::Point2f handBoxShift_{0.f, -0.1f};
    float handBoxEnlargeFactor_ = 1.65f;
    cv::dnn::Net net_;
    std::vector<cv::String> outputNames_;
};

inline void drawHandLine(cv::Mat& image, const cv::Mat& landmarks, int i1, int i2, int thickness = 2)
{
    cv::line(
        image,
        cv::Point(static_cast<int>(landmarks.at<float>(i1, 0)), static_cast<int>(landmarks.at<float>(i1, 1))),
        cv::Point(static_cast<int>(landmarks.at<float>(i2, 0)), static_cast<int>(landmarks.at<float>(i2, 1))),
        cv::Scalar(255, 255, 255),
        thickness);
}

inline void drawHandOverlay(cv::Mat& image, const HandResult& hand)
{
    if (!hand.valid || hand.landmarks.empty()) {
        return;
    }

    if (!hand.bbox.empty()) {
        cv::Mat box;
        hand.bbox.convertTo(box, CV_32S);
        cv::rectangle(
            image,
            cv::Point(box.at<int>(0, 0), box.at<int>(0, 1)),
            cv::Point(box.at<int>(1, 0), box.at<int>(1, 1)),
            cv::Scalar(0, 255, 0),
            2);
        const std::string label = hand.handedness <= 0.5f ? "Left" : "Right";
        cv::putText(
            image,
            label,
            cv::Point(box.at<int>(0, 0), box.at<int>(0, 1) + 12),
            cv::FONT_HERSHEY_DUPLEX,
            0.5,
            cv::Scalar(0, 0, 255),
            1);
    }

    const cv::Mat& lm = hand.landmarks;
    drawHandLine(image, lm, 0, 1);
    drawHandLine(image, lm, 1, 2);
    drawHandLine(image, lm, 2, 3);
    drawHandLine(image, lm, 3, 4);
    drawHandLine(image, lm, 0, 5);
    drawHandLine(image, lm, 5, 6);
    drawHandLine(image, lm, 6, 7);
    drawHandLine(image, lm, 7, 8);
    drawHandLine(image, lm, 0, 9);
    drawHandLine(image, lm, 9, 10);
    drawHandLine(image, lm, 10, 11);
    drawHandLine(image, lm, 11, 12);
    drawHandLine(image, lm, 0, 13);
    drawHandLine(image, lm, 13, 14);
    drawHandLine(image, lm, 14, 15);
    drawHandLine(image, lm, 15, 16);
    drawHandLine(image, lm, 0, 17);
    drawHandLine(image, lm, 17, 18);
    drawHandLine(image, lm, 18, 19);
    drawHandLine(image, lm, 19, 20);

    for (int i = 0; i < lm.rows; ++i) {
        const int radius = std::max(2, 5 - static_cast<int>(lm.at<float>(i, 2) / 5.f));
        cv::circle(
            image,
            cv::Point(static_cast<int>(lm.at<float>(i, 0)), static_cast<int>(lm.at<float>(i, 1))),
            std::min(radius, 14),
            cv::Scalar(0, 0, 255),
            -1);
    }
}

inline std::vector<HandResult> detectHands(PalmDetector& palmDetector,
                                           HandPoseEstimator& handEstimator,
                                           const cv::Mat& image)
{
    std::vector<HandResult> results;
    const cv::Mat palms = palmDetector.infer(image);
    if (palms.empty()) {
        return results;
    }

    for (int i = 0; i < palms.rows; ++i) {
        const cv::Mat palm = palms.row(i);
        if (auto hand = handEstimator.infer(image, palm)) {
            results.push_back(*hand);
        }
    }
    return results;
}

} // namespace MediaPipeHand
