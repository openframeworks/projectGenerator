"use strict";
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
// this is called from main when defaults are loaded in:
ipc.on('setDefaults', function (arg) {
	defaultSettings = arg;
	setOFPath(defaultSettings['defaultOfPath']);
	enableAdvancedMode( defaultSettings['advancedMode'] );
});

ipc.on('setProjectPath', function (arg) {
	var elem = document.getElementById("projectPath");
	elem.value = arg;
});

ipc.on('setGenerateMode', function (arg) {
	switchGenerateMode(arg);
});

ipc.on('importProjectSettings', function(settings){
	$("#projectPath").val(settings['projectPath']);
	$("#projectName").val(settings['projectName']).trigger('blur'); // blur triggers addon scanning
});


/*ipc.on('setUpdatePath', function(arg) {
	var elem = document.getElementById("updatePath");
	elem.value = arg;
});*/

ipc.on('setAddons', function (arg) {

	var select = document.getElementById("addonsSelect");
	select.innerHTML = "";



	if (arg !== null && arg.length > 0) {
		// add:
		for (var i = 0; i < arg.length; i++) {
			var option = document.createElement("option");
			option.text = arg[i];
			select.add(option);
		}
		$("#addonsSelect").attr("data-placeholder", "addons...");
	} else {
		$("#addonsSelect").attr("data-placeholder", "no addons found, is OF path right?");
	}

	// call select2 to make a good selectable 
	$("#addonsSelect").select2();
	//$("#addonsSelect").trigger("chosen:updated");
});

// select the list of addons and notify if some aren't installed
ipc.on('selectAddons', function (arg) {
	var installedAddons = $("#addonsSelect option");
	var neededAddons = arg;

	$.each(installedAddons, function (i, ia) {
		if ($.inArray(this.value, neededAddons) != -1) {
			$(this).attr('selected', 'selected');
			var tmpVal = this.value;
			neededAddons = $.grep(neededAddons, function (val) {
				return val != tmpVal;
			});
		}
		else {
			$(this).removeAttr('selected');
		}
	});
	

	$("#addonsSelect").select2();
	
	if (neededAddons.length > 0) {
		$("#missingAddons").addClass("missing");
		$("#missingAddonsList").text(neededAddons.toString());
	}
	else {
		$("#missingAddons").removeClass("missing");
	}
});

// allow main to send UI messages
ipc.on('sendUIMessage', function (arg) {
	alert(arg);
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
		if (defaultSettings['useRelativePath'] === true) {
			var relativePath = path.normalize(path.relative(path.resolve(__dirname), arg)) + "/";
			elem.value = relativePath;
		} else {
			elem.value = arg;
		}
	}

	// update settings & remember the new OF path for next time
	defaultSettings['defaultOfPath'] = elem.value;
	if( !saveDefaultSettings() ){
		ipc.send('sendUIMessage', "OFPath changed but unable to write out the setting.");
	}

	// trigger reload addons from the new OF path
	ipc.send('refreshAddonList', '');

}


//----------------------------------------
function setup() {

	// var select = $("#platformSelect").get(0);
	// //var selectUpdate = document.getElementById("platformsSelectUpdate");

	// select.innerHTML = "";
	// //selectUpdate.innerHTML = "";
	var option, i;
	for (i = 0; i < platforms.length; i++) {
		// option = document.createElement("span");
		// option.className = "platform";
		// option.text = platforms[i];
		// select.add(option);
		$(".platformSelect").append("<div class='platform'>" + platforms[i] + "</span>");
	}


	$( "div.platform" ).click(function() {
  		$( this ).toggleClass( "platform-selected" );
	});


	// for (i = 0; i < platforms.length; i++) {
	// 	option = document.createElement("option");
	// 	option.text = platforms[i];
	// 	//selectUpdate.add(option);
	// }

	$("#projectName").val('myApp');

	// bind ofxAddons URL (load it in default browser; not within Electron)
	$("#visitOfxAddons").click(function () {
		var shell = require('shell');
		shell.openExternal('http://www.ofxaddons.com/');
	});

	// should this be on blur or on change ?
	$("#projectName").on('blur', function () {
		var project = {};
		project['projectName'] = $("#projectName").val();
		project['projectPath'] = $("#projectPath").val();

		// check if project exists
		ipc.send('isOFProjectFolder', project);
	});

	$("#advanced-toggle").on("change", function () {
		enableAdvancedMode( $(this).is(':checked') );

	});
}

function saveDefaultSettings() {
	var fs = require('fs');
	fs.writeFile(path.resolve(__dirname, 'settings.json'), JSON.stringify(defaultSettings), function (err) {
		if (err) {
			console.log("Unable to save defaultSettings to settings.json... (Error=" + err.code + ")");
			return false;
		}
		else {
			console.log("Updated default settings for the PG. (written to settings.json)");
			return true;
		}
	});
}

function generate() {

	// let's get all the info:

	var gen = {};

	gen['projectName'] = $("#projectName").val();
	gen['projectPath'] = $("#projectPath").val();
	
	// TODO: 
	//gen['platformList'] = $("#platformSelect").val();
	gen['addonList'] = $("#addonsSelect").val();
	gen['ofPath'] = $("#ofPath").val();

	//console.log(gen);

	ipc.send('generate', gen);

}


function update() {
	// let's get all the info:

	var up = {};

	up['updatePath'] = $("#updatePath").val();
	
	//TODO: 
	//up['platformList'] = $("#platformSelect").val();
	up['updateRecursive'] = $("#platformRecursive").is(":checked");
	up['ofPath'] = $("#ofPath").val();
	//console.log(up);

	ipc.send('update', up);
}

function switchGenerateMode(mode) {
	// mode can be 'createMode' or 'updateMode'

	// switch to update mode
	if (mode == 'updateMode') {
		console.log('Switching GenerateMode to Update...');
		$("#generate-mode-section").removeClass('createMode').addClass('updateMode');
	}
		// [default]: switch to createMode (generate new projects)
	else {
		console.log('Switching GenerateMode to Create...');

		// if previously in update mode, deselect Addons
		if ($("#generate-mode-section").hasClass('updateMode')) {clearAddonSelection();}

		$("#generate-mode-section").removeClass('updateMode').addClass('createMode');
	}
}

function clearAddonSelection() {
	var installedAddons = $("#addonsSelect option");
	$.each(installedAddons, function (i, ia) {
		$(this).removeAttr('selected');
	});
	$("#addonsSelect").select2();
	
}

function enableAdvancedMode( isAdvanced ){
	if( isAdvanced ) {
		$("body").addClass('advanced');
	}
	else {
		$("body").removeClass('advanced');
	}
	defaultSettings['advancedMode'] = isAdvanced;
	saveDefaultSettings();
	$("#advanced-toggle").prop('checked', defaultSettings['advancedMode'] );
}


//---------------------------------------- button calls this
function getPath() {
	ipc.send('pickOfPath', '');	// current path could go here (but the OS also remembers the last used folder)
}

function browseProjectPath() {
	ipc.send('pickProjectPath', '');	// current path could go here
}

function browseImportProject() {
	ipc.send('pickProjectImport', '');
}

function getUpdatePath() {
	ipc.send('pickUpdatePath', '');	// current path could go here
}
