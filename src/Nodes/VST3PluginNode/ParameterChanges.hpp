//
// Created by WuBin on 2025/9/4.
//

#pragma once
#include <QtCore/QObject>
#include "DataTypes/NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>
#include <QtCore/qglobal.h>
#include <cstdio>
#include <iostream>
#if WIN32
#include "windows.h"
#include <wtypes.h>
#endif
#include "public.sdk/source/vst/hosting/hostclasses.h"
#include "public.sdk/source/vst/hosting/module.h"
#include "public.sdk/source/vst/hosting/plugprovider.h"
#include "pluginterfaces/gui/iplugview.h"
#include <pluginterfaces/vst/ivstcomponent.h>
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "base/source/fdebug.h"
#include <QFileDialog>

#include <memory>

#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
using namespace Steinberg;
using namespace VST3;

class ParamValueQueue : public Vst::IParamValueQueue
    {
    public:
        ParamValueQueue(Vst::ParamID id) : paramId(id) {}

        // 禁用拷贝构造和赋值操作
        ParamValueQueue(const ParamValueQueue&) = delete;
        ParamValueQueue& operator=(const ParamValueQueue&) = delete;

        // 启用移动构造和赋值操作
        ParamValueQueue(ParamValueQueue&& other) noexcept
            : paramId(other.paramId), points(std::move(other.points)), refCount_(other.refCount_.load()) {}

        ParamValueQueue& operator=(ParamValueQueue&& other) noexcept
        {
            if (this != &other) {
                paramId = other.paramId;
                points = std::move(other.points);
                refCount_.store(other.refCount_.load());
            }
            return *this;
        }

        /**
         * @brief 查询接口实现
         * @param iid 接口ID
         * @param obj 输出对象指针
         * @return 查询结果
         */
        tresult PLUGIN_API queryInterface(const TUID iid, void** obj) override
        {
            QUERY_INTERFACE(iid, obj, FUnknown::iid, FUnknown)
            QUERY_INTERFACE(iid, obj, Vst::IParamValueQueue::iid, Vst::IParamValueQueue)
            *obj = nullptr;
            return kNoInterface;
        }

        /**
         * @brief 增加引用计数
         * @return 新的引用计数值
         */
        uint32 PLUGIN_API addRef() override
        {
            return ++refCount_;
        }

        /**
         * @brief 减少引用计数
         * @return 新的引用计数值
         */
        uint32 PLUGIN_API release() override
        {
            uint32 count = --refCount_;
            if (count == 0)
                delete this;
            return count;
        }

        Vst::ParamID PLUGIN_API getParameterId() override { return paramId; }
        int32 PLUGIN_API getPointCount() override { return static_cast<int32>(points.size()); }
        tresult PLUGIN_API getPoint(int32 index, int32& sampleOffset, Vst::ParamValue& value) override
        {
            if (index >= 0 && index < static_cast<int32>(points.size()))
            {
                sampleOffset = points[index].sampleOffset;
                value = points[index].value;
                return kResultTrue;
            }
            return kResultFalse;
        }
        tresult PLUGIN_API addPoint(int32 sampleOffset, Vst::ParamValue value, int32& index) override
        {
            points.push_back({sampleOffset, value});
            index = static_cast<int32>(points.size() - 1);
            return kResultTrue;
        }

        void clear() { points.clear(); }

    private:
        struct Point { int32 sampleOffset; Vst::ParamValue value; };

        Vst::ParamID paramId;
        std::vector<Point> points;
        std::atomic<uint32> refCount_{1}; // 引用计数
    };

class ParameterChanges : public Vst::IParameterChanges
{
public:
    ParameterChanges() = default;
    virtual ~ParameterChanges() = default;

    /**
     * @brief 查询接口实现
     * @param iid 接口ID
     * @param obj 输出对象指针
     * @return 查询结果
     */
    tresult PLUGIN_API queryInterface(const TUID iid, void** obj) override
    {
        QUERY_INTERFACE(iid, obj, FUnknown::iid, FUnknown)
        QUERY_INTERFACE(iid, obj, Vst::IParameterChanges::iid, Vst::IParameterChanges)
        *obj = nullptr;
        return kNoInterface;
    }

    /**
     * @brief 增加引用计数
     * @return 新的引用计数值
     */
    uint32 PLUGIN_API addRef() override
    {
        return ++refCount_;
    }

    /**
     * @brief 减少引用计数
     * @return 新的引用计数值
     */
    uint32 PLUGIN_API release() override
    {
        uint32 count = --refCount_;
        if (count == 0)
            delete this;
        return count;
    }

    // IParameterChanges interface
    int32 PLUGIN_API getParameterCount() override { return static_cast<int32>(queues.size()); }

    Vst::IParamValueQueue* PLUGIN_API getParameterData(int32 index) override
    {
        if (index >= 0 && index < static_cast<int32>(queues.size()))
            return queues[index].get();
        return nullptr;
    }

    Vst::IParamValueQueue* PLUGIN_API addParameterData(const Vst::ParamID& id, int32& index) override
    {
        for (size_t i = 0; i < queues.size(); ++i)
        {
            if (queues[i]->getParameterId() == id)
            {
                index = static_cast<int32>(i);
                return queues[i].get();
            }
        }
        queues.push_back(std::make_unique<ParamValueQueue>(id));
        index = static_cast<int32>(queues.size() - 1);
        return queues.back().get();
    }

    void clear() { queues.clear(); }



private:
    std::vector<std::unique_ptr<ParamValueQueue>> queues; // 使用智能指针存储
    std::atomic<uint32> refCount_{1}; // 引用计数
};

