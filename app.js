"use strict";
// instead of ipc, maybe?
// https://github.com/atom/electron/blob/master/docs/api/remote.md

var ipc = require('ipc');
var path = require('path');


var platforms = {
    "osx": "OS X (Xcode)",
    "vs": "Windows (Visual Studio 2015)",
    "ios": "iOS (Xcode)",
    "linux": "Linux 32-bit (Code::Blocks)",
    "linux64": "Linux 64-bit (Code::Blocks)",
    "linuxarmv6l": "Linux ARMv6 (Makefiles)",
    "linuxarmv7l": "Linux ARMv7 (Makefiles)"
};

var defaultSettings;
var addonsInstalled;


//-----------------------------------------------------------------------------------
// IPC 
//-----------------------------------------------------------------------------------

//-------------------------------------------
ipc.on('setOfPath', function(arg) {
    setOFPath(arg);
});

//-------------------------------------------
ipc.on('setup', function(arg) {
    setup();
});

//-----------------------------------------
// this is called from main when defaults are loaded in:
ipc.on('setDefaults', function(arg) {

    defaultSettings = arg;
    setOFPath(defaultSettings['defaultOfPath']);
    enableAdvancedMode(defaultSettings['advancedMode']);


});

//-------------------------------------------
ipc.on('setStartingProject', function(arg) {
    $("#projectPath").val(arg['path']);
    $("#projectName").val(arg['name']);
});

//-------------------------------------------
ipc.on('setProjectPath', function(arg) {
    $("#projectPath").val(arg);
    //defaultSettings['lastUsedProjectPath'] = arg;
    //saveDefaultSettings();
    $("#projectName").trigger('change'); // checks if we need to be in update or generate mode
});

//-------------------------------------------
ipc.on('setGenerateMode', function(arg) {
    switchGenerateMode(arg);
});

//-------------------------------------------
ipc.on('importProjectSettings', function(settings) {
    $("#projectPath").val(settings['projectPath']);
    $("#projectName").val(settings['projectName']).trigger('change'); // change triggers addon scanning
});

//-------------------------------------------
ipc.on('setAddons', function(arg) {

    console.log("got set addons");

    addonsInstalled = arg;

    var select = document.getElementById("addonsList");
    select.innerHTML = "";

    if (arg !== null && arg.length > 0) {
        // add:
        for (var i = 0; i < arg.length; i++) {

            $('<div/>', {
                "class": 'item',
                "data-value": arg[i]
            }).html(arg[i]).appendTo(select);
        }
    
    } else {

        // if there's no addons, something is wrong ! 
        // let's tell them via an overlay
        // and bounce to settings (if it's wrong when you open the app, we should move you to seetings)

        // give the something is wrong message: 
        $('.ui.dimmer').dimmer('show')
  
        // bounce to settings
        $('.main .ui').tab('change tab', 'settings')
        
    }


    $('#addonsDropdown')
        .dropdown({
            allowAdditions: false
        });
});

//-------------------------------------------
// select the list of addons and notify if some aren't installed
ipc.on('selectAddons', function(arg) {

    var addonsAlreadyPicked = $("#addonsDropdown").val().split(',');

    console.log(addonsAlreadyPicked);
    console.log(arg);
    console.log(addonsInstalled);

    var neededAddons = [];

    //haystack.indexOf(needle) >= 0

    for (var i = 0; i < arg.length; i++) {
        // first, check if it's already picked, then do nothing
        if (addonsAlreadyPicked.indexOf(arg[i]) >= 0){
            console.log("already picked"); // alread picked
        } else {
            
            // if not picked, check if have it and try to pick it
            if (addonsInstalled.indexOf(arg[i]) >= 0){
                $('#addonsDropdown').dropdown('set selected', arg[i]);
            } else {
                neededAddons.push(arg[i]);
            }

        }
    }

    if (neededAddons.length > 0) {
        console.log("missing addons");
        // $("#generate-mode-section").addClass("has-missing-addons");
        $('#missingAddonList').empty();
        $('#missingAddonList').append("<b>" + neededAddons.join(", ") + "</b>");
        $("#missingAddonMessage").show();

    } else {

        $("#missingAddonMessage").hide();
        
        // $("#generate-mode-section").removeClass("has-missing-addons");
    }


// <div class="ui red message" id="missingAddonMessage" style="display: none">
//     <p>
//         <div class="header">
//             Missing addons
//         </div>
//     </p>
//     <p>you are attempting to update a project that is missing the following addons</p>
//     <p><div id="missingAddonList"></div></p>
//     <p>please download the missing addons and put them in your addons folder, then relaunch the project generator.</p>
//     <p>if you choose to update this project without these addons, you may overwrite the settings on the project.</p>
// </div>




});

