//nodeRequire is used instead of require due to clash with node and jquery
//see section - I can not use jQuery/RequireJS/Meteor/AngularJS in Electron : https://www.electronjs.org/docs/latest/faq/
const { ipcRenderer } = nodeRequire('electron');
path = nodeRequire('path');
fs = nodeRequire('fs');

let platforms;
let templates;

// var platforms = {
//     "osx": "OS X (Xcode)",
//     "vs": "Windows (Visual Studio)",
//     "ios": "iOS (Xcode)",
//     "linux": "Linux 32-bit (Code::Blocks)",
//     "linux64": "Linux 64-bit (Code::Blocks)",
//     "linuxarmv6l": "Linux ARMv6 (Makefiles)",
//     "linuxarmv7l": "Linux ARMv7 (Makefiles)"
// };

let defaultSettings;
let addonsInstalled;
let isOfPathGood = false;
let isFirstTimeSierra = false;
let bVerbose = false;
let localAddons = [];

let numAddedSrcPaths = 1;

//-----------------------------------------------------------------------------------
// IPC
//-----------------------------------------------------------------------------------

//-------------------------------------------
ipcRenderer.on('setOfPath', (event, arg) => {
    setOFPath(arg);
});

ipcRenderer.on('cwd', (event, arg) => {
    console.log(arg);
});

ipcRenderer.on('setUpdatePath', (event, arg) => {
    /** @type {HTMLInputElement} */
    const elem = document.getElementById("updateMultiplePath");
    elem.value = arg;
    $("#updateMultiplePath").change();
});

ipcRenderer.on('isUpdateMultiplePathOk', (event, arg) => {
    if (arg == true){
        $("#updateMultipleWrongMessage").hide();
        $("#updateMultipleButton").removeClass("disabled");
    } else {
        $("#updateMultipleWrongMessage").show();
        $("#updateMultipleButton").addClass("disabled");
    }
});

//-------------------------------------------
ipcRenderer.on('setup', (event, arg) => {
    setup();
});

//-----------------------------------------
// this is called from main when defaults are loaded in:
ipcRenderer.on('setDefaults', (event, arg) => {
    defaultSettings = arg;
    setOFPath(defaultSettings.defaultOfPath);
    enableAdvancedMode(defaultSettings.advancedMode);

});

//-------------------------------------------
ipcRenderer.on('setStartingProject', (event, arg) =>  {
    $("#projectPath").val(arg.path);
    $("#projectName").val(arg.name);
});

//-------------------------------------------
ipcRenderer.on('setProjectPath', (event, arg) => {
    $("#projectPath").val(arg);
    //defaultSettings['lastUsedProjectPath'] = arg;
    //saveDefaultSettings();
    $("#projectName").trigger('change'); // checks if we need to be in update or generate mode
});

//-------------------------------------------
ipcRenderer.on('setSourceExtraPath', (event, [arg, index]) => { // TODO:
    checkAddSourcePath(index);
    $("#sourceExtra-" + index).val(arg);
});

//-------------------------------------------
ipcRenderer.on('setGenerateMode', (event, arg) => {
    switchGenerateMode(arg);
});

//-------------------------------------------
ipcRenderer.on('importProjectSettings', (event, settings) => {
    $("#projectPath").val(settings.projectPath);
    $("#projectName").val(settings.projectName).trigger('change'); // change triggers addon scanning
});

//-------------------------------------------
ipcRenderer.on('setAddons', (event, arg) => {
    console.log("got set addons:", arg);

    addonsInstalled = arg;

    const select = document.getElementById("addonsList");
    select.innerHTML = "";

    if (addonsInstalled !== null && addonsInstalled.length > 0) {
        // add:
        for(let i = 0; i < addonsInstalled.length; i++) {
            $('<div/>', {
                "class": 'item',
                "data-value": addonsInstalled[i]
            }).html(addonsInstalled[i]).appendTo(select);
        }

        //$("#ofPathSierraMessage").hide();
        $("#ofPathWrongMessage").hide();
        isOfPathGood = true;
    } else {
        if(isFirstTimeSierra) {
            $("#ofPathSierraMessage").show();
        } else {
            $("#ofPathWrongMessage").show();
        }
        isOfPathGood = false;
        $('#settingsMenuButton').click();

        // bounce to settings
        //$('.main .ui').tab('change tab', 'settings')
    }


    $('#addonsDropdown')
        .dropdown({
            allowAdditions: false,
            fullTextSearch: 'exact',
            match: "text"
        });
});


