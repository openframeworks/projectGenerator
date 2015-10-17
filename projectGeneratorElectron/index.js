"use strict";

var app = require('app'); // Module to control application life.
var BrowserWindow = require('browser-window'); // Module to create native browser window.
var dialog = require('dialog');
var ipc = require('ipc');
var fs = require('fs');
var path = require('path');
var menu = require('menu');
var moniker = require('moniker');
var process = require('process');
var os = require("os");



// Debugging: start the Electron PG from the terminal to see the messages from console.log()
// Example: /path/to/PG/Contents/MacOS/Electron /path/to/PG/Contents/Ressources/app
// Note: app.js's console.log is also visible from the WebKit inspector. (look for mainWindow.openDevTools() below )



//--------------------------------------------------------- load settings
var obj;


try {
    var settings = fs.readFileSync(path.resolve(__dirname, 'settings.json'));
    obj = JSON.parse(settings, 'utf8');
    console.log(obj);
} catch (e) {

    // automatic platform detection
    var os = require("os");
    var myPlatform = "Unknown";
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

    obj = {
        "defaultOfPath": "",
        "advancedMode": false,
        "defaultPlatform": myPlatform,
        "verboseOutput" : false,
        "showDeveloperTools": false,
        "defaultRelativeProjectPath": "apps/myApps"
    };
}

var hostplatform = "";
if (/^win/.test(process.platform)) {
    hostplatform = 'windows';
} else if (process.platform === "darwin") {
    hostplatform = 'osx';
} else if (process.platform === "linux") {
    hostplatform = 'linux';
}

console.log("detected platform: " + hostplatform + " in " + __dirname);

