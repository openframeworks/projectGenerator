"use strict";
// instead of ipc, maybe?
// https://github.com/atom/electron/blob/master/docs/api/remote.md

var ipc = require('ipc');
var path = require('path');
var fs = require('fs');


var platforms;

// var platforms = {
//     "osx": "OS X (Xcode)",
//     "vs": "Windows (Visual Studio 2015)",
//     "ios": "iOS (Xcode)",
//     "linux": "Linux 32-bit (Code::Blocks)",
//     "linux64": "Linux 64-bit (Code::Blocks)",
//     "linuxarmv6l": "Linux ARMv6 (Makefiles)",
//     "linuxarmv7l": "Linux ARMv7 (Makefiles)"
// };

var defaultSettings;
var addonsInstalled;
var currentPath;
var isOfPathGood = false;
var bVerbose = false;
var localAddons = [];



//-----------------------------------------------------------------------------------
// IPC 
//-----------------------------------------------------------------------------------

//-------------------------------------------
ipc.on('setOfPath', function(arg) {
    setOFPath(arg);
});

ipc.on('cwd', function(arg) {
    
    console.log(arg);
});

ipc.on('setUpdatePath', function(arg) {
    var elem = document.getElementById("updateMultiplePath");
    elem.value = arg;
    $("#updateMultiplePath").change();

});

ipc.on('isUpdateMultiplePathOk', function(arg) {
   if (arg == true){
        $("#updateMultipleWrongMessage").hide();
        $("#updateMultipleButton").removeClass("disabled");

   } else {
        $("#updateMultipleWrongMessage").show();
        $("#updateMultipleButton").addClass("disabled");
        
   }
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
    console.log(arg);
    
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
        
        $("#ofPathWrongMessage").hide();
        isOfPathGood = true;

       

    } else {

        $("#ofPathWrongMessage").show();
        isOfPathGood = false;
        $('#settingsMenuButton').click();

        // bounce to settings
        //$('.main .ui').tab('change tab', 'settings')
        


    }


  $('#addonsDropdown')
        .dropdown({
            allowAdditions: false,
            fullTextSearch: false,
            match: "text"
        });
});


ipc.on('setPlatforms', function(arg) {

    console.log("got set platforms");
    console.log(arg);
    console.log("got set platforms");
    
    platforms = arg;

    
    var select = document.getElementById("platformList");
    var option, i;
    for (var i in platforms) {
        var myClass = 'platform';

        $('<div/>', {
            "class": 'item',
            "data-value": i
        }).html(platforms[i]).appendTo(select);
    }

    // start the platform drop down. 
    $('#platformsDropdown').dropdown({
            allowAdditions: false
        });

    // set the platform to default
    $('#platformsDropdown').dropdown('set exactly', defaultSettings['defaultPlatform']);

    var select = document.getElementById("platformListMulti");
    var option, i;
    for (var i in platforms) {
        var myClass = 'platform';

        $('<div/>', {
            "class": 'item',
            "data-value": i
        }).html(platforms[i]).appendTo(select);        }

    // start the platform drop down. 
    $('#platformsDropdownMulti')
        .dropdown({
            allowAdditions: false
        });

    // // set the platform to default
    $('#platformsDropdownMulti').dropdown('set exactly', defaultSettings['defaultPlatform']);
});