//-------------------------------------------
// allow main to send UI messages
ipc.on('sendUIMessage', function(arg) {
    displayModal(arg);
});

//-------------------------------------------
ipc.on('consoleMessage', function(msg) {
    consoleMessage(msg);
});

//-------------------------------------------
ipc.on('generateCompleted', function(isSuccessful) {
    if (isSuccessful === true) {
        // We want to switch to update mode now
        $("#projectName").trigger('change');
    }
});

//-------------------------------------------
ipc.on('updateCompleted', function(isSuccessful) {
    if (isSuccessful === true) {
        // eventual callback after update completed
    }
});


//-----------------------------------------------------------------------------------
// functions
//-----------------------------------------------------------------------------------


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

    console.log("requesting addons");
    // trigger reload addons from the new OF path
    ipc.send('refreshAddonList', '');

}


//----------------------------------------
function setup() {



    $(document).ready(function() {



        // populate platform selection forms
        var select = document.getElementById("platformList");
        var option, i;
        for (var i in platforms) {
            var myClass = 'platform';

            $('<div/>', {
                "class": 'item',
                "data-value": i
            }).html(platforms[i]).appendTo(select);        }

        // start the platform drop down. 
        $('#platformsDropdown')
            .dropdown({
                allowAdditions: false
            });

        // set the platform to default
        $('#platformsDropdown').dropdown('set exactly', defaultSettings['defaultPlatform']);


        // // bind ofxAddons URL (load it in default browser; not within Electron)
        // $(".visitOfxAddons").click(function (e) {
        // 	e.preventDefault();
        // 	var shell = require('shell');
        // 	shell.openExternal('http://www.ofxaddons.com/');
        // });

        $("#projectPath").on('change', function () {
        	if( $(this).is(":focus")===true ){ return; }

            $("#projectName").trigger('change'); // checks the project on the new location
        });
        $("#projectPath").on('focusout', function () {
        	$(this).trigger('change');
        });

        $("#projectName").on('change', function () {
        	if( $(this).is(":focus")===true ){ return; }

        	var project = {};

            // fix "non alpha numeric characters here" as we did in the old PG
            var currentStr = $("#projectName").val()
            var stripped = currentStr.replace(/[^A-Za-z0-9]/g, '_');
            $("#projectName").val(stripped)

        	project['projectName'] = $("#projectName").val();
        	project['projectPath'] = $("#projectPath").val();

        	// check if project exists
        	ipc.send('isOFProjectFolder', project);
        }).trigger('change');

        $("#projectName").on('focusout', function () {
        	$(this).trigger('change');
        });

        $("#advancedOptions").checkbox();
        $("#advancedOptions").on("change", function() {
            if ($("#advancedOptions").filter(":checked").length > 0) {
                enableAdvancedMode(true);
            } else {
                enableAdvancedMode(false);
            }
        });


        if (defaultSettings['advancedMode'] === true){
        	// for some reason I can only get to the check box not via ID
        	// this may break if we have more then one checkbox...
        	// but for now it's ok.  I think getting this more specific 
        	// would be great: 
        	$('.checkbox').checkbox("set checked");
        }




        /* Stuff for the console setting (removed from UI)
	$("#consoleToggle").on("change", function () {
		enableConsole( $(this).is(':checked') );
	});*/

        // initialise the overall-use modal
        $("#uiModal").modal({
            'show': false
        });

        // show default platform in GUI
        $("#defaultPlatform").html(defaultSettings['defaultPlatform']);

        // Enable tooltips
        //$("[data-toggle='tooltip']").tooltip();

        // add current menu element in body tag for CSS styling
        // $('a[data-toggle="tab"]').on('shown.bs.tab', function (e) {
        //  		$('body').removeClass('page-create_update page-settings page-advanced').addClass( 'page-' + $(e.target).attr("href").replace('#', '') );
        // });



        $('.main.menu .item').tab({
            history: false
        });
    });


}

//----------------------------------------
function saveDefaultSettings() {

    var fs = require('fs');
    fs.writeFile(path.resolve(__dirname, 'settings.json'), JSON.stringify(defaultSettings, null, '\t'), function(err) {
        if (err) {
            console.log("Unable to save defaultSettings to settings.json... (Error=" + err.code + ")");
        } else {
            console.log("Updated default settings for the PG. (written to settings.json)");
        }
    });
}

//----------------------------------------
function generate() {

    // let's get all the info:

    var gen = {};

    gen['projectName'] = $("#projectName").val();
    gen['projectPath'] = $("#projectPath").val();

    gen['platformList'] = getPlatformList("#singlePlatformSelect");
    gen['addonList'] = $("#addonsDropdown").val();
    gen['ofPath'] = $("#ofPath").val();

    console.log(gen);

    if (gen['projectName'] === '') {
        displayModal("Please name your sketch first.");
    } else if (gen['projectPath'] === '') {
        displayModal("Your project path is empty...");
    } else if (gen['platformList'] === null) {
        displayModal("Please select a platform first.");
    } else {
        ipc.send('generate', gen);
    }

}


