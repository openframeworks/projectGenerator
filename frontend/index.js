"use strict";


const {app} = require('electron'); // Module to control application life.
const {BrowserWindow} = require('electron'); // Module to create native browser window.
const {dialog} = require('electron');
const {ipcMain} = require('electron');
const {Menu} = require('electron');

// const {crashReporter} = require('electron'); // Requires server destination

const fs = require('fs');
const path = require('path');
const moniker = require('moniker');
const exec = require('child_process').exec;
const os = require("os");

const debugDevTools = false;
let dialogIsOpen = false;

// Debugging: start the Electron PG from the terminal to see the messages from console.log()
// Example: /path/to/PG/Contents/MacOS/Electron /path/to/PG/Contents/Ressources/app
// Note: app.js's console.log is also visible from the WebKit inspector. (look for mainWindow.openDevTools() below )



const getStartingProjectName = () => {

    let ofRoot = obj["defaultOfPath"];
    if (!ofRoot || ofRoot === "") ofRoot = guessOFRoot()
    let defaultPathForProjects = path.join(ofRoot, obj["defaultRelativeProjectPath"]);
    let foundOne = false;
    let goodName = getGoodSketchName(defaultPathForProjects);
    startingProject['path'] = defaultPathForProjects;
    startingProject['name'] = goodName;
}

const parseAddonsAndUpdateSelect = (arg) => {
    console.log("in parseAddonsAndUpdateSelect " + arg);
    //path = require('path').resolve(__dirname, defaultOfPath + "/addons");
    addons = getDirectories(arg + "/addons","ofx");

    if (addons){
    if (addons.length > 0){
        addons = addons.filter( function(addon) {
            return addonsToSkip.indexOf(addon)==-1;
        });
    }
    }

    console.log("Reloading the addons folder, these were found:");
    console.log(addons);
    mainWindow.webContents.send('setAddons', addons);
}

const  parsePlatformsAndUpdateSelect = (arg) => {
    let folders = getDirectories(arg + "/scripts/templates");
    console.log("Reloading the templates folder, these were found:");
    console.log(folders);

    let platformsWeHave = {};
    let templatesWeHave = {};

    if (folders === undefined || folders === null) {
        //do something
    } else {
        // check all folder name under /scripts/templates
        for (let id in folders) {
            let key = folders[id];
            if (platforms[key]) {
                // this folder is for platform
                console.log("Found platform, key " + key + " has value " + platforms[key]);
                platformsWeHave[key] = platforms[key];
            }else{
                // this folder is for template
                if(templates[key]){
                    console.log("Found template folder, key " + key + " has value " + templates[key]);
                    templatesWeHave[key] = templates[key];
                }else{
                    // Unofficial folder name, maybe user's custom template? 
                    // We use folder name for both of key and value
                    console.log("Found unofficial folder, key " + key + " has value " + key);
                    templatesWeHave[key] = key;
                }
            }
        }
    }
    // saninty check...
    // for(let key in platformsWeHave){
    //  console.log("key " + key + " has value " + platformsWeHave[key]);
    // }
    mainWindow.webContents.send('setPlatforms', platformsWeHave);

   mainWindow.webContents.send('setTemplates', templatesWeHave);

}

const  getGoodSketchName = (arg) => {

    let currentProjectPath = arg;
    let foundOne = false;
    let goodName = "mySketch";

    if (bUseMoniker){

        let projectNames = new moniker.Dictionary();
        let tmpPath = require('path');
        projectNames.read(  tmpPath.join(__dirname, 'static', 'data', 'sketchAdjectives.txt'));
        goodName = "mySketch";

        while (foundOne === false) {
            if (fs.existsSync(tmpPath.join(currentProjectPath, goodName))) {
                console.log("«" + goodName + "» already exists, generating a new name...");
                let adjective = projectNames.choose();
                goodName = "my" + adjective.charAt(0).toUpperCase() + adjective.slice(1) + "Sketch";
            } else {
                foundOne = true;
            }
        }

    } else {

        let date = new Date();
        let formattedDate = formatDate(date);
        goodName = "sketch_" + formattedDate;
        let count = 1;

         while (foundOne === false) {
            if (fs.existsSync(path.join(currentProjectPath, goodName))) {
                console.log("«" + goodName + "» already exists, generating a new name...");
                goodName = "sketch_" + formattedDate + toLetters(count);
                count++;
            } else {
                foundOne = true;
            }
        }
    }

    return goodName;

}


