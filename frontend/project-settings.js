const fs = require('node:fs');
const path = require('node:path');
const { execFileSync } = require('node:child_process');

const applePlatforms = new Set(['osx', 'macos', 'ios', 'tvos']);
const unique = (items) => [...new Set(items.filter(Boolean))];
const text = (file) => { try { return fs.readFileSync(file, 'utf8'); } catch { return ''; } };
const scalar = (value) => Array.isArray(value) ? value.join(' ') : String(value ?? '');
const unquote = (value) => String(value).replace(/^(["'])(.*)\1$/, '$2');
let sdkRoots;

function installedSDKItem(type, name) {
    if (process.platform !== 'darwin') return false;
    if (!sdkRoots) sdkRoots = ['macosx', 'iphoneos', 'appletvos'].flatMap((sdk) => {
        try {
            const root = execFileSync('/usr/bin/xcrun', ['--sdk', sdk, '--show-sdk-path'],
                { encoding: 'utf8', timeout: 5000, stdio: ['ignore', 'pipe', 'ignore'] }).trim();
            return root ? [root] : [];
        } catch { return []; }
    });
    return sdkRoots.some((root) => type === 'frameworks'
        ? fs.existsSync(path.join(root, 'System/Library/Frameworks', name + '.framework'))
        : ['.tbd', '.dylib'].some((ext) => fs.existsSync(path.join(root, 'usr/lib', 'lib' + name + ext))));
}

function words(value) {
    return (scalar(value).match(/(?:[^\s"']|"(?:\\.|[^"\\])*"|'[^']*')+/g) || []).map(unquote);
}

function expand(value, settings) {
    let result = scalar(value);
    for (let i = 0; i < 10; i++) {
        const next = result.replace(/\$\(([^)]+)\)|\$\{([^}]+)\}/g, (original, a, b) => {
            const [key, modifier] = (a || b).split(':');
            if (!(key in settings)) return original;
            const resolved = scalar(settings[key]);
            return modifier === 'rfc1034identifier' ? resolved.replace(/[^A-Za-z0-9.-]/g, '-') : resolved;
        });
        if (next === result || next.length > 1024 * 1024) break;
        result = next;
    }
    return unquote(result.trim());
}

function apply(settings, key, value, context = {}) {
    const name = key.split('[')[0].trim();
    for (const condition of key.matchAll(/\[([^=]+)=([^\]]+)\]/g)) {
        const actual = condition[1] === 'config' ? context.configuration : condition[1] === 'sdk' ? context.sdk : null;
        if (actual == null) continue;
        const pattern = condition[2].replace(/[.+^${}()|[\]\\]/g, '\\$&').replace(/\*/g, '.*').replace(/\?/g, '.');
        if (!new RegExp('^' + pattern + '$').test(actual)) return;
    }
    const previous = scalar(settings[name]);
    settings[name] = Array.isArray(value)
        ? value.flatMap((item) => /^\$\(inherited\)$|^\$\{inherited\}$/.test(item) ? words(previous) : [item])
        : scalar(value).replace(/\$\(inherited\)|\$\{inherited\}/g, previous);
}

function assignments(contents, settings = {}, { file = '', context = {}, baseline = false, seen = new Set() } = {}) {
    for (const line of contents.replace(/\\\r?\n/g, ' ').split(/\r?\n/)) {
        const include = /^\s*#include\??\s*["<]([^">]+)[">]/.exec(line);
        if (include && file && (!baseline || path.basename(include[1]) === 'CoreOF.xcconfig')) {
            const included = path.resolve(path.dirname(file), expand(include[1], settings));
            if (!seen.has(included) && seen.size < 32) {
                seen.add(included);
                assignments(text(included), settings, { file: included, context, baseline, seen });
            }
            continue;
        }
        const match = /^\s*([A-Za-z_][A-Za-z0-9_]*(?:\s*\[[^\]]+\])*)\s*(\+=|:=|\?=|=)\s*(.*)$/.exec(line);
        if (!match) continue;
        const key = match[1].trim();
        // Comments begin outside quoted strings; a # in a quoted define remains data.
        const value = (match[3].match(/(?:"(?:\\.|[^"\\])*"|'[^']*'|[^#])*/)?.[0] || '').replace(/\s+\/\/.*$/, '').trim();
        if (match[2] === '?=' && key in settings) continue;
        const assigned = match[2] === '+=' ? scalar(settings[key]) + ' ' + value : value;
        apply(settings, key, assigned.trim(), context);
    }
    return settings;
}

function propertyList(file) {
    const contents = text(file).replace(/^\uFEFF/, '');
    try { return JSON.parse(contents); } catch { /* Xcode saves OpenStep; plutil reads it without running the project. */ }
    if (process.platform === 'darwin' && contents) {
        try {
            return JSON.parse(execFileSync('/usr/bin/plutil', ['-convert', 'json', '-o', '-', '--', file],
                { encoding: 'utf8', timeout: 5000, maxBuffer: 32 * 1024 * 1024, stdio: ['ignore', 'pipe', 'ignore'] }));
        } catch { /* Metadata fallback below still reads xcconfig / plist values. */ }
    }
    return null;
}

function plistString(file, key) {
    if (!file || !fs.existsSync(file)) return '';
    const json = propertyList(file);
    if (json && typeof json[key] === 'string') return json[key];
    const xml = text(file);
    const match = new RegExp('<key>\\s*' + key + '\\s*</key>\\s*<string>([\\s\\S]*?)</string>').exec(xml);
    if (match) return match[1].replace(/&#(?:x([0-9a-f]+)|(\d+));/gi, (original, hex, decimal) => {
        const code = parseInt(hex || decimal, hex ? 16 : 10);
        return code <= 0x10ffff ? String.fromCodePoint(code) : original;
    }).replace(/&lt;/g, '<').replace(/&gt;/g, '>').replace(/&quot;/g, '"').replace(/&apos;/g, "'").replace(/&amp;/g, '&');
    if (process.platform === 'darwin') {
        try { return execFileSync('/usr/bin/plutil', ['-extract', key, 'raw', '-o', '-', '--', file],
            { encoding: 'utf8', timeout: 5000, stdio: ['ignore', 'pipe', 'ignore'] }).trim(); } catch { /* Absent key. */ }
    }
    return '';
}

function projectFile(folder) {
    const expected = path.join(folder, path.basename(folder) + '.xcodeproj', 'project.pbxproj');
    if (fs.existsSync(expected)) return expected;
    try {
        const projects = fs.readdirSync(folder).filter((name) => name.endsWith('.xcodeproj')).sort();
        if (projects.length === 1) return path.join(folder, projects[0], 'project.pbxproj');
    } catch { /* Not an existing project. */ }
    return '';
}

function configurations(project) {
    const objects = project?.objects || {};
    const root = objects[project?.rootObject] || {};
    const result = [];
    for (const targetId of root.targets || []) {
        const target = objects[targetId];
        if (target?.productType !== 'com.apple.product-type.application') continue;
        const list = objects[target.buildConfigurationList]?.buildConfigurations || [];
        const ordered = [...list].sort((a, b) => (objects[a]?.name === 'Release' ? -1 : objects[b]?.name === 'Release' ? 1 : 0));
        for (const id of ordered) if (objects[id]) result.push({ id, targetId, target, configuration: objects[id], root });
    }
    return result;
}

function configFile(folder, objects, reference, settings) {
    const ref = objects[reference];
    if (!ref) return path.join(folder, 'Project.xcconfig');
    const filename = expand(ref.path || ref.name || 'Project.xcconfig', settings);
    if (path.isAbsolute(filename) || ref.sourceTree === 'SOURCE_ROOT' || fs.existsSync(path.resolve(folder, filename))) return path.resolve(folder, filename);
    const main = Object.values(objects).find((object) => object.isa === 'PBXProject')?.mainGroup;
    const pending = [[main, folder]], seen = new Set();
    while (pending.length) {
        const [id, base] = pending.pop();
        if (seen.has(id)) continue;
        seen.add(id);
        for (const childId of objects[id]?.children || []) {
            if (childId === reference) return path.resolve(base, filename);
            const child = objects[childId];
            if (child?.isa === 'PBXGroup') pending.push([childId, path.resolve(child.sourceTree === 'SOURCE_ROOT' ? folder : base, expand(child.path || '', settings))]);
        }
    }
    return path.resolve(folder, filename);
}

function effectiveSettings(folder, project, entry, baseline = null, platform = '') {
    const objects = project.objects;
    const context = { configuration: entry.configuration.name,
        sdk: platform === 'ios' ? 'iphoneos' : platform === 'tvos' ? 'appletvos' : 'macosx' };
    const settings = { SRCROOT: folder, PROJECT_DIR: folder, TARGET_NAME: entry.target.name || path.basename(folder),
        PRODUCT_NAME: entry.target.productName || entry.target.name || path.basename(folder), CONFIGURATION: context.configuration };
    const rootList = objects[entry.root.buildConfigurationList]?.buildConfigurations || [];
    const rootId = rootList.find((id) => objects[id]?.name === context.configuration);
    const rootConfig = objects[rootId];
    if (rootConfig?.baseConfigurationReference) {
        const rootFile = configFile(folder, objects, rootConfig.baseConfigurationReference, settings);
        assignments(text(rootFile), settings, { file: rootFile, context, baseline: !!baseline });
    }
    const rootValues = baseline ? baseline.configurations?.[rootId]?.settings || {} : rootConfig?.buildSettings || {};
    for (const [key, value] of Object.entries(rootValues)) apply(settings, key, value, context);
    const file = baseline ? path.join(folder, 'Project.xcconfig') : configFile(folder, objects, entry.configuration.baseConfigurationReference, settings);
    const contents = baseline ? baseline.files?.['Project.xcconfig'] || '' : text(file);
    assignments(contents, settings, { file, context, baseline: !!baseline });
    let values = entry.configuration.buildSettings || {};
    if (baseline) {
        values = baseline.configurations?.[entry.id]?.settings
            || Object.values(baseline.configurations || {}).find((config) => config.scope === `PBXNativeTarget/${entry.target.name}/${context.configuration}`)?.settings || {};
    }
    for (const [key, value] of Object.entries(values)) apply(settings, key, value, context);
    return settings;
}

function sdkReference(ref) {
    const file = String(ref?.path || ref?.name || '').replaceAll('\\', '/');
    const sdk = ref?.sourceTree === 'SDKROOT' || /(?:\$\(SDKROOT\)|\$\{SDKROOT\}|System\/Library\/Frameworks|\/usr\/lib\/)/.test(file);
    if (!sdk) return null;
    const framework = /(?:^|\/)([^/]+)\.framework(?:\/.*)?$/.exec(file);
    if (framework) return { type: 'frameworks', name: framework[1] };
    const library = /(?:^|\/)lib([^/]+)\.(?:tbd|dylib)$/.exec(file);
    return library ? { type: 'libs', name: library[1] } : null;
}

function linkedSDK(project, target) {
    const objects = project?.objects || {};
    const found = { frameworks: [], libs: [] };
    for (const id of target?.buildPhases || []) {
        if (objects[id]?.isa !== 'PBXFrameworksBuildPhase') continue;
        for (const buildId of objects[id].files || []) {
            const ref = sdkReference(objects[objects[buildId]?.fileRef]);
            if (ref) found[ref.type].push(ref.name);
        }
    }
    return found;
}

function cleanLibrary(value) { return path.basename(unquote(unquote(value).replace(/^-l/, ''))).replace(/^lib/, '').replace(/\.(?:tbd|dylib)$/, ''); }
function cleanFramework(value) { return path.basename(unquote(value)).replace(/\.framework$/, ''); }
function defineWords(value, settings) {
    const values = Array.isArray(value) ? value.flatMap((part) => words(expand(part, settings))) : words(expand(value, settings));
    return values.filter((define) => /^[A-Za-z_][A-Za-z0-9_]*(?:=.*)?$/.test(define));
}

function cflagDefines(value, settings) {
    const flags = words(expand(value, settings)), found = [];
    for (let i = 0; i < flags.length; i++) {
        if (flags[i] === '-D' && flags[i + 1]) found.push(flags[++i]);
        else if (flags[i].startsWith('-D')) found.push(flags[i].slice(2));
    }
    return found;
}

function readProjectSettings(folder, { platforms = [], ofPath = '' } = {}) {
    const make = assignments(text(path.join(folder, 'config.make')));
    const result = { version: '', build: '', bundleId: '', defines: words(make.PROJECT_DEFINES), frameworks: [], libs: [] };
    for (const item of words(make.PROJECT_FRAMEWORKS)) {
        if (/^-l|^lib|\.(?:tbd|dylib)$/.test(item)) result.libs.push(cleanLibrary(item));
        else result.frameworks.push(cleanFramework(item));
    }
    result.libs.push(...words(make.PROJECT_SYSTEM_LIBS).map(cleanLibrary));
    const native = (!platforms.length || platforms.some((p) => applePlatforms.has(p))) ? propertyList(projectFile(folder)) : null;
    const entries = configurations(native);
    if (entries.length) {
        const state = propertyList(path.join(folder, '.ofprojectgenerator/apple-settings.json'))?.platforms || {};
        const platform = platforms.find((p) => applePlatforms.has(p)) || Object.keys(state)[0] || 'osx';
        const effective = new Map(entries.map((entry) => [entry.id, effectiveSettings(folder, native, entry, null, platform)]));
        const sdk = platform === 'ios' ? 'iphoneos' : platform === 'tvos' ? 'appletvos' : 'macosx';
        const score = (entry) => {
            const values = effective.get(entry.id);
            return scalar(values.SDKROOT) === sdk ? 3 : scalar(values.SUPPORTED_PLATFORMS).includes(sdk) ? 2 : 0;
        };
        entries.sort((a, b) => score(b) - score(a));
        let baseline = state[platform] || Object.values(state)[0] || {};
        const templateName = platform === 'tvos' ? 'tvOS' : platform;
        const template = ofPath ? propertyList(path.join(ofPath, 'scripts/templates', templateName, 'emptyExample.xcodeproj/project.pbxproj')) : null;
        if (!baseline.files && template) {
            const defaults = configurations(template);
            baseline = { files: { 'Project.xcconfig': text(path.join(ofPath, 'scripts/templates', templateName, 'Project.xcconfig')).replaceAll('../../..', ofPath) }, configurations: {} };
            for (const entry of entries) {
                const match = defaults.find((candidate) => candidate.id === entry.id)
                    || defaults.find((candidate) => candidate.configuration.name === entry.configuration.name);
                if (match) baseline.configurations[entry.id] = { settings: match.configuration.buildSettings || {} };
            }
        }
        const templateSDK = { frameworks: [], libs: [] };
        for (const entry of configurations(template)) {
            const linked = linkedSDK(template, entry.target);
            templateSDK.frameworks.push(...linked.frameworks); templateSDK.libs.push(...linked.libs);
        }
        for (const [index, entry] of entries.entries()) {
            const settings = effective.get(entry.id);
            const defaults = effectiveSettings(folder, native, entry, baseline, platform);
            if (index === 0) {
                const infoFile = path.resolve(folder, expand(settings.INFOPLIST_FILE || (fs.existsSync(path.join(folder, 'ofxiOS-Info.plist')) ? 'ofxiOS-Info.plist' : 'openFrameworks-Info.plist'), settings));
                result.version = expand(plistString(infoFile, 'CFBundleShortVersionString') || settings.MARKETING_VERSION, settings);
                result.build = expand(plistString(infoFile, 'CFBundleVersion') || settings.CURRENT_PROJECT_VERSION, settings);
                result.bundleId = expand(plistString(infoFile, 'CFBundleIdentifier') || settings.PRODUCT_BUNDLE_IDENTIFIER, settings);
            }
            if (entry.targetId !== entries[0].targetId) continue;
            const generated = defineWords(defaults.GCC_PREPROCESSOR_DEFINITIONS, defaults);
            for (const define of defineWords(settings.GCC_PREPROCESSOR_DEFINITIONS, settings)) if (!generated.includes(define)) result.defines.push(define);
            for (const key of ['OTHER_CFLAGS', 'OTHER_CPLUSPLUSFLAGS']) {
                const generatedDefines = cflagDefines(defaults[key], defaults);
                for (const define of cflagDefines(settings[key], settings)) if (!generatedDefines.includes(define)) result.defines.push(define);
            }
            const linked = linkedSDK(native, entry.target);
            const recorded = baseline.systemReferences?.[entry.targetId]?.references || {};
            const isSDK = (type, name) => linked[type].includes(name) || result[type].includes(name) || installedSDKItem(type, name);
            const generatedSDK = (type, name) => templateSDK[type].includes(name) || !!recorded[`${type}:${name}`];
            for (const type of ['frameworks', 'libs']) {
                for (const name of linked[type]) if (!templateSDK[type].includes(name) && !recorded[`${type}:${name}`]) result[type].push(name);
            }
            const flags = words(expand(settings.OTHER_LDFLAGS, settings));
            const defaultFlags = words(expand(defaults.OTHER_LDFLAGS, defaults));
            for (let i = 0; i < flags.length; i++) {
                const flag = flags[i];
                if (flag === '-framework' || flag === '-weak_framework') {
                    const name = flags[++i];
                    if (name && !defaultFlags.includes(name) && !generatedSDK('frameworks', name) && isSDK('frameworks', name)) result.frameworks.push(cleanFramework(name));
                } else if (flag === '-l' && flags[i + 1]) {
                    const name = cleanLibrary(flags[++i]);
                    if (!defaultFlags.includes(name) && !generatedSDK('libs', name) && isSDK('libs', name)) result.libs.push(name);
                } else if (/^-l.+/.test(flag) && !defaultFlags.includes(flag)) {
                    const name = cleanLibrary(flag);
                    if (!generatedSDK('libs', name) && isSDK('libs', name)) result.libs.push(name);
                }
                else if (/^(?:\/|\$\(?SDKROOT).*\/lib[^/]+\.(?:tbd|dylib)$/.test(flag) && !defaultFlags.includes(flag)) result.libs.push(cleanLibrary(flag));
            }
        }
    }
    const xcconfig = assignments(text(path.join(folder, 'Project.xcconfig')));
    const appConfig = assignments(text(path.join(folder, 'App.xcconfig')), { ...xcconfig });
    const cmake = text(path.join(folder, 'CMakeLists.txt'));
    const gradle = text(path.join(folder, 'ofApp/build.gradle'));
    const last = (contents, regex) => [...contents.matchAll(regex)].at(-1)?.[1] || '';
    result.version ||= unquote(make.APP_VERSION || last(cmake, /^\s*set\(\s*APP_VERSION\s+"?([0-9.]+)/gm) || appConfig.MARKETING_VERSION || last(gradle, /^\s*versionName\s+['"]([^'"]+)/gm));
    result.build ||= unquote(make.APP_BUILD || last(cmake, /^\s*set\(\s*APP_BUILD\s+"?([0-9]+)/gm) || appConfig.CURRENT_PROJECT_VERSION || last(gradle, /^\s*versionCode\s+([0-9]+)/gm));
    result.bundleId ||= unquote(make.APP_BUNDLE_ID || last(cmake, /^\s*set\(\s*APP_BUNDLE_ID\s+"?([^\s")]+)/gm) || appConfig.PRODUCT_BUNDLE_IDENTIFIER || last(gradle, /^\s*applicationId\s+['"]([^'"]+)/gm));
    for (const key of ['version', 'build', 'bundleId']) if (result[key].includes('$')) result[key] = '';
    result.defines = unique(result.defines.filter((define) => !define.includes('$')));
    result.frameworks = unique(result.frameworks); result.libs = unique(result.libs);
    return result;
}

module.exports = { readProjectSettings };
