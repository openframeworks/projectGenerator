var app = require('app');  // Module to control application life.
var BrowserWindow = require('browser-window');  // Module to create native browser window.
var dialog = require('dialog');
var ipc = require('ipc');
var fs = require('fs');
var path = require('path');
var async = require('async'); // https://github.com/caolan/async
var Menu = require('menu');

// Debugging: start the Electron PG from the terminal to see the messages from console.log()
// Example: /path/to/PG/Contents/MacOS/Electron /path/to/PG/Contents/Ressources/app
// Note: app.js's console.log is also visible from the WebKit inspector. (look for mainWindow.openDevTools() below )


//--------------------------------------------------------- load settings
var settings        = fs.readFileSync(path.resolve(__dirname, 'settings.json'));
var obj             = JSON.parse(settings, 'utf8');
var defaultOfPath   = obj["defaultOfPath"];
var addons;

if (!path.isAbsolute(defaultOfPath)){
   defaultOfPath = path.resolve(path.join(path.join(__dirname, "../../../../"), defaultOfPath));
   console.log("...... OF path changed to absolute: " + defaultOfPath);
   obj["defaultOfPath"] = defaultOfPath;
}


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
  mainWindow = new BrowserWindow({width: 500, height: 800});

  // load jquery here: 
  // http://stackoverflow.com/questions/30271011/electron-jquery-errors
 
  
  // and load the index.html of the app.
  mainWindow.loadUrl('file://' + __dirname + '/index.html');

  // Open the devtools.
  mainWindow.openDevTools();

  //when the window is loaded send the defaults
   mainWindow.webContents.on('did-finish-load', function() {
      //parseAddonsAndUpdateSelect();
      mainWindow.webContents.send('setDefaults', obj);
      mainWindow.webContents.send('setup','');
  });


  //http://electron.atom.io/docs/v0.29.0/api/dialog/
  //console.log();

  // Emitted when the window is closed.
  mainWindow.on('closed', function() {

    mainWindow = null;
    process.exit()
    // Dereference the window object, usually you would store windows
    // in an array if your app supports multi windows, this is the time
    // when you should delete the corresponding element.
  });

  var menuTmpl = [
    {
      label: 'Atom Shell',
      submenu: [
        {
          label: 'Quit',
          accelerator: 'Command+Q',
          click: function() { mainWindow.close(); }
        }
      ]
    },
    {
      label: 'View',
      submenu: [
        {
          label: 'Reload',
          accelerator: 'Command+R',
          click: function() { mainWindow.reload(); }
        },
        {
          label: 'Toggle DevTools',
          accelerator: 'Alt+Command+I',
          click: function() { mainWindow.toggleDevTools(); }
        }
      ]
    },
    {
      label: 'Edit',
      submenu: [
        {
          label: 'Undo',
          accelerator: 'Command+Z',
          selector: 'undo:'
        },
        {
          label: 'Redo',
          accelerator: 'Shift+Command+Z',
          selector: 'redo:'
        },
        {
          type: 'separator'
        },
        {
          label: 'Cut',
          accelerator: 'Command+X',
          selector: 'cut:'
        },
        {
          label: 'Copy',
          accelerator: 'Command+C',
          selector: 'copy:'
        },
        {
          label: 'Paste',
          accelerator: 'Command+V',
          selector: 'paste:'
        },
        {
          label: 'Select All',
          accelerator: 'Command+A',
          selector: 'selectAll:'
        },
      ]
    }
  ];
  menu = Menu.buildFromTemplate(menuTmpl);
  Menu.setApplicationMenu(menu);

});


function parseAddonsAndUpdateSelect(){


  //path = require('path').resolve(__dirname, defaultOfPath + "/addons");
  addons = getDirectories(defaultOfPath + "/addons");

  console.log("Reloading the addons folder, these were found:");
  console.log(addons);

  mainWindow.webContents.send('setAddons', addons)

  
}