var defaultOfPath = obj["defaultOfPath"];
var addons;
var platforms = {
    "osx": "OS X (Xcode)",
    "vs": "Windows (Visual Studio 2015)",
    "ios": "iOS (Xcode)",
    "linux": "Linux 32-bit (Code::Blocks)",
    "linux64": "Linux 64-bit (Code::Blocks)",
    "linuxarmv6l": "Linux ARMv6 (Makefiles)",
    "linuxarmv7l": "Linux ARMv7 (Makefiles)"
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
var startingProject = {};
startingProject['name'] = "";
startingProject['path'] = "";
getStartingProjectName();

//---------------------------------------------------------
// Report crashes to our server.
require('crash-reporter').start();

//---------------------------------------------------------
// Keep a global reference of the window object, if you don't, the window will
// be closed automatically when the JavaScript object is GCed.
var mainWindow = null;

// Quit when all windows are closed.
app.on('window-all-closed', function() {
    // On OS X it is common for applications and their menu bar
    // to stay active until the user quits explicitly with Cmd + Q
    if (process.platform != 'darwin') {
        app.quit();
    }
});

//-------------------------------------------------------- window
// This method will be called when Electron has finished
// initialization and is ready to create browser windows.
app.on('ready', function() {
    // Create the browser window.
    mainWindow = new BrowserWindow({
        width: 500,
        height: 600,
        resizable: false,
        frame: false
    });

    // load jquery here:
    // http://stackoverflow.com/questions/30271011/electron-jquery-errors

    // and load the index.html of the app.
    mainWindow.loadUrl('file://' + __dirname + '/index.html');

    // Open the devtools.
    if (obj["showDeveloperTools"]) {
        mainWindow.openDevTools();
    }
    //when the window is loaded send the defaults
    mainWindow.webContents.on('did-finish-load', function() {
        //parseAddonsAndUpdateSelect();

        mainWindow.webContents.send('cwd', app.getAppPath());
        mainWindow.webContents.send('cwd', __dirname);
        mainWindow.webContents.send('cwd', process.resourcesPath);
        mainWindow.webContents.send('setStartingProject', startingProject);
        mainWindow.webContents.send('setDefaults', obj);
        mainWindow.webContents.send('setup', '');
        mainWindow.webContents.send('checkOfPathAfterSetup', '');
    });


    //http://electron.atom.io/docs/v0.29.0/api/dialog/
    //console.log();

    // Emitted when the window is closed.
    mainWindow.on('closed', function() {

        mainWindow = null;
        process.exit();
        // Dereference the window object, usually you would store windows
        // in an array if your app supports multi windows, this is the time
        // when you should delete the corresponding element.
    });

    var menuTmpl = [{
        label: 'Atom Shell',
        submenu: [{
            label: 'Quit',
            accelerator: 'Command+Q',
            click: function() {
                mainWindow.close();
            }
        }]
    }, {
        label: 'View',
        submenu: [{
            label: 'Reload',
            accelerator: 'Command+R',
            click: function() {
                mainWindow.reload();
            }
        }, {
            label: 'Toggle DevTools',
            accelerator: 'Alt+Command+I',
            click: function() {
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
    var menuV = menu.buildFromTemplate(menuTmpl);
    menu.setApplicationMenu(menuV);

});

function getStartingProjectName() {
    var defaultPathForProjects = path.join(obj["defaultOfPath"], obj["defaultRelativeProjectPath"]);
    var foundOne = false;

    var projectNames = new moniker.Dictionary();
    projectNames.read(  path.join(__dirname, 'static', 'data', 'sketchAdjectives.txt'));
    var goodName = "mySketch";

    while (foundOne === false) {
        if (fs.existsSync(path.join(defaultPathForProjects, goodName))) {
            console.log("«" + goodName + "» already exists, generating a new name...");
            var adjective = projectNames.choose();
            goodName = "my" + adjective.charAt(0).toUpperCase() + adjective.slice(1) + "Sketch";
        } else {
            foundOne = true;
        }
    }

    startingProject['path'] = defaultPathForProjects;
    startingProject['name'] = goodName;
}

function parseAddonsAndUpdateSelect(arg) {
    console.log("in parseAddonsAndUpdateSelect " + arg);
    //path = require('path').resolve(__dirname, defaultOfPath + "/addons");
    addons = getDirectories(arg + "/addons","ofx");
    console.log("Reloading the addons folder, these were found:");
    console.log(addons);
    mainWindow.webContents.send('setAddons', addons);
}

function parsePlatformsAndUpdateSelect(arg) {
    var folders = getDirectories(arg + "/scripts/templates");
    console.log("Reloading the templates folder, these were found:");
    console.log(folders);

    var platformsWeHave = {};
    if (folders === undefined || folders === null) {
        //do something
    } else {
        for (var key in platforms) {
            if (folders.indexOf(key) > -1) {
                console.log("key " + key + " has value " + platforms[key]);
                platformsWeHave[key] = platforms[key];
            }
        }
    }
    // saninty check...
    // for(var key in platformsWeHave){
    // 	console.log("key " + key + " has value " + platformsWeHave[key]);
    // }
    mainWindow.webContents.send('setPlatforms', platformsWeHave);


}


function getDirectories(srcpath, acceptedPrefix) {

    // because this is called at a different time, fs and path
    // seemed to be "bad" for some reason...
    // that's why I am making temp ones here.
    // console.log(path);

    var fsTemp = require('fs');
    var pathTemp = require('path');

    try {

        return fsTemp.readdirSync(srcpath).filter(function(file) {

            //console.log(srcpath);
            //console.log(file);
            try{
                var joinedPath = pathTemp.join(srcpath, file);
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
        // 	console.log("This doesn't seem to be a valid addons folder:\n" + srcpath);
        // 	mainWindow.webContents.send('sendUIMessage', "No addons were found in " + srcpath + ".\nIs the OF path correct?");
        // } else {
        // 	throw e;
        // }
    }
}

// function getDirs(srcpath, cb) {
//   fs.readdir(srcpath, function (err, files) {
//     if(err) {
//       console.error(err);
//       return cb([]);
//     }
//     var iterator = function (file, cb)  {
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

ipc.on('isOFProjectFolder', function(event, project) {
    var fsTemp = require('fs');
    var pathTemp = require('path');
    var folder;
    folder = pathTemp.join(project['projectPath'], project['projectName']);

    try {

        var tmpFiles = fsTemp.readdirSync(folder);
        if (!tmpFiles || tmpFiles.length <= 1) {
            return false;
        } // we need at least 2 files/folders within

        // todo: also check for config.make & addons.make ?
        var foundSrcFolder = false;
        var foundAddons = false;
        tmpFiles.forEach(function(el, i) {
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
                var projectAddons = fsTemp.readFileSync(pathTemp.resolve(folder, 'addons.make')).toString().split("\n");

                projectAddons = projectAddons.filter(function(el) {
                    if (el === '' || el === 'addons') {
                        return false;
                    } // eleminates these items
                    else {
                        return true;
                    }
                });

                //console.log(projectAddons);

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

ipc.on('refreshAddonList', function(event, arg) {
    console.log("in refresh " + arg)
    parseAddonsAndUpdateSelect(arg);
});

ipc.on('refreshPlatformList', function(event, arg) {
    parsePlatformsAndUpdateSelect(arg);
});



ipc.on('getRandomSketchName', function(event, arg) {
    var currentProjectPath = arg;
    var foundOne = false;

    // Note: path.join throws an error when switched to (single) update and back to create mode...
    var projectNames = new moniker.Dictionary();
    var pathTemp = require('path');
    projectNames.read( pathTemp.join(__dirname, 'static', 'data', 'sketchAdjectives.txt') );
    var goodName = "";

    while (foundOne === false) {
        if (goodName === "" || fs.existsSync(pathTemp.join(currentProjectPath, goodName))) {
            console.log("«" + goodName + "» already exists, generating a new name...");
            var adjective = projectNames.choose();
            goodName = "my" + adjective.charAt(0).toUpperCase() + adjective.slice(1) + "Sketch";
        } else {
            foundOne = true;
        }
    }
    event.sender.send('setRandomisedSketchName', goodName);
});

ipc.on('update', function(event, arg) {

    var update = arg;
    var exec = require('child_process').exec;
    var pathTemp = require('path');

    console.log(update);

    var updatePath = "";
    var pathString = "";
    var platformString = "";
    var recursiveString = "";
    var verboseString = "";

    if (update['updatePath'] !== null) {
        updatePath = update['updatePath'];
        updatePath = "\"" + updatePath + "\"";
    }

    if (update['platformList'] !== null) {
        platformString = "-p\"" + update['platformList'].join(",") + "\"";
    }

    if (update['ofPath'] !== null) {
        pathString = "-o\"" + update['ofPath'] + "\"";
    }

    if (update['updateRecursive'] === true) {
        recursiveString = "-r";
    }

    if (update['verbose'] === true) {
        verboseString = "-v";
    }

    var pgApp = pathTemp.normalize(pathTemp.join(pathTemp.join(__dirname, "app"), "projectGenerator"));


    pgApp = pgApp.replace(/ /g, '\\ ');

    var wholeString = pgApp + " " + recursiveString + " " + verboseString + " " + pathString + " " + platformString + " " + updatePath;

    exec(wholeString, function callback(error, stdout, stderr) {

        if (error === null) {

            if( generate.verbose === true ){
                //event.sender.send('sendUIMessage', "<strong>" + wholeString + "</strong><br>" + stdout);
                event.sender.send('sendUIMessage',
                    '<strong>Success!</strong><br>' +
                    'Updating your project was successful! <a href="file:///' + update['updatePath'] + '" class="monospace" data-toggle="external_target">' + update['updatePath'] + '</a><br><br>' +
                    '<button class="btn btn-default console-feature" onclick="$(\'#fullConsoleOutput\').toggle();">Show full log</button><br>' +
                    '<div id="fullConsoleOutput"><br><textarea class="selectable">' + stdout + '</textarea></div>'
                );
            }
            event.sender.send('updateCompleted', true);
        } else {
            //event.sender.send('consoleMessage', "<strong>" + wholeString + "</strong><br>" + error.message);
            event.sender.send('sendUIMessage',
                '<strong>Error...</strong><br>' +
                'There was a problem updating your project... <span class="monospace">' + update['updatePath'] + '</span>' +
                '<div id="fullConsoleOutput" class="not-hidden"><br><textarea class="selectable">' + error.message + '</textarea></div>'
            );
            event.sender.send('updateCompleted', false);
        }
    });

    console.log(wholeString);

    //console.log(__dirname);

});

ipc.on('generate', function(event, arg) {


    var generate = arg;

    var exec = require('child_process').exec;


    var pathTemp = require('path');

    var projectString = "";
    var pathString = "";
    var addonString = "";
    var platformString = "";
    var verboseString = "";



    if (generate['platformList'] !== null) {
        platformString = "-p\"" + generate['platformList'].join(",") + "\"";
    } 

    if (generate['addonList'] !== null &&
        generate['addonList'].length > 0) {
        addonString = "-a\"" + generate['addonList'].join(",") + "\"";
    } else {
        addonString = "-a\" \"";
    }

    if (generate['ofPath'] !== null) {
        pathString = "-o\"" + generate['ofPath'] + "\"";
    }

    if (generate['verbose'] === true) {
        verboseString = "-v";
    }

    if (generate.projectName !== null &&
        generate.projectPath !== null) {
        projectString = "\"" + pathTemp.join(generate['projectPath'], generate['projectName']) + "\"";
    }

    var pgApp="";
    if(hostplatform == "linux"){	
        pgApp = "projectGenerator";
    }else{
        pgApp = pathTemp.normalize(pathTemp.join(pathTemp.join(__dirname, "app"), "projectGenerator"));
    }

    pgApp = pgApp.replace(/ /g, '\\ ');

    var wholeString = pgApp + " " + verboseString + " " + pathString + " " + addonString + " " + platformString + " " + projectString;

    exec(wholeString, function callback(error, stdout, stderr) {

        var wasError = false;
        var text = stdout; //Big text with many line breaks
        var lines = text.split(os.EOL); //Will return an array of lines on every OS node works
        for (var i = 0; i < lines.length; i++) {
            if (lines[i].indexOf("Result:") > -1) {
                if (lines[i].indexOf("error") > -1) {
                    wasError = true;
                }
            }
        }

        // wasError = did the PG spit out an error (like a bad path, etc) 
        // error = did node have an error running this command line app

        var fullPath = pathTemp.join(generate['projectPath'], generate['projectName']);
        if (error === null && wasError === false) {
            //event.sender.send('consoleMessage', "<strong>" + wholeString + "</strong><br>" + stdout);
            if( generate.verbose === true ){
                event.sender.send('sendUIMessage',
                    '<strong>Success!</strong><br>' +
                    'Your can now find your project in <a href="file:///' + fullPath + '" data-toggle="external_target" class="monospace">' + fullPath + '</a><br><br>' +
                    '<div id="fullConsoleOutput" class="not-hidden"><br><textarea class="selectable">' + stdout + '</textarea></div>'
                );
            }
            event.sender.send('generateCompleted', true);
        } else if (error !== null) {
            //event.sender.send('consoleMessage', "<strong>" + wholeString + "</strong><br>" + error.message);
            // note: stderr mostly seems to be also included in error.message
            // also available: error.code, error.killed, error.signal, error.cmd
            // info: error.code=127 means commandLinePG was not found
            event.sender.send('sendUIMessage',
                '<strong>Error...</strong><br>' +
                'There was a problem generating your project... <span class="monospace">' + fullPath + '</span>' +
                '<div id="fullConsoleOutput" class="not-hidden"><br><textarea class="selectable">' + error.message + '</textarea></div>'
            );
            event.sender.send('generateCompleted', false);
        } else if (wasError === true) {
            //event.sender.send('consoleMessage', "<strong>" + wholeString + "</strong><br>" + stdout);
            event.sender.send('sendUIMessage',
                '<strong>Error!</strong><br>' +
                '<strong>Error...</strong><br>' +
                'There was a problem generating your project... <span class="monospace">' + fullPath + '</span>' +
                '<div id="fullConsoleOutput" class="not-hidden"><br><textarea class="selectable">' + stdout + '</textarea></div>'

            );
            event.sender.send('generateCompleted', false);
        }
    });

    console.log(wholeString);

    console.log(__dirname);


    //console.log(arg);
});

ipc.on('pickOfPath', function(event, arg) {

    path = dialog.showOpenDialog({
        title: 'select the root of OF, where you see libs, addons, etc',
        properties: ['openDirectory'],
        filters: [],
        defaultPath: arg
    }, function(filenames) {
        if (filenames !== undefined && filenames.length > 0) {
            defaultOfPath = filenames[0];
            event.sender.send('setOfPath', filenames[0]);
        }
    });
});

ipc.on('pickUpdatePath', function(event, arg) {
    path = dialog.showOpenDialog({
        title: 'select root folder where you want to update',
        properties: ['openDirectory'],
        filters: [],
        defaultPath: arg
    }, function(filenames) {
        if (filenames !== undefined && filenames.length > 0) {
            defaultOfPath = filenames[0];
            event.sender.send('setUpdatePath', filenames[0]);
        }
    });
});

ipc.on('pickProjectPath', function(event, arg) {
    path = dialog.showOpenDialog({
        title: 'select parent folder for project, typically apps/myApps',
        properties: ['openDirectory'],
        filters: [],
        defaultPath: arg
    }, function(filenames) {
        if (filenames !== undefined && filenames.length > 0) {
            event.sender.send('setProjectPath', filenames[0]);
        }
    });
});

ipc.on('checkMultiUpdatePath', function(event, arg) {


    if (fs.existsSync(arg)) {
        event.sender.send('isUpdateMultiplePathOk', true);
    } else {
        event.sender.send('isUpdateMultiplePathOk', false);
    }

});

ipc.on('pickProjectImport', function(event, arg) {
    path = dialog.showOpenDialog({
        title: 'Select the folder of your project, typically apps/myApps/myGeniusApp',
        properties: ['openDirectory'],
        filters: [],
        defaultPath: arg
    }, function(filenames) {
        if (filenames != null) {
            // gather project information
            var tmpPath = require('path');
            var projectSettings = {};
            projectSettings['projectName'] = tmpPath.basename(filenames[0]);
            projectSettings['projectPath'] = tmpPath.dirname(filenames[0]);
            event.sender.send('importProjectSettings', projectSettings);
        }
    });
});

// arg holds: projectName, projectPath, platform, ofPath
ipc.on('launchProjectinIDE', function(event, arg) {
    if( arg.platform != obj.defaultPlatform ){
        event.sender.send('projectLaunchCompleted', false);
        return;
    }

    var pathTemp = require('path');
    var fsTemp = require('fs');
    var fullPath = pathTemp.join(arg['projectPath'], arg['projectName']);

    if( fsTemp.statSync(fullPath).isDirectory() == false ){
        // project doesn't exist
        event.sender.send('projectLaunchCompleted', false );
        return;
    }

    // launch xcode
    if( arg.platform == 'osx' ){
        var osxPath = pathTemp.join(fullPath, arg['projectName']+'.xcodeproj');
         console.log( osxPath );
        if( fsTemp.statSync(osxPath).isDirectory() == true ){ // note: .xcodeproj is a folder, not a file
            var exec = require('child_process').exec;
            exec('open '+osxPath, function callback(error, stdout, stderr){
                event.sender.send('projectLaunchCompleted', (error==null) );
                return;
            });
        }
        else {
            console.log('OSX project file not found!');
            event.sender.send('projectLaunchCompleted', false );
            return;
        }
    }

    // linux
    else if( arg.platform == 'linux' || arg.platform == 'linux64' ){
        // xdg-open $DESTINATION_PATH
    }

    // unknown platform
    else {
        console.log("Unsupported platform for Launch feature...");
        event.sender.send('projectLaunchCompleted', false );
                return;
    }
});

ipc.on('quit', function(event, arg) {
    app.quit();
});
