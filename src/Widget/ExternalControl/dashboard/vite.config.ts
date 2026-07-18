import { defineConfig } from 'vite';
import vue from '@vitejs/plugin-vue';
import { resolve } from 'path';

export default defineConfig({
  plugins: [vue()],
  // IIFE 浏览器包需擦除 Node 全局，避免 echarts 等依赖访问 process 报错
  define: {
    'process.env.NODE_ENV': JSON.stringify('production'),
  },
  build: {
    lib: {
      entry: resolve(__dirname, 'src/main.ts'),
      name: 'EPWidgetsSfc',
      formats: ['iife'],
      fileName: () => 'widgets-sfc.js',
    },
    outDir: resolve(__dirname, '../www/assets'),
    emptyOutDir: false,
    rollupOptions: {
      external: ['vue'],
      output: {
        globals: { vue: 'Vue' },
        extend: true,
      },
    },
  },
});