const  getDirectories = (srcpath, acceptedPrefix) => {

    // because this is called at a different time, fs and path
    // seemed to be "bad" for some reason...
    // that's why I am making temp ones here.
    // console.log(path);

    let fsTemp = require('fs');
    let pathTemp = require('path');

    try {

        return fsTemp.readdirSync(srcpath).filter( (file)  => {

            //console.log(srcpath);
            //console.log(file);
            try{
                let joinedPath = pathTemp.join(srcpath, file);
                if ((acceptedPrefix==null || file.substring(0,acceptedPrefix.length)==acceptedPrefix) && joinedPath !== null) {
                    // only accept folders (potential addons)
                    return fsTemp.statSync(joinedPath).isDirectory();
                }
            }catch(e){}
        });
    } catch (e) {
        console.log(e);
        return null;
        // if (e.code === 'ENOENT') {
        //  console.log("This doesn't seem to be a valid addons folder:\n" + srcpath);
        //  mainWindow.webContents.send('sendUIMessage', "No addons were found in " + srcpath + ".\nIs the OF path correct?");
        // } else {
        //  throw e;
        // }
    }
}

let guessOFRoot = () => {
    console.log('[index.js] guessing of root...');
    const githubOFRoot = path.resolve(__dirname, '../../../');
    const releaseOFRoot = path.resolve(__dirname, '../../');

    console.log('Which root am I likely to be in?', path.basename(githubOFRoot), path.basename(releaseOFRoot) );

    let ofRoot = githubOFRoot;
    if (path.basename(releaseOFRoot).indexOf("of_v") !== -1) ofRoot = releaseOFRoot;
    return ofRoot;
};

let getPlatform = () => {

    console.log('[index.js] getting platform...');
    let myPlatform = "Unknown";
    if (/^win/.test(process.platform)) {
        myPlatform = 'vs';
    }
    // TODO: make the difference between osx and ios
    else if (process.platform === "darwin") {
        myPlatform = 'osx';
    } else if (process.platform === "linux") {
        myPlatform = 'linux';
        if (process.arch === 'ia32') {
            myPlatform = 'linux';
        } else if (process.arch === 'arm') {
            if (os.cpus()[0].model.indexOf('ARMv6') == 0) {
                myPlatform = 'linuxarmv6l';
            } else {
                myPlatform = 'linuxarmv7l';
            }
        } else if (process.arch === 'x64') {
            myPlatform = 'linux64';
        }
    }
    return myPlatform;
};


//--------------------------------------------------------- load settings

let obj;

// load settings.json

try {
    let settings = fs.readFileSync(path.resolve(__dirname, 'settings.json'));
    obj = JSON.parse(settings, 'utf8');
} catch (e) {
    console.error("could not load settings.json");
    obj = {
        defaultOfPath : guessOFRoot(),
        defaultPlatform : getPlatform(),
        advancedMode : false,
        showConsole : false,
        showDeveloperTools : false,
        defaultRelativeProjectPath : "apps/myApps",
        useDictionaryNameGenerator : false,
        useAddonDefines : false
    };
}

let hostplatform = "";
if (/^win/.test(process.platform)) {
    hostplatform = 'windows';
} else if (process.platform === "darwin") {
    hostplatform = 'osx';
} else if (process.platform === "linux") {
    hostplatform = 'linux';
}

console.log("detected platform: " + hostplatform + " in " + __dirname);

let defaultOfPath = obj["defaultOfPath"];
let addons;

// hide some addons, per https://github.com/openframeworks/projectGenerator/issues/62

let addonsToSkip = [
    "ofxiOS",
    "ofxMultiTouch",
    "ofxEmscripten",
    "ofxAccelerometer",
    "ofxAndroid"
]

