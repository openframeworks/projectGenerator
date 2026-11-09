const fs = require('node:fs');
const path = require('node:path');

function bundledGeneratorPath(appDirectory, platform = process.platform) {
    const directory = path.basename(appDirectory) === 'app.asar' ? appDirectory + '.unpacked' : appDirectory;
    return path.join(directory, 'app', platform === 'win32' ? 'projectGenerator.exe' : 'projectGenerator');
}

function settingsStore(appDirectory, userDataDirectory, packaged) {
    const filename = path.join(packaged ? userDataDirectory : appDirectory, 'settings.json');
    const candidates = [filename];
    if (packaged) {
        candidates.push(path.join(appDirectory, 'settings.json'));
        if (path.basename(appDirectory) === 'app.asar') candidates.push(path.join(path.dirname(appDirectory), 'app', 'settings.json'));
    }
    return {
        filename,
        read() {
            for (const file of candidates) {
                try { return JSON.parse(fs.readFileSync(file, 'utf8')); }
                catch (error) { if (error.code !== 'ENOENT') throw error; }
            }
            throw Object.assign(new Error('No saved settings'), { code: 'ENOENT' });
        },
        write(contents) {
            fs.mkdirSync(path.dirname(filename), { recursive: true });
            fs.writeFileSync(filename, contents);
        },
    };
}

module.exports = { bundledGeneratorPath, settingsStore };