ipcRenderer.on('setPlatforms', (event, arg) => {

    console.log("got set platforms");
    console.log(arg);

    platforms = arg;


    let select = $("#platformList");
    for (const i in platforms) {
        $('<div/>', {
            "class": 'item',
            "data-value": i
        }).html(platforms[i]).appendTo(select);
    }

    // start the platform drop down.
    $('#platformsDropdown')
        .dropdown({
            allowAdditions: false
        });

    // set the platform to default
    $('#platformsDropdown').dropdown('set exactly', defaultSettings.defaultPlatform);

    select = $("#platformListMulti");
    for (const i in platforms) {
        $('<div/>', {
            "class": 'item',
            "data-value": i
        }).html(platforms[i]).appendTo(select);
    }

    // start the platform drop down.
    $('#platformsDropdownMulti')
        .dropdown({
            allowAdditions: false
        });

    // // set the platform to default
    $('#platformsDropdownMulti').dropdown('set exactly', defaultSettings.defaultPlatform);
});


ipcRenderer.on('setTemplates', (event, arg) => {
    console.log("----------------");
    console.log("got set templates");
    console.log(arg);

    templates = arg;

    let select = $("#templateList");
    for (const i in templates) {
        console.log(i);
        $('<div/>', {
            "class": 'item',
            "data-value": i
        }).html(templates[i]).appendTo(select);
    }

    console.log(select);

    // start the template drop down.
    $('#templatesDropdown')
        .dropdown({
            allowAdditions: false,
            fullTextSearch: 'exact',
            match: "text",
            maxSelections: 1
        });

    // // set the template to default
    //$('#templatesDropdown').dropdown('set exactly', defaultSettings['defaultTemplate']);

    // Multi
    select = $("#templateListMulti");
    for (const i in templates) {
        $('<div/>', {
            "class": 'item',
            "data-value": i
        }).html(templates[i]).appendTo(select);        
    }

    // start the platform drop down.
    $('#templatesDropdownMulti')
        .dropdown({
            allowAdditions: false,
            maxSelections: 1
        });

    // // set the template to default
    //$('#templatesDropdownMulti').dropdown('set exactly', defaultSettings['defaultTemplate']);
});


ipcRenderer.on('enableTemplate', (event, arg) => {

    console.log('enableTemplate');
    const items = arg.bMulti === false
                ? $('#templatesDropdown .menu .item')
                : $('#templatesDropdownMulti .menu .item');

    // enable all first
    for (let i = 0; i < items.length; i++) {
        const item = $(items[i]);
        item.removeClass("disabled");
    }

    for (const template of arg.invalidTemplateList) {
        for (let i = 0; i < items.length; i++) {
            const item = $(items[i]);
            if (item.attr('data-value') === template) {
                item.addClass("disabled");
            }
        }
    }
});