let platforms = {
    "osx": "OS X (Xcode)",
    "vs": "Windows (Visual Studio 2017)",
    "ios": "iOS (Xcode)",
    "android": "Android (Android Studio)",
    "linux64": "Linux 64-bit (qtCreator)",
    "linuxarmv6l": "Linux ARMv6 (Makefiles)",
    "linuxarmv7l": "Linux ARMv7 (Makefiles)"
};
let bUseMoniker = obj["useDictionaryNameGenerator"];

let templates = {
    "emscripten": "Emscripten",
    "gitignore": "Git Ignore",
    "gles2": "Open GL ES 2",
    "gl3.1": "Open GL 3.1",
    "gl3.2": "Open GL 3.2",
    "gl3.3": "Open GL 3.3",
    "gl4.0": "Open GL 4.0",
    "gl4.1": "Open GL 4.1",
    "gl4.2": "Open GL 4.2",
    "gl4.3": "Open GL 4.3",
    "gl4.4": "Open GL 4.4",
    "gl4.5": "Open GL 4.5",
    "gles2": "Open GL ES 2",
    "linux": "Linux",  // !!??
    "msys2": "MSYS2/MinGW project template",
    "nofmod": "OSX application with no FMOD linking",
    "nowindow": "No window application",
    "tvOS": "Apple tvOS template",
    "unittest": "Unit test no window application",
    "vscode": "Visual Studio Code",
};



if (!path.isAbsolute(defaultOfPath)) {

    // todo: this needs to be PLATFORM specific b/c of where things are placed.
    // arturo, this may differ on linux, if putting ../ in settings doesn't work for the default path
    // take a look at this...

    if (hostplatform=="windows" || hostplatform=="linux"){
    	defaultOfPath = path.resolve(path.join(path.join(__dirname,"../../"), defaultOfPath));
    } else if(hostplatform=="osx"){
    	defaultOfPath = path.resolve(path.join(path.join(__dirname, "../../../../"), defaultOfPath));
    }

    obj["defaultOfPath"] = defaultOfPath;
}

// now, let's look for a folder called mySketch, and keep counting until we find one that doesn't exist
let startingProject = {};
startingProject['name'] = "";
startingProject['path'] = "";
getStartingProjectName();

//---------------------------------------------------------
// Report crashes to our server.
// crashReporter.start();

//---------------------------------------------------------
// Keep a global reference of the window object, if you don't, the window will
// be closed automatically when the JavaScript object is GCed.
let mainWindow = null;

// Quit when all windows are closed.
app.on('window-all-closed', () => {
    // On OS X it is common for applications and their menu bar
    // to stay active until the user quits explicitly with Cmd + Q
    if (process.platform != 'darwin') {
        app.quit();
    }
});



function formatDate(d){
    //get the month
    let month = d.getMonth();
    //get the day
    let day = d.getDate();
    //get the year
    let year = d.getFullYear();
    //pull the last two digits of the year
    year = year.toString().substr(2,2);
    //increment month by 1 since it is 0 indexed
    month = month + 1;
    //converts month to a string
    month = month + "";

    //if month is 1-9 pad right with a 0 for two digits
    if (month.length == 1){
        month = "0" + month;
    }
    //convert day to string
    day = day + "";
    //if day is between 1-9 pad right with a 0 for two digits
    if (day.length == 1){
        day = "0" + day;
    }
    //return the string "MMddyy"
    return month + day + year;
}

// wraps over to bb no aa, why?
function toLetters(num) {
    let mod = num % 26,
        pow = num / 26 | 0,
        out = mod ? String.fromCharCode(96 + (num % 26)) : (--pow, 'z');
    return pow ? toLetters(pow) + out : out;
}