function getDirectories(srcpath) {

  // because this is called at a different time, fs and path 
  // seemed to be "bad" for some reason...
  // that's why I am making temp ones here.
  // console.log(path);

  var fsTemp = require('fs');
  var pathTemp = require('path');
  var folder;

  try {

    return fsTemp.readdirSync(srcpath).filter(function(file) {
    
    //console.log(srcpath);
    //console.log(file);

    joinedPath = pathTemp.join(srcpath, file);
      if (joinedPath != null){
        // only accept folders (potential addons)
        return fsTemp.statSync(joinedPath).isDirectory();
      }
    });
  } catch (e) {
    if (e.code === 'ENOENT') {
      console.log("This doesn't seem to be a valid addons folder:\n"+srcpath);
      mainWindow.webContents.send('sendUIMessage', "No addons were found in "+srcpath+".\nIs the OF path correct?");
    } else {
      throw e;
    }
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
    if(!tmpFiles || tmpFiles.length <= 1) return false; // we need at least 2 files/folders within

    // todo: also check for config.make & addons.make ?
    var foundSrcFolder = false;
    var foundAddons = false;
    tmpFiles.forEach( function(el, i){
      if(el=='src') foundSrcFolder = true;
      if(el=='addons.make') foundAddons = true;
    });

    if(foundSrcFolder){
      event.sender.send('setGenerateMode', 'updateMode');

      // todo: pre-fill addons based on the 
      if(foundAddons){
        var projectAddons = fsTemp.readFileSync( pathTemp.resolve( folder, 'addons.make')).toString().split("\n");

        projectAddons = projectAddons.filter(function (el) {
          if (el == '' || el == 'addons') return false; // eleminates these items
          else return true;
        });

        //console.log(projectAddons);

        event.sender.send('selectAddons', projectAddons);
      }
    }
    else{
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
  parseAddonsAndUpdateSelect()
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


  if (update['updatePath'] != null){
    updatePath = "-p\"" + update['updatePath'] + "\""
  }

  if (update['platformList'] != null){
    platformString = "-x\"" + update['platformList'].join(", ") + "\""
  }
  
  if (update['ofPath'] != null){
    pathString = "-o\"" + update['ofPath'] + "\""
  }

  if (update['updateRecursive'] == true){
     recursiveString = "-r"
  }

  var pgApp = pathTemp.normalize(pathTemp.join(pathTemp.join(__dirname, "app"), "commandLinePG"));


  pgApp = pgApp.replace(/ /g, '\\ ');

  var wholeString = pgApp + " -u " + recursiveString + " " + pathString + " " + platformString + " " + updatePath;

  exec(wholeString, function callback(error, stdout, stderr){
      // result
      console.log(stdout);
  });

  console.log(  wholeString  );

  //console.log(__dirname);

});



ipc.on('generate', function(event, arg) {
  

 var generate = arg;

  /*

-r, --recursive                                 update recursively (applies 
                                                only to update)
-h, --help                                      
-c, --create                                    create a project file if it 
                                                doesn't exist
-u, --update                                    update a project file if it 
                                                does exist
-x"platform list", --platforms="platform list"  platform list
-a"addons list", --addons="addons list"         addons list
-o"OF path", --ofPath="OF path"                 openframeworks path
-p"project path", --projectPath="project path"  project path
-v, --verbose                                   run verbose
-d, --dryrun                                    don't change files

*/

  var exec = require('child_process').exec;

  
  var pathTemp = require('path');

  var projectString = ""
  var pathString = "";
  var addonString = "";
  var platformString = "";

  console.log(generate);

  if (generate['platformList'] != null){
    platformString = "-x\"" + generate['platformList'].join(", ") + "\""
  }
  
  if (generate['addonList'] != null){
    addonString = "-a\"" + generate['addonList'].join(", ") + "\""
  }

  if (generate['ofPath'] != null){
    pathString = "-o\"" + generate['ofPath'] + "\""
  }

  if (generate['projectName'] != null && 
      generate['projectPath'] != null){

      projectString = "-p\"" + pathTemp.join(generate['projectPath'], generate['projectName'] ) + "\""
  }

  var pgApp = pathTemp.normalize(pathTemp.join(pathTemp.join(__dirname, "app"), "commandLinePG"));

  pgApp = pgApp.replace(/ /g, '\\ ');

  var wholeString = pgApp + " -c " + pathString + " " + addonString + " " + platformString + " " + projectString;

  exec(wholeString, function callback(error, stdout, stderr){
      // result
      console.log(stdout);
  });

  console.log(  wholeString  );

  console.log(__dirname);


  //console.log(arg);
});

ipc.on('pickOfPath', function(event, arg) {

  path = dialog.showOpenDialog({
    title: 'select the root of OF, where you see libs, addons, etc',
    properties: ['openDirectory'],
    filters: []
  }, function(filenames) {
    if (filenames != null) {
      defaultOfPath = filenames[0];
      event.sender.send('setOfPath', filenames[0]);
    }
  });
});


ipc.on('pickUpdatePath', function(event, arg) {
  path = dialog.showOpenDialog({
    title: 'select the folder or root folder where you want to update',
    properties: ['openDirectory'],
    filters: []
  }, function(filenames) {
    if (filenames != null) {
      defaultOfPath = filenames[0];
      event.sender.send('setUpdatePath', filenames[0]);
    }
  });
});


ipc.on('pickProjectPath', function(event, arg) {
  path = dialog.showOpenDialog({
    title: 'select folder for project, typically apps/myApps',
    properties: ['openDirectory'],
    filters: []
  }, function(filenames) {
    if (filenames != null) {
      event.sender.send('setProjectPath', filenames[0]);
    }
  });
});
