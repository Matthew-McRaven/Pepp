/* eslint-disable import/no-extraneous-dependencies */
import { defineConfig } from 'vite';
import { resolve } from 'path';

export default defineConfig(() => ({
    build: {
        lib: {
            formats: ['es', 'umd'],
            entry: resolve(__dirname, 'src/asty.js'),
            name: 'asty',
            fileName: (format) => `index.${format}.js`,
        },
    },
    test: {
        environment: 'jsdom',
        globals: true,
    },
}));
