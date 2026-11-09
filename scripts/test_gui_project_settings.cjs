const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');
const vm = require('node:vm');
const { execFileSync } = require('node:child_process');
const { readProjectSettings } = require('../frontend/project-settings');

function fixture(t) {
    const folder = fs.mkdtempSync(path.join(os.tmpdir(), 'pg-gui-settings-'));
    t.after(() => fs.rmSync(folder, { recursive: true, force: true }));
    const write = (name, value) => { const file = path.join(folder, name); fs.mkdirSync(path.dirname(file), { recursive: true }); fs.writeFileSync(file, value); };
    const defaults = { rootDebug: { scope: 'PBXProject//Debug', settings: {} }, rootRelease: { scope: 'PBXProject//Release', settings: {} },
        debug: { scope: 'PBXNativeTarget/demo/Debug', settings: { GCC_PREPROCESSOR_DEFINITIONS: ['$(inherited)', 'ADDON_GENERATED=1'] } },
        release: { scope: 'PBXNativeTarget/demo/Release', settings: { GCC_PREPROCESSOR_DEFINITIONS: ['$(inherited)', 'ADDON_GENERATED=1'] } } };
    const xcconfig = 'MARKETING_VERSION = 1.2\nCURRENT_PROJECT_VERSION = 7\nPRODUCT_BUNDLE_IDENTIFIER = com.example.from_config\nUSER_PREPROCESSOR_DEFINITIONS = CORE_DEFAULT=1\nGCC_PREPROCESSOR_DEFINITIONS = $(inherited) $(USER_PREPROCESSOR_DEFINITIONS)\n#include? "App.xcconfig"\n';
    const project = { rootObject: 'project', objects: {
        project: { isa: 'PBXProject', targets: ['app'], buildConfigurationList: 'projectConfigs' },
        projectConfigs: { isa: 'XCConfigurationList', buildConfigurations: ['rootDebug', 'rootRelease'] },
        rootDebug: { isa: 'XCBuildConfiguration', name: 'Debug', buildSettings: { GCC_PREPROCESSOR_DEFINITIONS: ['ROOT_CUSTOM=1'] } },
        rootRelease: { isa: 'XCBuildConfiguration', name: 'Release', buildSettings: { GCC_PREPROCESSOR_DEFINITIONS: ['ROOT_CUSTOM=1'], MARKETING_VERSION: '9.9' } },
        app: { isa: 'PBXNativeTarget', name: 'demo', productType: 'com.apple.product-type.application', buildConfigurationList: 'appConfigs', buildPhases: ['frameworks'] },
        appConfigs: { isa: 'XCConfigurationList', buildConfigurations: ['debug', 'release'] },
        configFile: { isa: 'PBXFileReference', path: 'Project.xcconfig', sourceTree: '<group>' },
        frameworks: { isa: 'PBXFrameworksBuildPhase', files: ['cocoaBuild', 'sqliteBuild', 'bluetoothBuild', 'addonBuild'] },
        cocoaBuild: { isa: 'PBXBuildFile', fileRef: 'cocoa' }, cocoa: { isa: 'PBXFileReference', path: 'System/Library/Frameworks/Cocoa.framework', sourceTree: 'SDKROOT' },
        sqliteBuild: { isa: 'PBXBuildFile', fileRef: 'sqlite' }, sqlite: { isa: 'PBXFileReference', path: 'usr/lib/libsqlite3.tbd', sourceTree: 'SDKROOT' },
        bluetoothBuild: { isa: 'PBXBuildFile', fileRef: 'bluetooth' }, bluetooth: { isa: 'PBXFileReference', path: 'System/Library/Frameworks/CoreBluetooth.framework', sourceTree: 'SDKROOT' },
        addonBuild: { isa: 'PBXBuildFile', fileRef: 'addon' }, addon: { isa: 'PBXFileReference', path: 'local_addons/ThirdParty.framework', sourceTree: 'SOURCE_ROOT' },
    } };
    for (const [id, version, build, define] of [['debug', '0.2', '3', 'DEBUG_CUSTOM=1'], ['release', '0.1', '2', 'RELEASE_CUSTOM=1']]) {
        project.objects[id] = { isa: 'XCBuildConfiguration', name: id === 'debug' ? 'Debug' : 'Release', baseConfigurationReference: 'configFile',
            buildSettings: { MARKETING_VERSION: version, CURRENT_PROJECT_VERSION: build, PRODUCT_BUNDLE_IDENTIFIER: 'com.example.native', INFOPLIST_FILE: 'Info.plist',
                GCC_PREPROCESSOR_DEFINITIONS: ['$(inherited)', 'ADDON_GENERATED=1', define] } };
    }
    write('config.make', 'APP_VERSION = 1.0\nAPP_BUILD = 1\nAPP_BUNDLE_ID = com.example.stale\nPROJECT_DEFINES = MAKE_ONLY=1\nPROJECT_DEFINES += MAKE_APPEND=1 # comment\nPROJECT_FRAMEWORKS = Security.framework\nPROJECT_SYSTEM_LIBS = libz.tbd\n');
    write('Project.xcconfig', xcconfig);
    write('App.xcconfig', 'USER_PREPROCESSOR_DEFINITIONS = CORE_DEFAULT=1 APP_CUSTOM=1\n');
    write('Info.plist', '<?xml version="1.0"?><plist version="1.0"><dict><key>CFBundleShortVersionString</key><string>$(MARKETING_VERSION)</string><key>CFBundleVersion</key><string>$(CURRENT_PROJECT_VERSION)</string><key>CFBundleIdentifier</key><string>$(PRODUCT_BUNDLE_IDENTIFIER)</string><key>CustomDate</key><date>2025-01-01T00:00:00Z</date></dict></plist>');
    // A renamed .xcodeproj must be found even when it does not match the folder name.
    const projectPath = path.join(folder, 'Renamed.xcodeproj/project.pbxproj');
    write('Renamed.xcodeproj/project.pbxproj', JSON.stringify(project));
    write('.ofprojectgenerator/apple-settings.json', JSON.stringify({ format: 1, platforms: { osx: {
        files: { 'Project.xcconfig': xcconfig }, configurations: defaults,
        systemReferences: { app: { scope: 'demo', references: { 'frameworks:Cocoa': {} } } },
    } } }));
    return { folder, write, project, projectPath };
}