//-------------------------------------------------------- window
// This method will be called when Electron has finished
// initialization and is ready to create browser windows.
app.on('ready', () => {
    // Create the browser window.

    mainWindow = new BrowserWindow({
        width: 500,
        height: 600,
        resizable: false,
        frame: false,
        webPreferences: {
            nodeIntegration: true
        }
    });

    // load jquery here:
    // http://stackoverflow.com/questions/30271011/electron-jquery-errors

    // and load the index.html of the app.
    mainWindow.loadURL('file://' + __dirname + '/index.html');

    // Open the devtools.
    if (obj["showDeveloperTools"] || debugDevTools) {
        mainWindow.openDevTools();
    }
    //when the window is loaded send the defaults
    mainWindow.webContents.on('did-finish-load', () => {
        //parseAddonsAndUpdateSelect();

        mainWindow.webContents.send('cwd', app.getAppPath());
        mainWindow.webContents.send('cwd', __dirname);
        mainWindow.webContents.send('cwd', process.resourcesPath);
        mainWindow.webContents.send('setStartingProject', startingProject);
        mainWindow.webContents.send('checkOfPathAfterSetup', '');
        mainWindow.webContents.send('setDefaults', obj);
        mainWindow.webContents.send('setup', '');
    });


    //http://electron.atom.io/docs/v0.29.0/api/dialog/
    //console.log();

    // Emitted when the window is closed.
    mainWindow.on('closed', () => {

        mainWindow = null;
        process.exit();
        // Dereference the window object, usually you would store windows
        // in an array if your app supports multi windows, this is the time
        // when you should delete the corresponding element.
    });

    let menuTmpl = [{
        label: 'Atom Shell',
        submenu: [{
            label: 'Quit',
            accelerator: 'Command+Q',
            click: () => {
                mainWindow.close();
            }
        }]
    }, {
        label: 'View',
        submenu: [{
            label: 'Reload',
            accelerator: 'Command+R',
            click: () => {
                mainWindow.reload();
            }
        }, {
            label: 'Toggle DevTools',
            accelerator: 'Alt+Command+I',
            click: () => {
                mainWindow.toggleDevTools();
            }
        }]
    }, {
        label: 'Edit',
        submenu: [{
            label: 'Undo',
            accelerator: 'Command+Z',
            selector: 'undo:'
        }, {
            label: 'Redo',
            accelerator: 'Shift+Command+Z',
            selector: 'redo:'
        }, {
            type: 'separator'
        }, {
            label: 'Cut',
            accelerator: 'Command+X',
            selector: 'cut:'
        }, {
            label: 'Copy',
            accelerator: 'Command+C',
            selector: 'copy:'
        }, {
            label: 'Paste',
            accelerator: 'Command+V',
            selector: 'paste:'
        }, {
            label: 'Select All',
            accelerator: 'Command+A',
            selector: 'selectAll:'
        }, ]
    }];
    let menuV = Menu.buildFromTemplate(menuTmpl);
    Menu.setApplicationMenu(menuV);

});

// function getDirs(srcpath, cb) {
//   fs.readdir(srcpath, function (err, files) {
//     if(err) {
//       console.error(err);
//       return cb([]);
//     }
//     let iterator = function (file, cb)  {
//       fs.stat(path.join(srcpath, file), function (err, stats) {
//         if(err) {
//           console.error(err);
//           return cb(false);
//         }
//         cb(stats.isDirectory());
//       })
//     }
//     async.filter(files, iterator, cb);
//   });
// }

ipcMain.on('isOFProjectFolder', (event, project) => {
    let fsTemp = require('fs');
    let pathTemp = require('path');
    let folder;
    folder = pathTemp.join(project['projectPath'], project['projectName']);

    try {

        let tmpFiles = fsTemp.readdirSync(folder);
        if (!tmpFiles || tmpFiles.length <= 1) {
            return false;
        } // we need at least 2 files/folders within

        // todo: also check for config.make & addons.make ?
        let foundSrcFolder = false;
        let foundAddons = false;
        tmpFiles.forEach( (el, i) => {
            if (el == 'src') {
                foundSrcFolder = true;
            }
            if (el == 'addons.make') {
                foundAddons = true;
            }
        });

        if (foundSrcFolder) {
            event.sender.send('setGenerateMode', 'updateMode');

            if (foundAddons) {
                let projectAddons = fsTemp.readFileSync(pathTemp.resolve(folder, 'addons.make')).toString().split("\n");

                projectAddons = projectAddons.filter( (el) => {
                    if (el === '' || el === 'addons') {
                        return false;
                    } // eleminates these items
                    else {
                        return true;
                    }
                });

                // remove comments
                projectAddons.forEach( (element, index) => {
                    projectAddons[index] = projectAddons[index].split('#')[0];
                }, projectAddons);

                // console.log('addons', projectAddons);

                event.sender.send('selectAddons', projectAddons);
            } else {
                event.sender.send('selectAddons', {});
            }
        } else {
            event.sender.send('setGenerateMode', 'createMode');
        }

        /*if (joinedPath != null){
		  // only accept folders (potential addons)
		  return fsTemp.statSync(joinedPath).isDirectory();
		}*/
    } catch (e) { // error reading dir
        event.sender.send('setGenerateMode', 'createMode');

        if (e.code === 'ENOENT') { // it's not a directory
            return false;
        } else {
            throw e;
        }
    }
});