//----------------------------------------
function update() {
    // let's get all the info:

    var up = {};

    up['updatePath'] = $("#updatePath").val();

    //TODO: 
    up['platformList'] = getPlatformList("#singlePlatformSelect");
    up['updateRecursive'] = $("#platformRecursive").is(":checked");
    up['ofPath'] = $("#ofPath").val();

    if (up['platformList'] === null) {
        displayModal("Please select a platform first.");
    } else if ($("#generate-mode-section").hasClass("has-missing-addons")) {
        displayModal(
            '<div class="alert alert-info">' +
            '<div class="alert alert-danger">' + $("#missingAddons").html() + '</div>' +
            '<p>You can probably download them on <a href="#ofxaddons.com" class="visitOfxAddons">ofxaddons.com</a>.<br>Add them to your addons folder and rescan it.<br>If you don\'t need any of these addons, you can remove them from the adons.make file within your project (proceed with precaution).</p>' +
            '</div><button class="btn btn-primary" type="button" onclick="rescanAddons()">Rescan addons folder</button>'
        );
    } else {
        ipc.send('update', up);
    }
}

//----------------------------------------
function switchGenerateMode(mode) {
    // mode can be 'createMode' or 'updateMode'

    // switch to update mode
    if (mode == 'updateMode') {

        $("#generateButton").hide();
        $("#updateButton").show();
        $("#missingAddonMessage").hide();

        console.log('Switching GenerateMode to Update...');
        
        clearAddonSelection();
        
    }
    // [default]: switch to createMode (generate new projects)
    else {

        $("#generateButton").show();
        $("#updateButton").hide();
        $("#missingAddonMessage").hide();

        console.log('Switching GenerateMode to Create...');

        // if previously in update mode, deselect Addons
        clearAddonSelection();
        
    }
}

//----------------------------------------
function clearAddonSelection() {

    $('#addonsDropdown').dropdown('clear');

}

//----------------------------------------
function enableAdvancedMode(isAdvanced) {
    if (isAdvanced) {
        $('#platformsDropdown').removeClass("disabled");
        $("body").addClass('advanced');
        $('a.updateMultiMenuOption').show();

    } else {
        $('#platformsDropdown').addClass("disabled");
        $('#platformsDropdown').dropdown('set exactly', defaultSettings['defaultPlatform']);

        $("body").removeClass('advanced');
        $('a.updateMultiMenuOption').hide();

    }
    defaultSettings['advancedMode'] = isAdvanced;
    saveDefaultSettings();
    //$("#advancedToggle").prop('checked', defaultSettings['advancedMode'] );
}

/* Stuff for the console setting (removed from UI)
function enableConsole( showConsole ){
	if( showConsole ) {
		// this has to be in body for CSS reasons
		$("body").addClass('showConsole');
	}
	else {
		$("body").removeClass('showConsole');
	}
	defaultSettings['showConsole'] = showConsole;
	saveDefaultSettings();
	$("#consoleToggle").prop('checked', defaultSettings['showConsole'] );
}*/

//----------------------------------------
function getPlatformList(platformSelector) {
    var selected = [];

    $(platformSelector).children(".platform.platform-selected").each(function() {
        selected.push($(this).data('value'));
    });

    if (selected.length == 0) {
        return null;
    } else {
        return selected;
    }
}

//----------------------------------------
function displayModal(message) {
    $("#uiModal .modal-body").html(message).find('.visitOfxAddons').click(function() {
        var shell = require('shell');
        shell.openExternal('http://www.ofxaddons.com/');
    });
    $("#uiModal").modal('show');
}

//----------------------------------------
function consoleMessage(message) {
    message = (message + '').replace(/([^>\r\n]?)(\r\n|\n\r|\r|\n)/g, '$1' + "<br>\n" + '$2'); // nl2br
    $("#console").append($("<p>").html(message));
    $("#consoleContainer").scrollTop($('#console').offset().top); // scrolls to bottom
}

//-----------------------------------------------------------------------------------
// Button calls
//-----------------------------------------------------------------------------------

function browseOfPath() {
    ipc.send('pickOfPath', ''); // current path could go here (but the OS also remembers the last used folder)
}

function browseProjectPath() {
    ipc.send('pickProjectPath', ''); // current path could go here
}

function browseImportProject() {
    ipc.send('pickProjectImport', '');
}

function getUpdatePath() {
    ipc.send('pickUpdatePath', ''); // current path could go here
}

function rescanAddons() {
    ipc.send('refreshAddonList', '');
}