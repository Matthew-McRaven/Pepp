/* eslint-disable import/no-extraneous-dependencies */
import { defineConfig } from 'vite';
import typescript from '@rollup/plugin-typescript';
import { resolve } from 'path';

export default defineConfig(() => ({
  plugins: [typescript()],
  build: {
    lib: {
      formats: ['es', 'umd'],
      entry: resolve(__dirname, 'src/index.ts'),
      name: 'macro',
      fileName: (format) => `index.${format}.js`,
    },
    test: {
      globals: true,
    },
  },
}));