// todo: default directories

//----------------------------------------------------------- ipc

ipcMain.on('refreshAddonList', function(event, arg) {
    console.log("in refresh " + arg)
    parseAddonsAndUpdateSelect(arg);
});

ipcMain.on('refreshPlatformList', function(event, arg) {
    parsePlatformsAndUpdateSelect(arg);
});


ipcMain.on('refreshTemplateList', function (event, arg) {
    console.log("refreshTemplateList");
    let selectedPlatforms = arg.selectedPlatforms;
    let ofPath = arg.ofPath;

    // Everytime user select/deselect new platforms,
    // we check each templates and disable if it is not supported by selected platforms
    // iterate all avairable templates and check template.config file

    let supportedPlatforms = [];

    for (let template in templates) {
        let configFilePath = ofPath + "/scripts/templates/" + template + "/template.config";
        if (fs.existsSync(configFilePath)) {
            const lineByLine = require('n-readlines');
            const liner = new lineByLine(configFilePath);
            let line;
            let bFindPLATOFORMS = false;

            // read line by line and try to find PLATFORMS setting
            while (line = liner.next()) {
                let line_st = line.toString();
                if (line_st.includes('PLATFORMS')) {
                    line_st = line_st.replace('PLATFORMS', '');
                    line_st = line_st.replace('=', '');
                    let platforms = line_st.trim().split(' ');
                    supportedPlatforms[template] = platforms;
                    bFindPLATOFORMS = true;
                    break;
                }
            }

            // PLATFORMS parameter does not exist
            if (!bFindPLATOFORMS) {
                supportedPlatforms[template] = 'enable';
            }
        } else {
            // config file does not exist
            supportedPlatforms[template] = 'enable';
        }
    }

    let invalidTemplateList = [];
    for (let template in supportedPlatforms) {
        let platforms = supportedPlatforms[template];
        let bValidTemplate = false;
        if (platforms === 'enable') {
            bValidTemplate = true;
        } else {
            bValidTemplate = selectedPlatforms.every(function (p) { return platforms.indexOf(p) > -1; });
            // Another option to enable template when "some" of the platforms are supported. (not every)
            // let bValidTemplate = platforms.some(function (p) { supportedPlatforms.indexOf(p) > -1 });
        }

        if (!bValidTemplate) {
            console.log("Selected platform [" + selectedPlatforms + "] does not support template " + template);
            invalidTemplateList.push(template);
        }
    }

    let returnArg = {
        invalidTemplateList: invalidTemplateList,
        bMulti: arg.bMulti
    };
    mainWindow.webContents.send('enableTemplate', returnArg);
});

ipcMain.on('getRandomSketchName', function(event, arg) {
    let goodName = getGoodSketchName(arg);
    event.sender.send('setRandomisedSketchName', goodName);
    event.sender.send('setGenerateMode', 'createMode'); // it's a new sketch name, we are in create mode
});

