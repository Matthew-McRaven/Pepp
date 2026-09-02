// Launcher for Emscripten test binaries under node.
//
// We build every wasm executable with -sMODULARIZE -sEXPORT_NAME=<target>_entry.
// So the emitted js file only defines+exports a factory function.
// We actually need to call into that factory to execute the real program.
//
// QCoreApplication crashes when run headlessly because it looks for `window`.
// Specifically, QWasmTimer::clearTimeout() on application shutdown.
// We will just mock out `window` so the tests will have a chance to run + actually pass.
if (typeof globalThis.window === 'undefined')
    globalThis.window = globalThis;

const path = require('node:path');

const modulePath = process.argv[2];
if (!modulePath) {
    console.error('usage: wasm_test_runner.js <module.js> [args...]');
    process.exit(64);
}

const resolved = path.resolve(modulePath);
const factory = require(resolved);

// Catch reads argv[0] for its usage strings; give it the name minus the .js.
const thisProgram = resolved.replace(/\.js$/, '');

const setExit = code => {
    process.exitCode = typeof code === 'number' ? code : 0;
};

factory({
    thisProgram,
    arguments: process.argv.slice(3),
    print: text => console.log(text),
    printErr: text => console.error(text),
    quit: setExit,
    onExit: setExit
}).catch(e => {
    // A normal exit() unwinds as ExitStatus rather than resolving the promise.
    if (e && e.name === 'ExitStatus')
        return setExit(e.status);
    console.error('wasm_test_runner: module failed:', e);
    setExit(70);
});