test('existing Xcode values populate the fields ahead of stale config.make values', (t) => {
    const f = fixture(t);
    const values = readProjectSettings(f.folder, { platforms: ['osx'] });
    assert.equal(values.version, '0.1'); assert.equal(values.build, '2'); assert.equal(values.bundleId, 'com.example.native');
    assert.deepEqual(new Set(values.defines), new Set(['MAKE_ONLY=1', 'MAKE_APPEND=1', 'ROOT_CUSTOM=1', 'APP_CUSTOM=1', 'DEBUG_CUSTOM=1', 'RELEASE_CUSTOM=1']));
    assert.deepEqual(new Set(values.frameworks), new Set(['Security', 'CoreBluetooth']));
    assert.deepEqual(new Set(values.libs), new Set(['z', 'sqlite3']));
});

test('target xcconfig and App.xcconfig override project-level defaults', (t) => {
    const f = fixture(t);
    delete f.project.objects.release.buildSettings.MARKETING_VERSION;
    delete f.project.objects.release.buildSettings.CURRENT_PROJECT_VERSION;
    f.write('App.xcconfig', 'MARKETING_VERSION = 0.9\nCURRENT_PROJECT_VERSION = 12\n');
    f.write('Renamed.xcodeproj/project.pbxproj', JSON.stringify(f.project));
    const values = readProjectSettings(f.folder, { platforms: ['osx'] });
    assert.equal(values.version, '0.9'); assert.equal(values.build, '12');
});

test('literal Info.plist values and binary/OpenStep projects are read without rewriting files', { skip: process.platform !== 'darwin' }, (t) => {
    const f = fixture(t);
    f.write('Info.plist', '<?xml version="1.0"?><plist version="1.0"><dict><key>CFBundleShortVersionString</key><string>0.4</string><key>CFBundleVersion</key><string>8</string><key>CFBundleIdentifier</key><string>com.example.literal</string><key>CustomDate</key><date>2025-01-01T00:00:00Z</date></dict></plist>');
    const openstep = (value) => Array.isArray(value) ? '(' + value.map(openstep).join(',') + ')'
        : value && typeof value === 'object' ? '{' + Object.entries(value).map(([key, item]) => JSON.stringify(key) + '=' + openstep(item) + ';').join('\n') + '}' : JSON.stringify(String(value));
    f.write('Renamed.xcodeproj/project.pbxproj', '// !$*UTF8*$!\n' + openstep(f.project));
    const info = path.join(f.folder, 'Info.plist');
    execFileSync('/usr/bin/plutil', ['-convert', 'binary1', '--', info]);
    const before = fs.readFileSync(info);
    const values = readProjectSettings(f.folder, { platforms: ['osx'] });
    assert.equal(values.version, '0.4'); assert.equal(values.build, '8'); assert.equal(values.bundleId, 'com.example.literal');
    assert.deepEqual(fs.readFileSync(info), before);
});