ipcMain.on('update', function(event, arg) {

    console.log('[index.js] updating...', arg);
    let update = arg;
    let exec = require('child_process').exec;
    let pathTemp = require('path');

    console.log(update);

    let updatePath = "";
    let pathString = "";
    let platformString = "";
    let templateString = "";
    let recursiveString = "";
    let verboseString = "";
    let rootPath = defaultOfPath;
    let definesString = "";

    if (update['addondefines'] !== null) {
        definesString = "-D";
    }

    if (update['updatePath'] !== null) {
        updatePath = update['updatePath'];
        updatePath = "\"" + updatePath + "\"";
    }

    if (update['platformList'] !== null) {
        platformString = "-p\"" + update['platformList'].join(",") + "\"";
    }

    if (update['templateList'] !== null) {
        templateString = "-t\"" + update['templateList'].join(",") + "\"";
    }

    if (update['ofPath'] !== null) {
        pathString = "-o\"" + update['ofPath'] + "\"";
        rootPath = update['ofPath'];
    }

    if (update['updateRecursive'] === true) {
        recursiveString = "-r";
    }

    if (update['verbose'] === true) {
        verboseString = "-v";
    }

    let pgApp = pathTemp.normalize(pathTemp.join(pathTemp.join(__dirname, "app"), "projectGenerator"));
    
    if( hostplatform == "linux" || hostplatform == "linux64" ){
        pgApp = pathTemp.join(rootPath, "apps/projectGenerator/commandLine/bin/projectGenerator");
    }

    if( arg.platform == 'osx' || arg.platform == 'linux' || arg.platform == 'linux64' ){
        pgApp = pgApp.replace(/ /g, '\\ ');
    } else {
        pgApp = "\"" + pgApp + "\"";
    }


    let wholeString = pgApp + " " + recursiveString + " " + verboseString + " " + pathString + " " + platformString + " " + templateString + " " + definesString + " " + updatePath;

    console.log('[index.js] sending to pg via update... ', wholeString);

    exec(wholeString, {maxBuffer : Infinity}, function callback(error, stdout, stderr) {

        if (error === null) {
            event.sender.send('consoleMessage', "<strong>" + wholeString + "</strong><br>" + stdout);
            event.sender.send('sendUIMessage',
                '<strong>Success!</strong><br>' +
                'Updating your project was successful! <a href="file:///' + update['updatePath'] + '" class="monospace" data-toggle="external_target">' + update['updatePath'] + '</a><br><br>' +
                '<button class="btn btn-default console-feature" onclick="$(\'#fullConsoleOutput\').toggle();">Show full log</button><br>' +
                '<div id="fullConsoleOutput"><br><textarea class="selectable">' + stdout + '\n\n\n(command used:' + wholeString + ')\n\n\n</textarea></div>'
            );

            //
            event.sender.send('updateCompleted', true);
        } else {
            event.sender.send('consoleMessage', "<strong>" + wholeString + "</strong><br>" + error.message);
            event.sender.send('sendUIMessage',
                '<strong>Error...</strong><br>' +
                'There was a problem updating your project... <span class="monospace">' + update['updatePath'] + '</span>' +
                '<div id="fullConsoleOutput" class="not-hidden"><br><textarea class="selectable">' + error.message + '\n\n\n(command used:' + wholeString + ')\n\n\n</textarea></div>'
            );
        }
    });

    //console.log(__dirname);

});

