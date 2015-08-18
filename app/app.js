
// instead of ipc, maybe?
// https://github.com/atom/electron/blob/master/docs/api/remote.md

var ipc = require('ipc');
var path = require('path');


var platforms = ["osx","win_cb", "vs", "ios", "linux", "linux64", "linuxarmv6l", "linuxarmv7l"];

var defaultSettings;


ipc.on('setOfPath', function(arg) {
	setOFPath(arg);
});

ipc.on('setup', function(arg) {
	setup();
});



//-----------------------------------------
// this is called from main when defualts are loaded in: 
ipc.on('setDefaults', function(arg) {
	defaultSettings = arg;
	setOFPath(defaultSettings['defaultOfPath']);
});

ipc.on('setProjectPath', function(arg) {
	var elem = document.getElementById("projectPath");
	elem.value = arg;	
});

ipc.on('setGenerateMode', function(arg){
	switchGenerateMode(arg);
});
/*ipc.on('setUpdatePath', function(arg) {
	var elem = document.getElementById("updatePath");
	elem.value = arg;	
});*/

ipc.on('setAddons', function(arg) {
	
	var select = document.getElementById("addonsSelect");
	select.innerHTML = "";

	if ( arg!=null && arg.length > 0){
		// add: 
		for (i = 0; i < arg.length; i++){
			var option = document.createElement("option");
			option.text = arg[i];
			select.add(option);
		}
		$("#addonsSelect").attr( "data-placeholder", "addons..." );
	} else {
		$("#addonsSelect").attr( "data-placeholder", "no addons found, is OF path right?" );
	}

	$("#addonsSelect").trigger("chosen:updated");
});

// select the list of addons and notify if some aren't installed
ipc.on('selectAddons', function(arg){
	var installedAddons = $("#addonsSelect option");
	var neededAddons = arg;

	$.each( installedAddons, function(i, ia){
		if( $.inArray(this.value, neededAddons) != -1 ){
			$(this).attr('selected', 'selected');
			var tmpVal = this.value;
			neededAddons = $.grep( neededAddons, function(val){
  				return val != tmpVal;
			});
		}
		else {
			$(this).removeAttr('selected');
		}
	});
	$("#addonsSelect").trigger("chosen:updated");

	if(neededAddons.length>0){
		$("#missingAddons").addClass("missing");
		$("#missingAddonsList").text( neededAddons.toString() );
	}
	else {
		$("#missingAddons").removeClass("missing");
	}
});

// allow main to send UI messages
ipc.on('sendUIMessage', function(arg) {
	alert(arg);
});



//----------------------------------------
function setOFPath(arg){
	// get the element: 
	var elem = document.getElementById("ofPath");

	if( ! path.isAbsolute(arg)) {

		// if we are relative, don't do anything...

    	elem.value = arg;					

    } else {

		// else check settings for how we want this path.... make relative if we need to: 
		if (defaultSettings['useRelativePath'] == true){
			var relativePath = path.normalize(path.relative(path.resolve(__dirname), arg)) + "/";
			elem.value = relativePath;
		} else {
			elem.value = arg;
		}
	}

	// update settings & remember the new OF path for next time
	defaultSettings['defaultOfPath'] = elem.value;
	var fs = require('fs');
	fs.writeFile( path.resolve(__dirname, 'settings.json'), JSON.stringify(defaultSettings), function (err) {
	  if (err){
	  	console.log( "Unable to save defaultOfPath to settings.json... (Error=" + err.code + ")");
	  	ipc.send('sendUIMessage', "OFPath changed but unable to write out the setting.");
	  }
	  else console.log("defaultOfPath="+elem.value+" (written to settings.json)");
	});

	// trigger reload addons from the new OF path
	ipc.send('refreshAddonList','');
	
}


//----------------------------------------
function setup(){


	$('select').chosen();

	var select = $("#platformSelect").get(0);
	//var selectUpdate = document.getElementById("platformsSelectUpdate");
	
	select.innerHTML = "";
	//selectUpdate.innerHTML = "";

	for (i = 0; i < platforms.length; i++){
		var option = document.createElement("option");
		option.text = platforms[i];
		select.add(option);
	}

	for (i = 0; i < platforms.length; i++){
		var option = document.createElement("option");
		option.text = platforms[i];
		//selectUpdate.add(option);
	}

	
	$("#platformSelect").trigger("chosen:updated");
	//$("#platformsSelectUpdate").trigger("chosen:updated");

	$( "#projectName" ).val('myApp');

	// bind ofxAddons URL (load it in default browser; not within Electron)
	$("#visitOfxAddons").click( function(){
		var shell = require('shell');
		shell.openExternal('http://www.ofxaddons.com/');
	} );

	// should this be on blur or on change ?
	$("#projectName").on('blur', function(){
		var project = {};
		project['projectName'] = $( "#projectName" ).val();
		project['projectPath'] = $( "#projectPath" ).val();

		// check if project exists
		ipc.send('isOFProjectFolder', project);
	});
}

function generate(){	

	// let's get all the info: 

	var generate = {}

	generate['projectName'] = $( "#projectName" ).val();
	generate['projectPath']  = $( "#projectPath" ).val();
	generate['platformList']  = $( "#platformSelect" ).val();
	generate['addonList']  =  $( "#addonsSelect" ).val();
	generate['ofPath']  = $( "#ofPath" ).val();

	//console.log(generate);
		
	ipc.send('generate', generate);

}


function update(){	

	// let's get all the info: 

	var update = {}

	update['updatePath'] = $( "#updatePath" ).val();
	update['platformList'] = $( "#platformSelect" ).val();
	update['updateRecursive'] = $( "#platformRecursive" ).is(":checked")
	update['ofPath']  = $( "#ofPath" ).val();
//console.log(update);
	
	// //console.log(generate);
		
	ipc.send('update', update);

}

function switchGenerateMode(mode){
	// mode can be 'createMode' or 'updateMode'

	// switch to update mode
	if (mode=='updateMode') {
		console.log('Switching GenerateMode to Update...');
		$("#generate-mode-section").removeClass('createMode').addClass('updateMode');
	}
	// [default]: switch to createMode (generate new projects)
	else {
		console.log('Switching GenerateMode to Create...');

		// if previously in update mode, deselect Addons
		if($("#generate-mode-section").hasClass('updateMode')) clearAddonSelection();

		$("#generate-mode-section").removeClass('updateMode').addClass('createMode');
	}
}

function clearAddonSelection(){
	var installedAddons = $("#addonsSelect option");
	$.each( installedAddons, function(i, ia){
		$(this).removeAttr('selected');
	});
	$("#addonsSelect").trigger("chosen:updated");
}


//---------------------------------------- button calls this
function getPath() {
	ipc.send('pickOfPath', '');	// current path could go here
};

function browseProjectPath() {
	ipc.send('pickProjectPath', '');	// current path could go here
};

function getUpdatePath(){
	ipc.send('pickUpdatePath', '');	// current path could go here
}
