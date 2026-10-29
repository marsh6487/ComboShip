#pragma once

#include <memory>

struct CollisionContext;
struct BgCheckSnapshotData;
struct BgCheckRestoreData;

class BgCheckStateRestore {
  public:
    ~BgCheckStateRestore();
    bool Validate() const noexcept;
    // Call immediately after the raw heap copy, after Validate succeeded, without running game code between.
    void Commit() noexcept;

  private:
    friend class BgCheckStateSnapshot;
    explicit BgCheckStateRestore(std::unique_ptr<BgCheckRestoreData> data);
    std::unique_ptr<BgCheckRestoreData> data;
};

class BgCheckStateSnapshot {
  public:
    static std::unique_ptr<BgCheckStateSnapshot> Capture(CollisionContext* context) noexcept;
    bool Matches(const CollisionContext* context) const noexcept;
    std::unique_ptr<BgCheckStateRestore> Prepare(CollisionContext* context) const noexcept;

  private:
    explicit BgCheckStateSnapshot(std::shared_ptr<BgCheckSnapshotData> data);
    std::shared_ptr<BgCheckSnapshotData> data;
};
