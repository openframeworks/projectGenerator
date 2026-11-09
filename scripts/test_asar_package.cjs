// Run after packaging, on the same host platform/architecture as the bundled generator:
// node scripts/test_asar_package.cjs /path/to/app/resources
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const { pathToFileURL } = require('node:url');
const { spawnSync } = require('node:child_process');
const { bundledGeneratorPath } = require('../frontend/packaged-paths');

async function main() {
    assert.ok(process.argv[2], 'Pass the packaged app resources directory');
    const archive = path.resolve(process.argv[2], 'app.asar');
    const asar = await import(pathToFileURL(require.resolve('@electron/asar', { paths: [path.join(__dirname, '../frontend')] })).href);
    for (const file of ['index.html', 'index.js', 'app.js', 'preload.js', 'packaged-paths.js', 'locales/en.json']) {
        assert.ok(asar.extractFile(archive, file).length, `Missing ${file}`);
        assert.ok(!asar.statFile(archive, file).unpacked, `${file} should be packed`);
    }
    assert.equal(asar.extractFile(archive, 'packaged-paths.js').toString(), fs.readFileSync(path.join(__dirname, '../frontend/packaged-paths.js'), 'utf8'));
    const generator = bundledGeneratorPath(archive);
    const relative = 'app/' + path.basename(generator);
    assert.equal(asar.statFile(archive, relative).unpacked, true, 'Native generator must be unpacked');
    const result = spawnSync(generator, ['--version'], { encoding: 'utf8', timeout: 30000, env: { ...process.env, TZ: 'UTC0' } });
    if (result.error) throw result.error;
    assert.equal(result.status, 0, result.stderr || result.stdout);
    assert.match(result.stdout, /version/);
    console.log('ASAR assets, unpacked native generator and spawn --version passed');
    console.log(result.stdout.trim());
}

main().catch((error) => { console.error(error); process.exitCode = 1; });
