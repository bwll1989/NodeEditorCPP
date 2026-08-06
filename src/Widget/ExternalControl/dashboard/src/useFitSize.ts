import { onBeforeUnmount, onMounted, ref, type Ref } from 'vue';

/**
 * Observe a container and derive pixel sizes without CSS container-query units
 * (cqw/cqh), which are missing on older Chromium / Safari / Firefox.
 */
export function useContainerSize(elRef: Ref<HTMLElement | null>) {
  const width = ref(0);
  const height = ref(0);
  let ro: ResizeObserver | null = null;

  function measure() {
    const el = elRef.value;
    if (!el) return;
    width.value = el.clientWidth;
    height.value = el.clientHeight;
  }

  onMounted(() => {
    const el = elRef.value;
    if (!el) return;
    measure();
    if (typeof ResizeObserver !== 'undefined') {
      ro = new ResizeObserver(measure);
      ro.observe(el);
    }
  });

  onBeforeUnmount(() => {
    ro?.disconnect();
    ro = null;
  });

  return { width, height, measure };
}