//-------------------------------------------
// select the list of addons and notify if some aren't installed
ipc.on('selectAddons', function(arg) {


    // todo : DEAL WITH LOCAL ADDONS HERE....

    var addonsAlreadyPicked = $("#addonsDropdown").val().split(',');

    console.log(addonsAlreadyPicked);
    console.log(arg);
    console.log(addonsInstalled);

    var neededAddons = [];
    localAddons = [];

    //haystack.indexOf(needle) >= 0

    for (var i = 0; i < arg.length; i++) {
        arg[i] = arg[i].trim();
        // first, check if it's already picked, then do nothing
        if (addonsAlreadyPicked.indexOf(arg[i]) >= 0){
            console.log("already picked"); // alread picked
        } else {
            
            // if not picked, check if have it and try to pick it
            if (addonsInstalled.indexOf(arg[i]) >= 0){
                $('#addonsDropdown').dropdown('set selected', arg[i]);
            } else {

                var neededAddonPathRel = path.join($("#projectPath").val(), $("#projectName").val(), arg[i]);
                console.log(neededAddonPathRel);
                if (fs.existsSync(neededAddonPathRel) || 
                    fs.existsSync(neededAddons[i])){
                    localAddons.push(arg[i]);
                } else {
                    neededAddons.push(arg[i]);
                }

                
            }
        }
    }


    if (neededAddons.length > 0) {
        console.log("missing addons");
        // $("#generate-mode-section").addClass("has-missing-addons");
        $('#missingAddonList').empty();
        $('#missingAddonList').append("<b>" + neededAddons.join(", ") + "</b>");
        $("#missingAddonMessage").show();
        $("#adons-refresh-icon").show();

    } else {
        $("#adons-refresh-icon").hide();
        $("#missingAddonMessage").hide();

        // $("#generate-mode-section").removeClass("has-missing-addons");
    }

    if (localAddons.length > 0){
        // $("#generate-mode-section").addClass("has-missing-addons");
        $('#localAddonList').empty();
        $('#localAddonList').append("<b>" + localAddons.join(", ") + "</b>");
        $("#localAddonMessage").show();
        //$("#adons-refresh-icon").show();
    } else {
        $("#localAddonMessage").hide();
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

    // check if it has "success" message: 


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

ipc.on('setRandomisedSketchName', function(newName) {
    $("#projectName").val(newName);
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

    $("#ofPath").trigger('change');
}


//----------------------------------------
function setup() {


    jQuery.fn.extend({
      oneTimeTooltip: function(msg) {
        return this.each(function() {
            $(this).popup({
                content : msg,
                position : 'bottom center',
                on: 'manual',
                onVisible: function(e){
                    // hide on focus / change / onShow (for dropdowns)
                    $(e).one('focus change click', function(){ $(this).popup('hide');} );
                    console.log($(e).children('input'));
                }
            }).popup('show')
        });
      }
    });

    $(document).ready(function() {


        $('.main.menu .item').tab({
            history: false
        });

        $("#createMenuButon").tab({
            'onVisible':function(){
                if (isOfPathGood !== true){
                    $('#settingsMenuButton').click();
                     $('#ofPathError').modal({
                        onHide: function () {
                             $('#settingsMenuButton').click();
                        }
                    }).modal("show");
               }      
            }
        });

        $("#updateMenuButton").tab({
            'onVisible':function(){
                if (isOfPathGood !== true){
                    $('#settingsMenuButton').click();
                     $('#ofPathError').modal({
                        onHide: function () {
                             $('#settingsMenuButton').click();
                        }
                    }).modal("show");
               }  
            }
        });

        $("#settingsMenuButton").tab({
            'onVisible':function(){
                console.log("settings!! ");
                $('#createMenuButon').removeClass('active');
                $('#updateMenuButton').removeClass('active');
                $('#settingsMenuButton').addClass('active');
        }
        });
        // $('.main.menu .item').filter('.updateMultiMenuOption').tab({
        //     'onVisible':function(){
        //         alert("wh");
        //         // if (isOfPathGood !== true){
        //         //     $('.main .ui').tab('change tab', 'settings')
        //         // }
        //     }
        // });



        

        // bind external URLs (load it in default browser; not within Electron)
        $('*[data-toggle="external_target"]').click(function (e) {
            e.preventDefault();
            var shell = require('shell');
            shell.openExternal( $(this).prop('href') );
        });

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
            var stripped = currentStr.replace(/[&\/\\#,+()$~%.'":*?<>{}]/g,'_');
            $("#projectName").val(stripped)

        	project['projectName'] = $("#projectName").val();
        	project['projectPath'] = $("#projectPath").val();

        	// check if project exists
        	ipc.send('isOFProjectFolder', project);

            // update link to local project files
            $("#revealProjectFiles").prop('href', 'file:///' + path.join(project['projectPath'],project['projectName']).replace(/^\//, '') );
        }).trigger('change');

        $("#projectName").on('focusout', function () {
        	$(this).trigger('change');
        });

        $("#updateMultiplePath").on('change', function () {
            ipc.send('checkMultiUpdatePath', $("#updateMultiplePath").val());
        });

        $("#advancedOptions").checkbox();
        $("#advancedOptions").on("change", function() {
            if ($("#advancedOptions").filter(":checked").length > 0) {
                enableAdvancedMode(true);
            } else {
                enableAdvancedMode(false);
            }
        });

         $("#IDEButton").on("click", function() {  
            launchInIDE();
         });


         $("#verboseOption").checkbox();
         $("#verboseOption").on("change", function() {
            if ($("#verboseOption").filter(":checked").length > 0) {
                 defaultSettings['verboseOutput'] = true;
                 bVerbose = true;
                 saveDefaultSettings();
            } else {
                 defaultSettings['verboseOutput'] = false;
                 bVerbose = false;
                 saveDefaultSettings();
            }
        });


        $("#ofPath").on("change", function(){
            defaultSettings['defaultOfPath'] =  $("#ofPath").val();
            console.log("ofPath val " + $("#ofPath").val());
            saveDefaultSettings();

            console.log("requesting addons");
            // trigger reload addons from the new OF path
            ipc.send('refreshAddonList', $("#ofPath").val());
            ipc.send('refreshPlatformList', $("#ofPath").val());
        });


        if (defaultSettings['advancedMode'] === true){
        	$("#advancedOptions").attr('Checked','Checked'); 
        }

        if (defaultSettings['verboseOutput'] === true){
            $('#verboseOption').attr('Checked','Checked'); 
            bVerbose = true;
        }

        // updates ofPath when the field is manually changed
        $("#ofPath").on('blur', function(e){
            setOFPath($(this).val());
        }).on('keypress', function(e){
            if(e.which==13){
                e.preventDefault();
                $(this).blur();
            }
        });


        /* Stuff for the console setting (removed from UI)
	$("#consoleToggle").on("change", function () {
		enableConsole( $(this).is(':checked') );
	});*/
        // enable console? (hiddens setting)
        // if(defaultSettings['showConsole']){ $("body").addClass('enableConsole'); }
        // $("#showConsole").on('click', function(){ $('body').addClass('showConsole'); });
        // $("#hideConsole").on('click', function(){ $('body').removeClass('showConsole'); });

        // initialise the overall-use modal
        $("#uiModal, #fileDropModal").modal({
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

        
        // setup the multi update list as well. 



        $("#ofPath").change();

        // enable tool tips
        $('.tooltip').popup();
        
        // Open file drop zone
        $('body').on('dragenter', openDragInputModal);

        //Close file drop zone
        $(window).on('mouseout', closeDragInputModal );
        $(document).on('dragend', closeDragInputModal );

        // prevent dropping anywhere (dropping files loads them)
        // note: yes, dragover is also needed
        $(window).on('drop dragover', function(e){
            e.preventDefault();
            e.stopPropagation();
            return false;
        });

        
        // bind update thingy
        // note: dragover is needed because dragleave is called pretty randomly
        $("#dropZoneUpdate").on('dragenter dragover drop', onDragUpdateFile).on('dragleave', function(e){
            $(this).removeClass("accept deny");
        });

    });
}

function onDragUpdateFile( e ){
    e.preventDefault();
    e.stopPropagation();

    // handle file
    var files = e.originalEvent.dataTransfer.files;
    var types = e.originalEvent.dataTransfer.types;

    var rejected = true;
    // this first check filters out most files
    if(files.length == 1 && files[0].type==="" && types[0]=="Files"){

        // this folder check is more relayable
        var file = e.originalEvent.dataTransfer.items[0].webkitGetAsEntry();
        if( file.isDirectory ){
            rejected = false;
            $("#dropZoneUpdate").addClass("accept").removeClass("deny");

            // drop event ? --> import it!
            if( e.type=="drop" ){
                $("#projectName").val( files[0].name );
                var regExp = new RegExp("\\b/"+files[0].name+"\\b","gi");
                $("#projectPath").val( files[0].path.replace(regExp,"") ).trigger('change');

                $("createMenuButon").trigger('click');
                return true;
            }
        }
    }

    if(rejected) {
        $("#dropZoneUpdate").addClass("deny").removeClass("accept");

        if( e.type=="drop" ){
            displayModal(
                "The file you dropped is not compatible for importing.<br>"+
                "To import an OpenFrameworks project, drag & drop the whole project folder."
            );
        }
    }
    return false;
}

function closeDragInputModal(e){
    e.preventDefault();
    e.stopPropagation();

    if( $('body').hasClass('incommingFile') ){
        $('body').removeClass('incommingFile');
        $("#fileDropModal").modal('hide');
        $("#dropZoneUpdate").removeClass("accept deny");
    }
}

function openDragInputModal(e){
    e.preventDefault();
    e.stopPropagation();
    if(!$('body').hasClass('incommingFile')){
        $('body').addClass('incommingFile');
        $("#fileDropModal").modal('show');
    } 
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
    var platformsPicked = $("#platformsDropdown  .active");
    var addonsPicked = $("#addonsDropdown  .active");

    var platformValueArray = [];
    var addonValueArray = [];

    for (var i = 0; i < platformsPicked.length; i++){
        platformValueArray.push($(platformsPicked[i]).attr("data-value"));
    }

    for (var i = 0; i < addonsPicked.length; i++){
        addonValueArray.push($(addonsPicked[i]).attr("data-value"));
    }    

    // add any local addons
    for (var i = 0; i < localAddons.length; i++){
        addonValueArray.push(localAddons[i]);
    }
    
    var lengthOfPlatforms = platformValueArray.length;

    var gen = {};

    gen['projectName'] = $("#projectName").val();
    gen['projectPath'] = $("#projectPath").val();
    gen['platformList'] = platformValueArray;
    gen['addonList'] = addonValueArray; //$("#addonsDropdown").val();
    gen['ofPath'] = $("#ofPath").val();
    gen['verbose'] = bVerbose;

    // console.log(gen);
    if (gen['projectName'] === '') {
        $("#projectName").oneTimeTooltip("Please name your sketch first.");
    } else if (gen['projectPath'] === '') {
        $("#projectPath").oneTimeTooltip("Your project path is empty...");
    } else if (gen['platformList'] === null || gen['platformList'] === "" || lengthOfPlatforms == 0) {
        $("#platformsDropdown").oneTimeTooltip("Please select a platform first.");
    } else {
        ipc.send('generate', gen);
    }

}


//----------------------------------------
function updateRecursive() {

    // get the path and the platform list

    
    //platformsDropdownMulti

    
    
    var platformsPicked = $("#platformsDropdownMulti  .active");
    var platformValueArray = [];
    for (var i = 0; i < platformsPicked.length; i++){
        platformValueArray.push($(platformsPicked[i]).attr("data-value"));
    }



    var gen = {};
    gen['updatePath'] = $("#updateMultiplePath").val();
    gen['platformList'] = platformValueArray;
    gen['updateRecursive'] = true;
    gen['ofPath'] = $("#ofPath").val();
    gen['verbose'] = bVerbose;

    if (gen['updatePath'] === '') {
        displayModal("Please set update path");
    } else if (platformValueArray.length === 0) {
        displayModal("Please select a platform first.");
    } else {
        ipc.send('update', gen);
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
        $("#localAddonMessage").hide();
        $("#nameRandomiser").hide();
        $("#revealProjectFiles").show();
        $("#adons-refresh-icon").hide();

        console.log('Switching GenerateMode to Update...');
        
        clearAddonSelection();
        
    }
    // [default]: switch to createMode (generate new projects)
    else {
        // if previously in update mode, deselect Addons
        if( $("#updateButton").is(":visible") ){ clearAddonSelection(); }

        $("#generateButton").show();
        $("#updateButton").hide();
        $("#missingAddonMessage").hide();
        $("#localAddonMessage").hide();
        $("#nameRandomiser").show();
        $("#revealProjectFiles").hide();
        $("#adons-refresh-icon").hide();

        console.log('Switching GenerateMode to Create...');

        
        
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
    $("#uiModal .content").html(message).find('*[data-toggle="external_target"]').click(function (e) {
		e.preventDefault();
		var shell = require('shell');
		shell.openExternal( $(this).prop("href") );
    });

    if (message.indexOf("Success!") > -1){
        $("#IDEButton").show();
    } else {
        $("#IDEButton").hide();
    }

    $("#uiModal").modal('show');
}

//----------------------------------------
function consoleMessage(message) {
    message = (message + '').replace(/([^>\r\n]?)(\r\n|\n\r|\r|\n)/g, '$1' + "<br>\n" + '$2'); // nl2br
    $("#console").append($("<p>").html(message));
    $("#consoleContainer").scrollTop($('#console').offset().top); // scrolls console to bottom
}

//-----------------------------------------------------------------------------------
// Button calls
//-----------------------------------------------------------------------------------

function quit(){
    ipc.send('quit', '');
}
function browseOfPath() {
    ipc.send('pickOfPath', ''); // current path could go here (but the OS also remembers the last used folder)
}

function browseProjectPath() {

    var path = $("#projectPath").val();
    if (path === ''){
        path = $("#ofPath").val();
    }
    ipc.send('pickProjectPath', path); // current path could go here
}

function browseImportProject() {
    var path = $("#projectPath").val();
    if (path === ''){
        path = $("#ofPath").val();
    }
    ipc.send('pickProjectImport', path);
}

function getUpdatePath() {

    var path = $("#updateMultiplePath").val();
    if (path === ''){
        path = $("#ofPath").val();
    }

    ipc.send('pickUpdatePath', path); // current path could go here
}

function rescanAddons() {
    ipc.send('refreshAddonList', $("#ofPath").val());
    var projectInfo = {};
    projectInfo['projectName'] = $("#projectName").val();
    projectInfo['projectPath'] = $("#projectPath").val();
    ipc.send('isOFProjectFolder', projectInfo);     // <- this forces addon reload
}

function getRandomSketchName(){
    var path = $("#projectPath").val();
    if (path === ''){
        $("#projectPath").oneTimeTooltip('Please specify a path first...');
    }
    else {
        ipc.send('getRandomSketchName', path );
    }
}

function launchInIDE(){

    var project = {};
    project['projectName'] = $("#projectName").val();
    project['projectPath'] = $("#projectPath").val();
    project['platform'] = defaultSettings['defaultPlatform']; // ignores OS selection
    project['ofPath'] = $("#ofPath").val();

    ipc.send('launchProjectinIDE', project );
}

