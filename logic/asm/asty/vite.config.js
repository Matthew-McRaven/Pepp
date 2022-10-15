/* eslint-disable import/no-extraneous-dependencies */
const { defineConfig }  = require('vite');
const { resolve } = require('path');

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