//-------------------------------------------
// select the list of addons and notify if some aren't installed
ipcRenderer.on('selectAddons', (event, arg) => {
    // todo : DEAL WITH LOCAL ADDONS HERE....

    const addonsAlreadyPicked = $("#addonsDropdown").val().split(',');

    console.log(addonsAlreadyPicked);
    console.log(arg);
    console.log(addonsInstalled);

    const neededAddons = [];
    localAddons = [];

    //haystack.indexOf(needle) >= 0

    for (let i = 0; i < arg.length; i++) {
        arg[i] = arg[i].trim();
        // first, check if it's already picked, then do nothing
        if (addonsAlreadyPicked.indexOf(arg[i]) >= 0){
            console.log("already picked"); // alread picked
        } else {
            // if not picked, check if have it and try to pick it
            if (addonsInstalled.indexOf(arg[i]) >= 0){
                $('#addonsDropdown').dropdown('set selected', arg[i]);
            } else {
                const neededAddonPathRel = path.resolve($("#projectPath").val(), $("#projectName").val(), arg[i]);
                console.log(neededAddonPathRel);
                if (fs.existsSync(neededAddonPathRel)
                    || fs.existsSync(neededAddons[i]))
                {
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
ipcRenderer.on('sendUIMessage', (event, arg) => {
    // check if it has "success" message:
    displayModal(arg);
});

//-------------------------------------------
ipcRenderer.on('consoleMessage', (event, msg) => {
    consoleMessage(msg);
});

//-------------------------------------------
ipcRenderer.on('generateCompleted', (event, isSuccessful) => {
    if (isSuccessful === true) {
        // We want to switch to update mode now
        $("#projectName").trigger('change');
    }
});

//-------------------------------------------
ipcRenderer.on('updateCompleted', (event, isSuccessful) => {
    if (isSuccessful === true) {
        // eventual callback after update completed
    }
});

ipcRenderer.on('setRandomisedSketchName', (event, newName) => {
    $("#projectName").val(newName);
});


//-----------------------------------------------------------------------------------
// functions
//-----------------------------------------------------------------------------------


//----------------------------------------
/**
 * @param {string} ofPathValue 
 */
function setOFPath(ofPathValue) {
    // get the element:
    /** @type {HTMLInputElement} */
    const ofPathElem = document.getElementById("ofPath");

    if (ofPathValue != null && !path.isAbsolute(ofPathValue)) {
        // if we are relative, don't do anything...

        ofPathElem.value = ofPathValue;
    } else {
        // else check settings for how we want this path.... make relative if we need to:
        if (defaultSettings.useRelativePath === true) {
            const relativePath = path.normalize(path.relative(path.resolve(__dirname), ofPathValue)) + "/";
            ofPathElem.value = relativePath;
        } else {
            ofPathElem.value = ofPathValue;
        }
    }

    $("#ofPath").trigger('change');
}

//----------------------------------------
function setup() {
    // jQuery.fn.extend({
    //     oneTimeTooltip: function (msg) {
    //         return this.each(function () {
    //             $(this).popup({
    //                 content: msg,
    //                 position: 'bottom center',
    //                 on: 'manual',
    //                 onVisible: function (e) {
    //                     // hide on focus / change / onShow (for dropdowns)
    //                     $(e).one('focus change click', function () { $(this).popup('hide'); });
    //                     console.log($(e).children('input'));
    //                 }
    //             }).popup('show')
    //         });
    //     }
    // });


    $(document).ready(() => {
        try {
            const {
                release,
                platform
            } = ipcRenderer.sendSync('getOSInfo');
            const os_major_pos = release.indexOf(".");
            const os_major = release.slice(0, os_major_pos);
            const isSierra = (platform === 'darwin' || platform=== 'osx' );

            const ofpath = document.getElementById("ofPath").value;
            console.log("platform is " + platform + " isSierra is " + isSierra + " ofpath is " + ofpath  )
    
            if(isSierra) {
                try {
                    const runningOnVar = (ofpath.length >= 8 && ofpath.substring(0,8) === '/private');
                    isFirstTimeSierra = runningOnVar;
                } catch(e) {
                    isFirstTimeSierra = false;
                }
            }
        } catch(e) {
            isFirstTimeSierra = false;
        }
        
        console.log("App is translocated: " + isFirstTimeSierra);
 
        $('.main.menu .item').tab({
            history: false
        });

        $("#createMenuButon").tab({
            'onVisible':() => {
                if (isOfPathGood !== true){
                    $('#settingsMenuButton').click();
                     $('#ofPathError').modal({
                        onHide: () => {
                             $('#settingsMenuButton').click();
                        }
                    }).modal("show");
               }
            }
        });

        $("#updateMenuButton").tab({
            'onVisible':() => {
                if (isOfPathGood !== true) {
                    $('#settingsMenuButton').click();
                     $('#ofPathError').modal({
                        onHide: () => {
                             $('#settingsMenuButton').click();
                        }
                    }).modal("show");
               }
            }
        });

        $("#settingsMenuButton").tab({
            'onVisible': () => {
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
        $('*[data-toggle="external_target"]').click((e) => {
            e.preventDefault();
            ipcRenderer.send('openExternal', $(e.currentTarget).prop('href'));
        });

        $("#projectPath").on('change', () => {
        	if($("#projectPath").is(":focus") === true) {
                 return; 
            }

            $("#projectName").trigger('change'); // checks the project on the new location
        });
        $("#projectPath").on('focusout', () => {
        	$("#projectPath").trigger('change');
        });

        $("#projectName").on('change', () => {
        	if( $("#projectName").is(":focus") === true ){ return; }

            // fix "non alpha numeric characters here" as we did in the old PG
            const currentStr = $("#projectName").val();
            const stripped = currentStr.replace(/[&\/\\#,+()$~%.'":*?<>{}]/g, '_');
            $("#projectName").val(stripped)

        	const project = {
                projectName: $("#projectName").val(),
                projectPath: $("#projectPath").val()
            };

        	// check if project exists
        	ipcRenderer.send('isOFProjectFolder', project);

            // update link to local project files
            $("#revealProjectFiles").prop('href', 'file:///' + path.join(project.projectPath, project.projectName).replace(/^\//, '') );
        }).trigger('change');

        $("#projectName").on('focusout', () => {
        	$("#projectName").trigger('change');
        });

        $("#updateMultiplePath").on('change', () => {
            ipcRenderer.send('checkMultiUpdatePath', $("#updateMultiplePath").val());
        });

        $("#advancedOptions").checkbox();
        $("#advancedOptions").on("change", () => {
            if ($("#advancedOptions").filter(":checked").length > 0) {
                enableAdvancedMode(true);
            } else {
                enableAdvancedMode(false);
            }
        });

         $("#IDEButton").on("click", () => launchInIDE());


         $("#verboseOption").checkbox();
         $("#verboseOption").on("change", () => {
            if ($("#verboseOption").filter(":checked").length > 0) {
                 defaultSettings.verboseOutput = true;
                 bVerbose = true;
                 saveDefaultSettings();
            } else {
                 defaultSettings.verboseOutput = false;
                 bVerbose = false;
                 saveDefaultSettings();
            }
        });

        $("#ofPath").on("change", () => {
            const ofpath = $("#ofPath").val();
            defaultSettings.defaultOfPath = ofpath;
            console.log("ofPath val " + ofpath);
            if(isFirstTimeSierra) {
                //ipcRenderer.sendSync('firstTimeSierra', "xattr -r -d com.apple.quarantine " + ofpath + "/projectGenerator-osx/projectGenerator.app");
                //$("#projectPath").val(ofpath + "/apps/myApps").trigger('change');
                if( isFirstTimeSierra ){
                    $("#ofPathSierraMessage").show();
                    $('#settingsMenuButton').click();
                }
            }else{
                saveDefaultSettings();
                $("#projectPath").val(ofpath + "/apps/myApps").trigger('change');
                console.log("requesting addons");
                // trigger reload addons from the new OF path
                ipcRenderer.send('refreshAddonList', $("#ofPath").val());
                ipcRenderer.send('refreshPlatformList', $("#ofPath").val());
            }
        });


        if (defaultSettings.advancedMode === true){
        	$("#advancedOptions").attr('Checked','Checked');
        }

        if (defaultSettings.verboseOutput === true){
            $('#verboseOption').attr('Checked','Checked');
            bVerbose = true;
        }

        // updates ofPath when the field is manually changed
        $("#ofPath").on('blur', (e) => {
            const ofpath = $("#ofPath").val();
            setOFPath(ofpath);
            if(isFirstTimeSierra) {
                //$("#projectPath").val(ofpath + "/apps/myApps").trigger('change');
            }
        }).on('keypress', (e) => {
            if(e.which == 13){
                e.preventDefault();
                $("#ofPath").blur();
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
        $("#uiModal").modal({
            'show': false
        });

        $("#fileDropModal").modal({
            'show': false,
            onHide: () => {
                $('body').removeClass('incomingFile');
            },
            onShow: () => {
                $('body').addClass('incomingFile');
            }
        });


        // show default platform in GUI
        $("#defaultPlatform").html(defaultSettings.defaultPlatform);
        //$("#defaultTemplate").html(defaultSettings['defaultTemplate']);

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
        $(window).on('dragbetterenter', openDragInputModal);
        $(window).on('dragenter', openDragInputModal);

        // Close file drop zone
        $(window).on('dragbetterleave', closeDragInputModal );
        $(window).on('mouseleave', closeDragInputModal );

        // prevent dropping anywhere (dropping files loads their URL, unloading the PG)
        // note: weirdly, dragover is also needed
        $(window).on('drop dragover', blockDragEvent );
        //$(window).on('dragleave', blockDragEvent);

        $("#dropZoneOverlay").on('drop', onDropFile).on('dragend', closeDragInputModal);

        // this allows to close the drop zone if it ever stays open due to a bug.
        $("#dropZoneOverlay").on('click', closeDragInputModal);
        $(window).on('keypress', (e) => {
            if( e.which === 27 ){ // esc key
                e.stopPropagation();
                e.preventDefault();
                closeDragInputModal( e );
            }
        });

        // listen for drag events
        // note: dragover is needed because dragleave is called pretty randomly
        $("#dropZoneUpdate")
            .on('dragenter dragover drop', onDragUpdateFile)
            .on('dragleave', (e) => {
                $("#dropZoneUpdate").removeClass("accept deny");
            });


        // reflesh template dropdown list depends on selected platforms
        $("#platformsDropdown").on('change', () => {
            const selectedPlatforms = $("#platformsDropdown input").val();
            const selectedPlatformArray = selectedPlatforms.trim().split(',');
            let arg = {
                ofPath: $("#ofPath").val(),
                selectedPlatforms: selectedPlatformArray,
                bMulti: false
            }
            console.log(arg);
            ipcRenderer.send('refreshTemplateList', arg);
        })
        $("#platformsDropdownMulti").on('change', () => {
            const selectedPlatforms = $("#platformsDropdownMulti input").val();
            const selectedPlatformArray = selectedPlatforms.trim().split(',');
            let arg = {
                ofPath: $("#ofPath").val(),
                selectedPlatforms: selectedPlatformArray,
                bMulti: true
            }
            ipcRenderer.send('refreshTemplateList', arg);
        })

    });
}

function blockDragEvent(e){
    //console.log('blockDragEvent via '+e.type + ' on '+ e.target.nodeName + '#' + e.target.id);

    // open drop overlay if not already open
    if( !$('body').hasClass('incomingFile') ){
        $(window).triggerHandler('dragbetterenter');
    }

    e.stopPropagation();
    e.preventDefault();
    return false;
};

function acceptDraggedFiles( e ){
     // handle file
    const files = e.originalEvent.dataTransfer.files;
    const types = e.originalEvent.dataTransfer.types;

    // this first check filters out most files
    if(files && files.length == 1 && files[0].type == "" && types[0] == "Files"){
        // this folder check is more relayable
        const file = e.originalEvent.dataTransfer.items[0].webkitGetAsEntry();
        if(file.isDirectory) {
            return true;
        }
    }
    return false;
}

function onDragUpdateFile( e ){
    e.stopPropagation();
    e.preventDefault();
    //console.log('onDragUpdateFile via '+e.type + ' on '+ e.target.nodeName + '#' + e.target.id);

    if( !$('body').hasClass('incomingFile') ){
        return false;
    }

   if( acceptDraggedFiles( e ) ){
        $("#dropZone").addClass("accept").removeClass("deny");
        return true;
    }
    // files are rejected
    else {
        $("#dropZone").addClass("deny").removeClass("accept");
    }
    return false;
}

function onDropFile( e ){
    e.stopPropagation();
    e.preventDefault();

   if( acceptDraggedFiles( e ) ){
        $("#dropZone").addClass("accept").removeClass("deny");

        if( $('body').hasClass('advanced') && false ){ // todo: if (tab multiple is open)
            // do batch import

            $("updateMenuButton").triggerHandler('click');
        }
        else {
            const files = e.originalEvent.dataTransfer.files;
            // import single project folder
            $("#projectName").val( files[0].name );
            const projectFullPath = files[0].path;
            const projectParentPath = path.normalize(projectFullPath + '/..');
            $("#projectPath").val( projectParentPath ).triggerHandler('change');

            $("createMenuButon").triggerHandler('click');
        }
        closeDragInputModal(e);
        return true;
    }
    // files are rejected
    else {
        $("#dropZone").addClass("deny").removeClass("accept");

        displayModal(
            `The file you dropped is not compatible for importing.<br>
            To import an OpenFrameworks project, drag & drop the whole project folder.`
        );
    }
    return false;
}

function closeDragInputModal(e){
    e.stopPropagation();
    e.preventDefault();

    //console.log('closeDragInputModal via '+e.type + ' on '+ e.target.nodeName + '#' + e.target.id);

    // Prevent closing the modal while still fading in
    // if( $("#fileDropModal").filter('.ui.modal:not(.fade.in)').length===0 ){
    //     return;
    // }

    $("#fileDropModal").modal('hide');
    $("#dropZone").removeClass("accept deny");

    return false;
}

function openDragInputModal(e){
    e.stopPropagation();
    e.preventDefault();

    //console.log('openDragInputModal via '+e.type + ' on '+ e.target.nodeName + '#' + e.target.id);

    if( !$('body').hasClass('incomingFile') ){
        $("#fileDropModal").modal('show');
    }

    // check filetype when entering droppable zone
    if( e.type === 'dragenter' ){
        onDragUpdateFile(e);
    }

    return false;
}

//----------------------------------------
function saveDefaultSettings() {
    if(!defaultSettings) return;
    if(isFirstTimeSierra) return;

    const defaultSettingsJsonString = JSON.stringify(defaultSettings, null, '\t');
    const result = ipcRenderer.sendSync('saveDefaultSettings', defaultSettingsJsonString);
    console.log(result);
}

//----------------------------------------
function generate() {
    // let's get all the info:
    const platformValueArray = getPlatformList();

    const templatePicked = $("#templatesDropdown .active");
    const templateValueArray = [];
    for (let i = 0; i < templatePicked.length; i++){
        templateValueArray.push($(templatePicked[i]).attr("data-value"));
    }

    const addonsPicked = $("#addonsDropdown  .active");
    const addonValueArray = [];

    for(let i = 0; i < addonsPicked.length; i++) {
        addonValueArray.push($(addonsPicked[i]).attr("data-value"));
    }

    // add any local addons
    for(let i = 0; i < localAddons.length; i++) {
        addonValueArray.push(localAddons[i]);
    }

    // extra source locations
    const srcExtraArr = [];
    for(let i = 0; i < numAddedSrcPaths; i++) {
        const srcExtra = $("#sourceExtra-" + i).val();
        if( srcExtra != '' ){
            srcExtraArr.push(srcExtra);
        }
    }
    const srcExtraList = srcExtraArr.join(',');

    const lengthOfPlatforms = platformValueArray.length;

    const gen = {
        projectName: $("#projectName").val(),
        projectPath: $("#projectPath").val(),
        sourcePath: srcExtraList,
        platformList: platformValueArray,
        templateList: templateValueArray,
        addonList: addonValueArray,  //$("#addonsDropdown").val();
        ofPath: $("#ofPath").val(),
        verbose: bVerbose
    };

    // console.log(gen);
    if (gen.projectName === '') {
        $("#projectName").oneTimeTooltip("Please name your sketch first.");
    } else if (gen.projectPath === '') {
        $("#projectPath").oneTimeTooltip("Your project path is empty...");
    } else if (gen.platformList == null || lengthOfPlatforms == 0) {
        $("#platformsDropdown").oneTimeTooltip("Please select a platform first.");
    } else {
        ipcRenderer.send('generate', gen);
    }
}


//----------------------------------------
function updateRecursive() {
    // get the path and the platform list
    // platformsDropdownMulti

    const platformsPicked = $("#platformsDropdownMulti  .active");
    const platformValueArray = [];
    for (let i = 0; i < platformsPicked.length; i++){
        platformValueArray.push($(platformsPicked[i]).attr("data-value"));
    }

    const templatePicked = $("#templatesDropdownMulti .active");
    const templateValueArray = [];
    for (let i = 0; i < templatePicked.length; i++){
        templateValueArray.push($(templatePicked[i]).attr("data-value"));
    }

    const gen = {
        updatePath: $("#updateMultiplePath").val(),
        platformList: platformValueArray,
        templateList: templateValueArray,
        updateRecursive: true,
        ofPath: $("#ofPath").val(),
        verbose: bVerbose
    };

    if (gen.updatePath === '') {
        displayModal("Please set update path");
    } else if (platformValueArray.length === 0) {
        displayModal("Please select a platform first.");
    } else {
        ipcRenderer.send('update', gen);
    }
}

//----------------------------------------
/**
 * 
 * @param {'createMode' | 'updateMode'} mode 
 */
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
        clearExtraSourceList();
    }
    // [default]: switch to createMode (generate new projects)
    else {
        // if previously in update mode, deselect Addons
        if( $("#updateButton").is(":visible") ){
            clearAddonSelection();
            clearExtraSourceList();
        }

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
        $('#sourceExtraSection').show();
        $('#templateSection').show();
        $('#templateSectionMulti').show();
    } else {
        $('#platformsDropdown').addClass("disabled");
        $('#platformsDropdown').dropdown('set exactly', defaultSettings.defaultPlatform);
        $('#sourceExtraSection').hide();
        $('#templateSection').hide();
        $('#templateSectionMulti').hide();
        $('#templateDropdown').dropdown('set exactly', '');
        $('#templateDropdownMulti').dropdown('set exactly', '');

        $("body").removeClass('advanced');
        $('a.updateMultiMenuOption').hide();
    }
    defaultSettings.advancedMode = isAdvanced;
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
function getPlatformList() {
    const platformsPicked = $("#platformsDropdown  .active");
    const platformValueArray = [];
    for (let i = 0; i < platformsPicked.length; i++){
        platformValueArray.push($(platformsPicked[i]).attr("data-value"));
    }
    return platformValueArray;
}

//----------------------------------------
function displayModal(message) {
    $("#uiModal .content")
        .html(message)
        .find('*[data-toggle="external_target"]')
        .click((e) => {
            e.preventDefault();
            ipcRenderer.send('openExternal', $(e.currentTarget).prop("href") );
        });

    if (message.indexOf("Success!") > -1){
        $("#IDEButton").show();
    } else {
        $("#IDEButton").hide();
    }

    $("#uiModal").modal('show');
}

//----------------------------------------
function consoleMessage(orig_message) {
    const message = (orig_message + '').replace(/([^>\r\n]?)(\r\n|\n\r|\r|\n)/g, '$1' + "<br>\n" + '$2'); // nl2br
    $("#console").append($("<p>").html(message));
    $("#consoleContainer").scrollTop($('#console').offset().top); // scrolls console to bottom
}

//-----------------------------------------------------------------------------------
// Button calls
//-----------------------------------------------------------------------------------

function quit(){
    ipcRenderer.send('quit', '');
}
function browseOfPath() {
    ipcRenderer.send('pickOfPath', ''); // current path could go here (but the OS also remembers the last used folder)
}

function browseProjectPath() {
    let projectPath = $("#projectPath").val();
    if (projectPath === ''){
        projectPath = $("#ofPath").val();
    }
    ipcRenderer.send('pickProjectPath', projectPath); // current path could go here
}

function clearExtraSourceList(){
    $("#sourceExtraSection").empty();
    $("#sourceExtraSection").append("<label>Additional source folders:</label>");
    
    checkAddSourcePath(-1);
    numAddedSrcPaths = 1;
}

function checkAddSourcePath(index){
    //if we don't have another field below us - add one
    const nextFieldId = '#sourceExtra-' + (index + 1);
    if( $(nextFieldId).length == 0 ) {
        const nextIndex = index + 1;
        const extrafield = `<div class="field">
           <div class="ui icon input fluid">
               <input type="text" placeholder="Extra source path..." id="sourceExtra-${nextIndex}"> \
               <i class="search link icon" onclick="browseSourcePath(${nextIndex})"></i> \
           </div>
        </div>`;

        $("#sourceExtraSection").append(extrafield);
        numAddedSrcPaths++;
    }
}

function browseSourcePath(index) {
    const ofPath = $("#ofPath").val();
    ipcRenderer.send('pickSourcePath', [ ofPath, index ]); // current path could go here
}


function browseImportProject() {
    let projectPath = $("#projectPath").val();
    if (projectPath === ''){
        projectPath = $("#ofPath").val();
    }
    ipcRenderer.send('pickProjectImport', projectPath);
}

function getUpdatePath() {
    let updateMultiplePath = $("#updateMultiplePath").val();
    if (updateMultiplePath === ''){
        updateMultiplePath = $("#ofPath").val();
    }

    ipcRenderer.send('pickUpdatePath', updateMultiplePath); // current path could go here
}

function rescanAddons() {
    ipcRenderer.sendSync('refreshAddonList', $("#ofPath").val());

    const projectInfo = {
        'projectName': $("#projectName").val(),
        'projectPath': $("#projectPath").val(),
    };
    ipcRenderer.send('isOFProjectFolder', projectInfo);     // <- this forces addon reload
}

function getRandomSketchName(){
    const projectPath = $("#projectPath").val();
    if (projectPath === '') {
        $("#projectPath").oneTimeTooltip('Please specify a path first...');
    }
    else {
        const result = ipcRenderer.sendSync('getRandomSketchName', projectPath);
        const {
            randomisedSketchName,
            generateMode
        } = result;
        $("#projectName").val(randomisedSketchName);
        switchGenerateMode(generateMode);
    }
}

function launchInIDE(){
    const platform = getPlatformList()[0];

    const project = {
        'projectName': $("#projectName").val(),
        'projectPath': $("#projectPath").val(),
        'platform': platform,
        'ofPath': $("#ofPath").val()
    };

    ipcRenderer.send('launchProjectinIDE', project );
}
