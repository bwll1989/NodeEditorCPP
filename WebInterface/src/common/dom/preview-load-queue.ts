type PreviewTask = () => Promise<void>;

/** 限制卡片选择器预览的并行加载数量，避免一次性初始化全部卡片。 */
class PreviewLoadQueue {
  private active = 0;

  private readonly pending: PreviewTask[] = [];

  constructor(private readonly maxConcurrent = 2) {}

  enqueue(task: PreviewTask): void {
    this.pending.push(task);
    this.pump();
  }

  private pump(): void {
    while (this.active < this.maxConcurrent && this.pending.length > 0) {
      const task = this.pending.shift();
      if (!task) return;
      this.active += 1;
      void task().finally(() => {
        this.active -= 1;
        this.pump();
      });
    }
  }
}

export const pickerPreviewQueue = new PreviewLoadQueue(3);
