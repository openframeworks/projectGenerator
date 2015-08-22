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
	enableConsole( defaultSettings['showConsole'] );
	$("#projectPath").val( defaultSettings['lastUsedProjectPath'] );
});

ipc.on('setProjectPath', function (arg) {
	var elem = document.getElementById("projectPath");
	elem.value = arg;
	defaultSettings['lastUsedProjectPath'] = arg;
	saveDefaultSettings();
});

ipc.on('setGenerateMode', function (arg) {
	switchGenerateMode(arg);
});

ipc.on('importProjectSettings', function(settings){
	$("#projectPath").val(settings['projectPath']);
	$("#projectName").val(settings['projectName']).trigger('change'); // change triggers addon scanning
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
	
	// syncronises gui with form element
	$("#addonsSelect").select2();
	
	if (neededAddons.length > 0) {
		$("#generate-mode-section").addClass("has-missing-addons");
		$("#missingAddonsList").text(neededAddons.join(", "));
	}
	else {
		$("#generate-mode-section").removeClass("has-missing-addons");
	}
});

// allow main to send UI messages
ipc.on('sendUIMessage', function (arg) {
	displayModal(arg);
});

ipc.on('consoleMessage', function( msg ){
	consoleMessage( msg );
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
	saveDefaultSettings();

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
		var myClass = 'platform';
		if( platforms[i] === defaultSettings['defaultPlatform'] ){ myClass += " platform-selected" }
		$(".platformSelect").append("<div class='"+myClass+"'>" + platforms[i] + "</span>");
	}


	$( "div.platform" ).click(function() {
		if( $(this).hasClass("platform-selected") && $(this).parent().children(".platform-selected").length <= 1 ){
			// always 1 has to remain selected
			$( this ).addClass( "platform-selected" );
		}
		else {
			$( this ).toggleClass( "platform-selected" );
		}
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

	$("#projectName").on('change', function () {
		if( $(this).is(":focus")===true ){ console.log($(this).is(":focus")); return; }

		var project = {};
		project['projectName'] = $("#projectName").val();
		project['projectPath'] = $("#projectPath").val();

		// check if project exists
		ipc.send('isOFProjectFolder', project);
	}).trigger('change');
	$("#projectName").on('focusout', function () {
		$(this).trigger('change');
	});

	$("#advanced-toggle").on("change", function () {
		enableAdvancedMode( $(this).is(':checked') );
	});

	$("#consoleToggle").on("change", function () {
		enableConsole( $(this).is(':checked') );
	});

	$("#uiModal").modal({'show':false});

	$("#defaultPlatform").html( defaultSettings['defaultPlatform'] );

	// Enable tooltips
	$("[data-toggle='tooltip']").tooltip();
}

function saveDefaultSettings() {
	var fs = require('fs');
	fs.writeFile(path.resolve(__dirname, 'settings.json'), JSON.stringify(defaultSettings), function (err) {
		if (err) {
			console.log("Unable to save defaultSettings to settings.json... (Error=" + err.code + ")");
		}
		else {
			console.log("Updated default settings for the PG. (written to settings.json)");
		}
	});
}

function generate() {

	// let's get all the info:

	var gen = {};

	gen['projectName'] = $("#projectName").val();
	gen['projectPath'] = $("#projectPath").val();
	
	gen['platformList'] = getPlatformList("#singlePlatformSelect");
	gen['addonList'] = $("#addonsSelect").val();
	gen['ofPath'] = $("#ofPath").val();

	console.log(gen);

	if( gen['platformList'] === null ){
		displayModal("Please select a platform first.");
	}
	else {
		ipc.send('generate', gen);
	}

}


function update() {
	// let's get all the info:

	var up = {};

	up['updatePath'] = $("#updatePath").val();
	
	//TODO: 
	up['platformList'] = getPlatformList("#singlePlatformSelect");
	up['updateRecursive'] = $("#platformRecursive").is(":checked");
	up['ofPath'] = $("#ofPath").val();

	if( up['platformList'] === null ){
		displayModal("Please select a platform first.");
	}
	else if( $("#generate-mode-section").hasClass("has-missing-addons") ){
		displayModal(
			'<div class="alert alert-info">'+
				'<div class="alert alert-danger">'+ $("#missingAddons").html() +'</div>'+
				'<p>You can probably download them on <a href="#ofxaddons.com" class="visitOfxAddons">ofxaddons.com</a>.<br>Add them to your addons folder and rescan it.<br>If you don\'t need any of these addons, you can remove them from the adons.make file within your project (proceed with precaution).</p>'+
			'</div><button class="btn btn-primary" type="button" onclick="rescanAddons()">Rescan addons folder</button>'
		);
	}
	else {
		ipc.send('update', up);
	}
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
		if ($("#generate-mode-section").hasClass('updateMode')) {
			$("#generate-mode-section").removeClass("has-missing-addons");
			clearAddonSelection();
		}

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

function enableConsole( showConsole ){
	if( showConsole ) {
		$("body").addClass('showConsole');
	}
	else {
		$("body").removeClass('showConsole');
	}
	defaultSettings['showConsole'] = showConsole;
	saveDefaultSettings();
	$("#consoleToggle").prop('checked', defaultSettings['showConsole'] );
}

function getPlatformList( platformSelector ){
	var selected = [];

	$(platformSelector).children(".platform.platform-selected").each(function(){
		selected.push( $(this).text() );
	});

	if(selected.length == 0){ return null; }
	else { return selected; }
}

function displayModal( message ){
	$("#uiModal .modal-body").html(message).find('.visitOfxAddons').click(function () {
		var shell = require('shell');
		shell.openExternal('http://www.ofxaddons.com/');
	});
	$("#uiModal").modal('show');
}

function consoleMessage( message ){
	message = (message + '').replace(/([^>\r\n]?)(\r\n|\n\r|\r|\n)/g, '$1'+ "<br>\n" +'$2'); // nl2br
	$("#console").append( $("<p>").html(message) );
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

function rescanAddons() {
	ipc.send('refreshAddonList', '');
}
