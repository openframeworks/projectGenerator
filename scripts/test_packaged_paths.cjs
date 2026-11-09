const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');
const vm = require('node:vm');
const { bundledGeneratorPath, settingsStore } = require('../frontend/packaged-paths');

function fixture(t) {
    const folder = fs.mkdtempSync(path.join(os.tmpdir(), 'pg-packaged-paths-'));
    t.after(() => fs.rmSync(folder, { recursive: true, force: true }));
    return { app: path.join(folder, 'resources', 'app.asar'), userData: path.join(folder, 'preferences') };
}

test('native generators use real unpacked paths on all platforms and development paths stay direct', () => {
    const app = path.resolve('resources', 'app.asar');
    for (const platform of ['darwin', 'linux', 'win32']) {
        const binary = platform === 'win32' ? 'projectGenerator.exe' : 'projectGenerator';
        assert.equal(bundledGeneratorPath(app, platform), path.join(app + '.unpacked', 'app', binary));
        assert.equal(bundledGeneratorPath(path.resolve('frontend'), platform), path.resolve('frontend', 'app', binary));
    }
});

test('packaged preferences persist outside the archive and survive replacing the app directory', (t) => {
    const f = fixture(t);
    const store = settingsStore(f.app, f.userData, true);
    assert.throws(() => store.read(), { code: 'ENOENT' });
    store.write(JSON.stringify({ defaultOfPath: '/my/OF', advancedMode: true }));
    assert.deepEqual(store.read(), { defaultOfPath: '/my/OF', advancedMode: true });
    assert.ok(!fs.existsSync(f.app));
    const updated = settingsStore(path.join(path.dirname(f.app), 'replacement', 'app.asar'), f.userData, true);
    assert.deepEqual(updated.read(), store.read());
});

test('legacy unpacked preferences migrate on save without changing their original file', (t) => {
    const f = fixture(t);
    const legacy = path.join(path.dirname(f.app), 'app', 'settings.json');
    fs.mkdirSync(path.dirname(legacy), { recursive: true });
    fs.writeFileSync(legacy, '{"language":"ja"}');
    const store = settingsStore(f.app, f.userData, true);
    assert.deepEqual(store.read(), { language: 'ja' });
    assert.ok(!fs.existsSync(store.filename));
    store.write('{"language":"fr"}');
    assert.deepEqual(store.read(), { language: 'fr' });
    assert.equal(fs.readFileSync(legacy, 'utf8'), '{"language":"ja"}');
});

test('development settings stay alongside the source and invalid saved settings are reported', (t) => {
    const f = fixture(t);
    const directory = path.join(path.dirname(f.app), 'frontend');
    const store = settingsStore(directory, f.userData, false);
    store.write('{"advancedMode":true}');
    assert.equal(store.filename, path.join(directory, 'settings.json'));
    assert.deepEqual(store.read(), { advancedMode: true });
    store.write('{invalid');
    assert.throws(() => store.read(), SyntaxError);
    assert.ok(!fs.existsSync(f.userData));
});

test('main process uses the bundled generator for packaged Linux and preserves the development override', () => {
    const source = fs.readFileSync(path.join(__dirname, '../frontend/index.js'), 'utf8');
    const start = source.indexOf('function getPgPath()');
    const end = source.indexOf('// clear error', start);
    for (const platform of ['darwin', 'linux', 'win32']) {
        const appDirectory = path.resolve('resources', 'app.asar');
        const app = { isPackaged: true };
        const context = vm.createContext({ path, console, app, hostplatform: platform === 'linux' ? 'linux64' : platform === 'win32' ? 'windows' : 'osx',
            defaultOfPath: '/of', __dirname: appDirectory, bundledGeneratorPath: (folder) => bundledGeneratorPath(folder, platform) });
        vm.runInContext(source.slice(start, end), context);
        assert.equal(vm.runInContext('getPgPath()', context), bundledGeneratorPath(appDirectory, platform));
        if (platform === 'linux') {
            app.isPackaged = false;
            assert.equal(vm.runInContext('getPgPath()', context), path.join('/of', 'apps/projectGenerator/commandLine/bin/projectGenerator'));
        }
    }
});
