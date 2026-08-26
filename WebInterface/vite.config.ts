import { defineConfig } from "vite";
import { resolve } from "path";

export default defineConfig({
  base: "./",
  root: ".",
  publicDir: "public",
  define: {
    "process.env.NODE_ENV": JSON.stringify("production"),
  },
  build: {
    outDir: "dist",
    emptyOutDir: true,
    rollupOptions: {
      input: {
        main: resolve(__dirname, "index.html"),
      },
      output: {
        manualChunks(id) {
          if (id.includes("node_modules/@mdi/js")) {
            return "mdi-icons";
          }
          if (id.includes("node_modules/echarts") || id.includes("node_modules/echarts-gl")) {
            return "echarts";
          }
        },
      },
    },
  },
  server: {
    port: 5173,
    proxy: {
      "/api": { target: "http://127.0.0.1:8992", changeOrigin: true },
      "/ws": { target: "ws://127.0.0.1:8992", ws: true },
    },
  },
});
