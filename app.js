
// instead of ipc, maybe?
// https://github.com/atom/electron/blob/master/docs/api/remote.md

var ipc = require('ipc');
var path = require('path');


var platforms = ["osx", "win_cb", "vs", "ios", "linux", "linux64", "linuxarmv6l", "linuxarmv7l"];

var defaultSettings;


ipc.on('setOfPath', function (arg) {
	setOFPath(arg);
});

ipc.on('setup', function (arg) {
	setup();
});



//-----------------------------------------
// this is called from main when defualts are loaded in:
ipc.on('setDefaults', function (arg) {
	defaultSettings = arg;
	setOFPath(defaultSettings['defaultOfPath']);
});

ipc.on('setProjectPath', function (arg) {
	var elem = document.getElementById("projectPath");
	elem.value = arg;
});

ipc.on('setUpdatePath', function (arg) {
	var elem = document.getElementById("updatePath");
	elem.value = arg;
});

ipc.on('setAddons', function (arg) {

	var select = document.getElementById("addonsSelect");
	select.innerHTML = "";

	if (arg.length > 0) {
		// add:
		for (i = 0; i < arg.length; i++) {
			var option = document.createElement("option");
			option.text = arg[i];
			select.add(option);
		}
		$("#addonsSelect").attr("data-placeholder", "addons...");
	} else {
		$("#addonsSelect").attr("data-placeholder", "no addons found, is OF path right?");
	}

	$("#addonsSelect").trigger("chosen:updated");
});


//----------------------------------------
function setOFPath(arg) {

	// get the element:
	var elem = document.getElementById("ofPath");

	if (!path.isAbsolute(arg)) {

		// if we are relative, don't do anything...

		elem.value = arg;

	} else {

		// else check settings for how we want this path.... make relative if we need to:

		if (defaultSettings['useRelativePath'] == true) {
			var relativePath = path.normalize(path.relative(path.resolve(__dirname), arg)) + "/";
			elem.value = relativePath;
		} else {
			elem.value = arg;
		}
	}

	ipc.send('refreshAddonList', '');


}


//----------------------------------------
function setup() {


	$('select').chosen();

	var select = document.getElementById("platformSelect");
	var selectUpdate = document.getElementById("platformsSelectUpdate");

	select.innerHTML = "";
	selectUpdate.innerHTML = "";

	for (i = 0; i < platforms.length; i++) {
		var option = document.createElement("option");
		option.text = platforms[i];
		select.add(option);
	}

	for (i = 0; i < platforms.length; i++) {
		var option = document.createElement("option");
		option.text = platforms[i];
		selectUpdate.add(option);
	}


	$("#platformSelect").trigger("chosen:updated");
	$("#platformsSelectUpdate").trigger("chosen:updated");

	$("#projectName").val('myApp');
}

function generate() {

	// let's get all the info:

	var generate = {}

	generate['projectName'] = $("#projectName").val();
	generate['projectPath'] = $("#projectPath").val();
	generate['platformList'] = $("#platformSelect").val();
	generate['addonList'] = $("#addonsSelect").val();
	generate['ofPath'] = $("#ofPath").val();

	//console.log(generate);

	ipc.send('generate', generate);

}


function update() {

	// let's get all the info:

	var update = {}




	update['updatePath'] = $("#updatePath").val();
	update['platformList'] = $("#platformsSelectUpdate").val();
	update['updateRecursive'] = $("#platformRecursive").is(":checked")
	update['ofPath'] = $("#ofPath").val();
	//console.log(update);

	// //console.log(generate);

	ipc.send('update', update);

}


//---------------------------------------- button calls this
function getPath() {
	ipc.send('pickOfPath', '');	// current path could go here
};

function getProjectPath() {
	ipc.send('pickProjectPath', '');	// current path could go here
};

function getUpdatePath() {
	ipc.send('pickUpdatePath', '');	// current path could go here
}
