export default {
    roots: ['test'],
    setupFilesAfterEnv: ['./jest.setup.ts'],
    moduleFileExtensions: ['ts', 'js'],
    testPathIgnorePatterns: ['node_modules/'],
    extensionsToTreatAsEsm: ['.ts'],
    transform: {
        '^.+\\.ts?$': 'ts-jest',
    },
    testMatch: ['**/*.test.(js|ts|ts)'],
    globals: {
        'ts-jest': {
            useESM: true,
        },
    },
};
