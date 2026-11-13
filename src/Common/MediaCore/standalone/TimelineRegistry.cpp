// ============================================================================
//  TimelineRegistry.cpp —— standalone 多时间轴注册表实现
//
//  全部方法一把 mutex_ 串行化（见头文件线程模型）；条目本体以
//  shared_ptr<const TimelineEntry> 存储——add/update 生成新快照替换槽位，
//  调用方经 find()/active() 拿到的指针在并发变更下永不悬垂。
// ============================================================================

#include "standalone/TimelineRegistry.hpp"

#include <algorithm>
#include <utility>

namespace ms::standalone {

// ---------------------------------------------------------------------------
// 内部工具
// ---------------------------------------------------------------------------
std::shared_ptr<const TimelineEntry> TimelineRegistry::findByIdLocked(
    const std::string& id) const {
    for (const auto& e : entries_)
        if (e->id == id) return e;
    return nullptr;
}

// ---------------------------------------------------------------------------
// CRUD
// ---------------------------------------------------------------------------
std::string TimelineRegistry::add(TimelineEntry entry) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (entry.id.empty()) {
        // 生成不撞的递增 id（持久化恢复的条目可能已占用 tl-N 前段——
        // 生成循环以"查无此人"为准，next_seq_ 只保证单调不保证无洞）
        do {
            ++next_seq_;
            entry.id = "tl-" + std::to_string(next_seq_);
        } while (findByIdLocked(entry.id) != nullptr);
    } else if (findByIdLocked(entry.id) != nullptr) {
        return {}; // id 冲突：调用方决定改名/报错，注册表零副作用
    }
    auto stored = std::make_shared<const TimelineEntry>(std::move(entry));
    entries_.push_back(stored);
    return stored->id;
}

bool TimelineRegistry::update(const TimelineEntry& entry) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& slot : entries_) {
        if (slot->id != entry.id) continue;
        slot = std::make_shared<const TimelineEntry>(entry); // 快照整体替换
        return true;
    }
    return false;
}

bool TimelineRegistry::remove(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = std::remove_if(
        entries_.begin(), entries_.end(),
        [&id](const std::shared_ptr<const TimelineEntry>& e) {
            return e->id == id;
        });
    if (it == entries_.end()) return false;
    entries_.erase(it, entries_.end());
    // 被删条目的登记自动复位（注册表自洽；NodeController 在切换编排时
    // 读到空活跃/空默认即走"无播"分支，无需额外通知）
    if (active_id_ == id) active_id_.clear();
    if (default_id_ == id) default_id_.clear();
    return true;
}

std::vector<TimelineEntry> TimelineRegistry::list() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<TimelineEntry> out;
    out.reserve(entries_.size());
    for (const auto& e : entries_) out.push_back(*e);
    return out;
}

std::shared_ptr<const TimelineEntry> TimelineRegistry::find(
    const std::string& id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return findByIdLocked(id);
}

std::size_t TimelineRegistry::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return entries_.size();
}

// ---------------------------------------------------------------------------
// 唯一活跃仲裁
// ---------------------------------------------------------------------------
bool TimelineRegistry::setActive(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (findByIdLocked(id) == nullptr) return false; // 不存在：登记不变
    active_id_ = id;
    return true;
}

void TimelineRegistry::clearActive() {
    std::lock_guard<std::mutex> lock(mutex_);
    active_id_.clear();
}

std::shared_ptr<const TimelineEntry> TimelineRegistry::active() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return findByIdLocked(active_id_);
}

std::string TimelineRegistry::activeId() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return active_id_;
}

// ---------------------------------------------------------------------------
// 默认时间轴
// ---------------------------------------------------------------------------
bool TimelineRegistry::setDefault(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (findByIdLocked(id) == nullptr) return false;
    default_id_ = id;
    return true;
}

void TimelineRegistry::clearDefault() {
    std::lock_guard<std::mutex> lock(mutex_);
    default_id_.clear();
}

std::string TimelineRegistry::defaultId() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return default_id_;
}

std::shared_ptr<const TimelineEntry> TimelineRegistry::defaultEntry() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return findByIdLocked(default_id_);
}

} // namespace ms::standalone