ipcMain.on('generate', function(event, arg) {

    console.log('[index.js] generating...', arg);

    let generate = arg;

    let exec = require('child_process').exec;


    let pathTemp = require('path');

    let projectString = "";
    let pathString = "";
    let addonString = "";
    let platformString = "";
    let templateString = "";
    let verboseString = "";
    let rootPath = defaultOfPath;
    let definesString = "";

    if (generate['addondefines'] !== null) {
        definesString = "-D";
    }


    if (generate['platformList'] !== null) {
        platformString = "-p\"" + generate['platformList'].join(",") + "\"";
    }

    if (generate['templateList'] !== null) {
        templateString = "-t\"" + generate['templateList'].join(",") + "\"";
    }

    if (generate['addonList'] !== null &&
        generate['addonList'].length > 0) {
        addonString = "-a\"" + generate['addonList'].join(",") + "\"";
    } else {
        addonString = "-a\" \"";
    }

    if (generate['ofPath'] !== null) {
        pathString = "-o\"" + generate['ofPath'] + "\"";
        rootPath = generate['ofPath'];
    }

    if (generate['verbose'] === true) {
        verboseString = "-v";
    }

    if (generate.projectName !== null &&
        generate.projectPath !== null) {
        projectString = "\"" + pathTemp.join(generate['projectPath'], generate['projectName']) + "\"";
    }

    let pgApp="";
    if(hostplatform == "linux" || hostplatform == "linux64"){
        pgApp = pathTemp.join(rootPath, "apps/projectGenerator/commandLine/bin/projectGenerator");
        //pgApp = "projectGenerator";
    }else{
        pgApp = pathTemp.normalize(pathTemp.join(pathTemp.join(__dirname, "app"), "projectGenerator"));
    }

    if( arg.platform == 'osx' || arg.platform == 'linux' || arg.platform == 'linux64' ){
        pgApp = pgApp.replace(/ /g, '\\ ');
    } else {
        pgApp = pgApp = "\"" + pgApp + "\"";
    }

    let wholeString = pgApp + " " + verboseString + " " + pathString + " " + addonString + " " + platformString + " " + templateString + " " + definesString + " " + projectString;

    console.log('[index.js] sending to pg via generate... ', wholeString);

    exec(wholeString, {maxBuffer : Infinity}, function callback(error, stdout, stderr) {

        let wasError = false;
        let text = stdout; //Big text with many line breaks
        let lines = text.split(os.EOL); //Will return an array of lines on every OS node works
        for (let i = 0; i < lines.length; i++) {
            if (lines[i].indexOf("Result:") > -1) {
                if (lines[i].indexOf("error") > -1) {
                    wasError = true;
                }
            }
        }

        // wasError = did the PG spit out an error (like a bad path, etc)
        // error = did node have an error running this command line app

        let fullPath = pathTemp.join(generate['projectPath'], generate['projectName']);
        if (error === null && wasError === false) {
            event.sender.send('consoleMessage', "<strong>" + wholeString + "</strong><br>" + stdout);
            event.sender.send('sendUIMessage',
                '<strong>Success!</strong><br>' +
                'Your can now find your project in <a href="file:///' + fullPath + '" data-toggle="external_target" class="monospace">' + fullPath + '</a><br><br>' +
                '<div id="fullConsoleOutput" class="not-hidden"><br><textarea class="selectable">' + stdout + '\n\n\n(command used: ' + wholeString + ')\n\n\n</textarea></div>'
            );
            event.sender.send('generateCompleted', true);
        } else if (error !== null) {
            event.sender.send('consoleMessage', "<strong>" + wholeString + "</strong><br>" + error.message);
            // note: stderr mostly seems to be also included in error.message
            // also available: error.code, error.killed, error.signal, error.cmd
            // info: error.code=127 means commandLinePG was not found
            event.sender.send('sendUIMessage',
                '<strong>Error...</strong><br>' +
                'There was a problem generating your project... <span class="monospace">' + fullPath + '</span>' +
                '<div id="fullConsoleOutput" class="not-hidden"><br><textarea class="selectable">' + error.message + '</textarea></div>'
            );
        } else if (wasError === true) {
            event.sender.send('consoleMessage', "<strong>" + wholeString + "</strong><br>" + stdout);
            event.sender.send('sendUIMessage',
                '<strong>Error!</strong><br>' +
                '<strong>Error...</strong><br>' +
                'There was a problem generating your project... <span class="monospace">' + fullPath + '</span>' +
                '<div id="fullConsoleOutput" class="not-hidden"><br><textarea class="selectable">' + stdout + '\n\n\n(command used: ' + wholeString + ')\n\n\n</textarea></div>'

            );

        }
    });

    console.log(wholeString);

    console.log(__dirname);


    //console.log(arg);
});

ipcMain.on('pickOfPath', function(event, arg) {
    console.log('pick OF Path');
    dialog.showOpenDialog({
        title: 'select the root of OF, where you see libs, addons, etc',
        properties: ['openDirectory'],
        filters: [],
        defaultPath: arg
    }).then( response => {

        const filename = response.filePaths[0];
        if (filename) {
            defaultOfPath = filename;
            event.sender.send('setOfPath', filename);
        }
    }).catch(err => {
        console.error('pickUpdatePath', err);
    });
});