test('make projects load their libraries, defines and bundle ID without an Xcode project', (t) => {
    const f = fixture(t);
    const values = readProjectSettings(f.folder, { platforms: ['linux64'] });
    assert.equal(values.version, '1.0'); assert.equal(values.build, '1'); assert.equal(values.bundleId, 'com.example.stale');
    assert.deepEqual(values.defines, ['MAKE_ONLY=1', 'MAKE_APPEND=1']);
    assert.deepEqual(values.frameworks, ['Security']); assert.deepEqual(values.libs, ['z']);
});

test('metadata follows the app target matching the selected Apple platform', (t) => {
    const f = fixture(t);
    f.project.objects.release.buildSettings.SDKROOT = 'macosx';
    f.project.objects.project.targets.push('iosApp');
    f.project.objects.iosApp = { ...f.project.objects.app, name: 'iosApp', buildConfigurationList: 'iosConfigs' };
    f.project.objects.iosConfigs = { isa: 'XCConfigurationList', buildConfigurations: ['iosRelease'] };
    f.project.objects.iosRelease = { ...f.project.objects.release, buildSettings: { ...f.project.objects.release.buildSettings,
        SDKROOT: 'iphoneos', MARKETING_VERSION: '0.8', CURRENT_PROJECT_VERSION: '20', PRODUCT_BUNDLE_IDENTIFIER: 'com.example.ios' } };
    f.write('Renamed.xcodeproj/project.pbxproj', JSON.stringify(f.project));
    assert.equal(readProjectSettings(f.folder, { platforms: ['osx'] }).version, '0.1');
    const ios = readProjectSettings(f.folder, { platforms: ['ios'] });
    assert.equal(ios.version, '0.8'); assert.equal(ios.build, '20'); assert.equal(ios.bundleId, 'com.example.ios');
});

test('separate -D arguments load custom defines while generated addon flags stay excluded', (t) => {
    const f = fixture(t);
    f.project.objects.release.buildSettings.OTHER_CFLAGS = ['-D', 'GENERATED=1', '-D', 'USER_CFLAG=1'];
    f.write('Renamed.xcodeproj/project.pbxproj', JSON.stringify(f.project));
    const file = path.join(f.folder, '.ofprojectgenerator/apple-settings.json');
    const state = JSON.parse(fs.readFileSync(file));
    state.platforms.osx.configurations.release.settings.OTHER_CFLAGS = ['-D', 'GENERATED=1'];
    fs.writeFileSync(file, JSON.stringify(state));
    const values = readProjectSettings(f.folder, { platforms: ['osx'] });
    assert.ok(values.defines.includes('USER_CFLAG=1')); assert.ok(!values.defines.includes('GENERATED=1'));
});

