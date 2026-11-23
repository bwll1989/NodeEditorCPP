// ============================================================================
//  LiveFramePool —— LiveFrame 缓冲池（Ingest 生产端逐帧复用）
//
//  【2026-11-23 性能修复】原各 Ingest 逐帧 make_shared<LiveFrame> +
//  rgba.resize(w*h*4)：1080p60 = 500MB/s 堆分配/释放（Windows 大块走
//  VirtualAlloc/decommit 路径）+ 每帧 8MB 新页 first-touch 缺页零化
//  ——池化后缓冲常驻，两项开销双双消失（同分辨率 resize 恒 no-op）。
//
//  机制：
//    * acquire 产出带归池 deleter 的 shared_ptr——最后一个引用释放
//     （Feed 历史裁剪 / 排程缓冲弹出 / 渲染端 keep 释放）时帧自动回池，
//      持有方全链（LiveHub::Feed / sync_buffer / FrameView.keep）无感；
//    * 回收判据天然安全：deleter 触发 = 引用计数归零 = 无任何持有者，
//      不存在"渲染中帧被复用"窗口；
//    * 池自持（Recycler 捕获 shared_ptr）——Ingest 析构后仍在外的帧
//      归池回调依然安全，池活到最后一个帧消亡。
//
//  线程契约：acquire/recycle 任意线程（mutex 保护）；每生产点独立池
//  （同源分辨率恒定 → rgba 恒命中零 realloc）。
// ============================================================================

#pragma once

#include <cstddef>
#include <memory>
#include <mutex>
#include <vector>

#include "live/LiveFrame.hpp"

namespace ms {

class LiveFramePool : public std::enable_shared_from_this<LiveFramePool> {
public:
    using Ptr = std::shared_ptr<LiveFramePool>;

    /// 创建（max_idle = 空闲驻留上限；8 帧 @1080p ≈ 66MB，防突发膨胀）
    static Ptr create(std::size_t max_idle = 8) {
        return Ptr(new LiveFramePool(max_idle));
    }

    ~LiveFramePool() {
        for (LiveFrame* p : idle_) delete p;
    }

    LiveFramePool(const LiveFramePool&)            = delete;
    LiveFramePool& operator=(const LiveFramePool&) = delete;

    /// 取可写帧（池命中复用缓冲 / 池空新建）。调用方重写 w/h/pts_us 并
    /// 写 rgba（尺寸不变时 resize 为 no-op）；发布经 emitFrame 转常量
    /// 指针后进入"不可变快照"语义（LiveFrame.hpp 契约不变）。
    std::shared_ptr<LiveFrame> acquire() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!idle_.empty()) {
                LiveFrame* p = idle_.back();
                idle_.pop_back();
                return std::shared_ptr<LiveFrame>(p, Recycler{shared_from_this()});
            }
        }
        return std::shared_ptr<LiveFrame>(new LiveFrame(),
                                          Recycler{shared_from_this()});
    }

private:
    explicit LiveFramePool(std::size_t max_idle) : max_idle_(max_idle) {}

    /// 归池 deleter：keep 持池共享引用（保证回调期池存活——帧活多久
    /// 池就活多久，析构竞态从机制上排除）
    struct Recycler {
        Ptr keep;
        void operator()(LiveFrame* p) const { keep->recycle(p); }
    };

    void recycle(LiveFrame* p) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (idle_.size() < max_idle_)
            idle_.push_back(p);
        else
            delete p;
    }

    std::mutex              mutex_;
    std::vector<LiveFrame*> idle_;   ///< 空闲帧（独占——deleter 触发=无引用）
    std::size_t             max_idle_;
};

} // namespace ms