ipcMain.on('pickUpdatePath', function(event, arg) {
    console.log('update OF Path');


    dialog.showOpenDialog({
        title: 'select root folder where you want to update',
        properties: ['openDirectory'],
        filters: [],
        defaultPath: arg
    }).then( response => {

        const filename = response.filePaths[0];
        if (filename) {
            event.sender.send('setUpdatePath', filename);
        }
    }).catch(err => {
        console.error('pickUpdatePath', err);
    });
});

ipcMain.on('pickProjectPath', function(event, arg) {
    console.log('pick Project Path');

    dialog.showOpenDialog({
        title: 'select parent folder for project, typically apps/myApps',
        properties: ['openDirectory'],
        filters: [],
        defaultPath: arg
    }).then( response => {

        const filename = response.filePaths[0];
        if (filename) {
            event.sender.send('setProjectPath', filename);
        }
    }).catch(err => {
        console.error('pickProjectPath', err);
    })
});

ipcMain.on('checkMultiUpdatePath', function(event, arg) {


    if (fs.existsSync(arg)) {
        event.sender.send('isUpdateMultiplePathOk', true);
    } else {
        event.sender.send('isUpdateMultiplePathOk', false);
    }

});

ipcMain.on('pickProjectImport', function(event, arg) {

    if(dialogIsOpen){
        return;
    }

    dialogIsOpen = true;
    let currentPath = dialog.showOpenDialog(mainWindow, {
        title: 'Select the folder of your project, typically apps/myApps/myGeniusApp',
        properties: ['openDirectory'],
        filters: [],
        defaultPath: arg
    }).then(  response => {
        const filename = response.filePaths[0];
        if (filename) {
            // gather project information
            let projectSettings = {};
            projectSettings['projectName'] = path.basename(filename);
            projectSettings['projectPath'] = path.dirname(filename);
            event.sender.send('importProjectSettings', projectSettings);
        }
        dialogIsOpen = false;
    }).catch( err => {
        console.error('pickProjectImport', err);
        dialogIsOpen = false;
    });
});


ipcMain.on('launchProjectinIDE', function(event, arg) {

    let pathTemp = require('path');
    let fsTemp = require('fs');
    let fullPath = pathTemp.join(arg['projectPath'], arg['projectName']);

    if( fsTemp.statSync(fullPath).isDirectory() == false ){
        // project doesn't exist
        event.sender.send('projectLaunchCompleted', false );
        return;
    }

    // // launch xcode
    if( arg.platform == 'osx' ){
        if(hostplatform == 'osx'){
            let osxPath = pathTemp.join(fullPath, arg['projectName'] + '.xcodeproj');
            console.log( osxPath );
            osxPath = "\"" + osxPath + "\"";

            exec('open ' + osxPath, function callback(error, stdout, stderr){
                return;
            });
        }
    } else if( arg.platform == 'linux' || arg.platform == 'linux64' ){
        if(hostplatform == 'linux'){
            let linuxPath = pathTemp.join(fullPath, arg['projectName'] + '.qbs');
            linuxPath = linuxPath.replace(/ /g, '\\ ');
            console.log( linuxPath );
            exec('xdg-open ' + linuxPath, function callback(error, stdout, stderr){
                return;
            });
        }
    } else if( arg.platform == 'android'){
        console.log("Launching ", fullPath)
        exec('studio ' + fullPath, function callback(error, stdout, stderr){
            if(error){
                event.sender.send('sendUIMessage',
                '<strong>Error!</strong><br>' +
                '<span>Could not launch Android Studio. Make sure the command-line launcher is installed by running <i>Tools -> Create Command-line Launcher...</i> inside Android Studio and try again</span>'
            );
            }
        });
    } else if( hostplatform == 'windows'){
        let windowsPath = pathTemp.join(fullPath, arg['projectName'] + '.sln');
        console.log( windowsPath );
        windowsPath = "\"" + windowsPath + "\"";
        exec('start ' + "\"\"" + " " + windowsPath, function callback(error, stdout, stderr){
            return;
        });
    }
});

ipcMain.on('quit', function(event, arg) {
    app.quit();
});