test('renderer populates fields, keeps unedited values implicit, and sends deliberate edits', () => {
    const handlers = {}, elements = new Map(), sent = [];
    const element = (selector) => {
        if (elements.has(selector)) return elements.get(selector);
        const state = { value: '', dropdownValue: 'default', label: '', length: 0,
            val(value) { if (value === undefined) return this.value; this.value = value; return this; },
            text(value) { if (value === undefined) return this.label; this.label = value; return this; },
            dropdown(action, value) { if (action === 'get value') return this.dropdownValue; if (value !== undefined) this.dropdownValue = value; return this; },
            css() { return selector === '#systemFrameworksField' ? 'flex' : 'none'; },
            hide() { return this; }, show() { return this; }, toggle() { return this; }, oneTimeTooltip() { throw new Error('Unexpected form validation error'); },
        };
        elements.set(selector, state); return state;
    };
    const ipc = { on(channel, handler) { handlers[channel] = handler; }, send(channel, data) { sent.push({ channel, data }); }, path, fs };
    const context = vm.createContext({ window: { ipcWrapper: ipc }, document: {}, console, setTimeout, clearTimeout, $: element });
    vm.runInContext(fs.readFileSync(path.join(__dirname, '../frontend/app.js'), 'utf8'), context);
    vm.runInContext('getPlatformList = () => ["osx"]; openConsoleForOperation = () => {};', context);
    element('#projectName').val('demo'); element('#projectPath').val('/tmp');
    handlers.setProjectSettings(null, { projectName: 'demo', projectPath: '/tmp', version: '0.1', build: '2', bundleId: 'com.example.demo', defines: ['CUSTOM=1'], frameworks: ['CoreBluetooth'], libs: ['sqlite3'] });
    assert.equal(element('#appVersion').val(), '0.1'); assert.equal(element('#appBuild').val(), '2');
    assert.equal(element('#bundleId').val(), 'com.example.demo'); assert.equal(element('#customDefines').val(), 'CUSTOM=1');
    assert.equal(element('#systemLibs').val(), 'sqlite3'); assert.equal(element('#appVersionNote').text(), '');
    vm.runInContext('generate()', context);
    const first = sent.findLast((entry) => entry.channel === 'generate').data;
    assert.equal(first.appVersion, ''); assert.equal(first.appBuild, ''); assert.equal(first.appBundleId, '');
    assert.equal(first.defines, null); assert.equal(first.systemFrameworks, null); assert.equal(first.systemLibs, null);
    const frameworks = ['Foundation', 'AppKit', 'AVFoundation', 'Security', 'UserNotifications', 'NotificationCenter'];
    handlers.setProjectSettings(null, { projectName: 'demo', projectPath: '/tmp', version: '0.1', build: '2', bundleId: 'com.example.demo', defines: ['CUSTOM=1'], frameworks, libs: ['sqlite3'] });
    assert.equal(element('#systemFrameworks').val(), frameworks.join(', '));
    vm.runInContext('generate()', context);
    assert.equal(sent.findLast((entry) => entry.channel === 'generate').data.systemFrameworks, null);
    element('#systemFrameworks').val(frameworks.join(','));
    vm.runInContext('generate()', context);
    assert.equal(sent.findLast((entry) => entry.channel === 'generate').data.systemFrameworks, null, 'spacing-only edits must not override per-target libraries');
    element('#systemFrameworks').val('Foundation, AppKit, AVFoundation, Security, UserNotifications, NotificationCenter,  CoreBluetooth');
    vm.runInContext('generate()', context);
    assert.deepEqual(Array.from(sent.findLast((entry) => entry.channel === 'generate').data.systemFrameworks), [...frameworks, 'CoreBluetooth']);
    element('#systemFrameworks').val(frameworks.join(', '));
    element('#appVersion').val('0.2'); element('#customDefines').val(''); element('#systemLibs').val('z');
    vm.runInContext('generate()', context);
    const edited = sent.findLast((entry) => entry.channel === 'generate').data;
    assert.equal(edited.appVersion, '0.2'); assert.equal(edited.appBuild, ''); assert.equal(edited.defines, '');
    assert.deepEqual(Array.from(edited.systemLibs), ['z']);
    handlers.setProjectSettings(null, { projectName: 'demo', projectPath: '/tmp', preserveEdits: true,
        version: '0.3', build: '4', bundleId: 'com.example.refreshed', defines: ['UPDATED=1'], frameworks: ['Security'], libs: ['c++'] });
    assert.equal(element('#appVersion').val(), '0.2'); assert.equal(element('#appBuild').val(), '4');
    assert.equal(element('#customDefines').val(), ''); assert.equal(element('#systemLibs').val(), 'z');
    assert.equal(element('#systemFrameworks').val(), 'Security');
    vm.runInContext('generate()', context);
    const refreshed = sent.findLast((entry) => entry.channel === 'generate').data;
    assert.equal(refreshed.appVersion, '0.2'); assert.equal(refreshed.appBuild, '');
    assert.equal(refreshed.defines, ''); assert.equal(refreshed.systemFrameworks, null);
    // A late response from a previously selected project cannot replace the current fields.
    handlers.setProjectSettings(null, { projectName: 'other', projectPath: '/tmp', version: '99' });
    assert.equal(element('#appVersion').val(), '0.2');
    handlers.setProjectSettings(null, { projectName: 'demo', projectPath: '/tmp', defines: ['CUSTOM=1', 'OF_USE_ANGLE=1'] });
    element('#rendererDropdown').dropdownValue = 'dawn';
    vm.runInContext('generate()', context);
    const renderer = sent.findLast((entry) => entry.channel === 'generate').data.defines;
    assert.ok(renderer.includes('OF_USE_DAWN=1')); assert.ok(!renderer.includes('OF_USE_ANGLE=1'));
    vm.runInContext('clearProjectSettings()', context);
    assert.equal(element('#customDefines').val(), ''); assert.equal(element('#bundleId').val(), '');
});

test('main process omits implicit overrides and passes an explicitly empty defines field', () => {
    const contents = fs.readFileSync(path.join(__dirname, '../frontend/index.js'), 'utf8');
    const start = contents.indexOf('function generateFunction(');
    const end = contents.indexOf("ipcMain.on('generate'", start);
    let args;
    const context = vm.createContext({ path, runPG(value) { args = value; } });
    vm.runInContext(contents.slice(start, end), context);
    context.request = { projectName: 'demo', projectPath: '/tmp', platformList: ['osx'], ofPath: '/of', defines: null, appVersion: '', appBuild: '', appBundleId: '', systemLibs: null, systemFrameworks: null };
    vm.runInContext('generateFunction({}, request)', context);
    assert.ok(!args.some((arg) => /^(--defines|--app-version|--app-build|--bundle-id|--system-libs|-f)/.test(arg)));
    context.request.defines = ''; context.request.systemLibs = [];
    vm.runInContext('generateFunction({}, request)', context);
    assert.ok(args.includes('--defines=')); assert.ok(args.includes('--system-libs='));
});
