#include "testApp.h"
#include "Utils.h"
#include <stdio.h>
#include "ofConstants.h"

#ifdef TARGET_OSX
#include "g2AppKit.h"
#endif



//------------------------------------------------------
bool testApp::isAddonCore(string addon){    
    if (bInited == false){
        coreAddons.push_back("ofx3DModelLoader");
        coreAddons.push_back("ofxAssimpModelLoader");
        coreAddons.push_back("ofxDirList");
        coreAddons.push_back("ofxNetwork");
        coreAddons.push_back("ofxOpenCv");
        coreAddons.push_back("ofxOsc");
        coreAddons.push_back("ofxThread");
        coreAddons.push_back("ofxThreadedImageLoader");
        coreAddons.push_back("ofxVectorGraphics");
        coreAddons.push_back("ofxVectorMath");
        coreAddons.push_back("ofXmlSettings");
        bInited = true;
    }
    for (int i = 0; i < coreAddons.size(); i++){
        if (coreAddons[i] == addon){
            return true;
        }
    }
    return false;
}

//------------------------------------------------------
// for project names, let's fix replace any odd characters with _
void fixStringCharacters(string &toFix){
    
    // replace all non alpha numeric (ascii) characters with _
    for (int i = 0; i < toFix.size(); i++){
        int which = (int)toFix[i];
        if ((which >= 48 && which <= 57) || 
            (which >= 65 && which <= 90) || 
            (which >= 97 && which <= 122)){
        } else {
            toFix[i] = '_';
        }
    }
}



//------------------------------------------------------
string testApp::setupForTarget(int targ){

    if(project){
		delete project;
	}    
    string target;
    switch(targ){
        case OF_TARGET_OSX:
            project = new xcodeProject;
            if (useDataFolderTemplates) project->setTemplatePath(ofToDataPath("templates/osx/"));
            target = "osx";
            break;
        case OF_TARGET_WINGCC:
            project = new CBWinProject;
           if (useDataFolderTemplates)  project->setTemplatePath(ofToDataPath("templates/win_cb/"));
            target = "win_cb";
            break;
        case OF_TARGET_WINVS:
            project = new visualStudioProject;
            if (useDataFolderTemplates) project->setTemplatePath(ofToDataPath("templates/vs2010/"));
            target = "vs2010";
            break;
        case OF_TARGET_IPHONE:
            project = new xcodeProject();
            if (useDataFolderTemplates) project->setTemplatePath(ofToDataPath("templates/ios/"));
            target = "ios";
            break;
        case OF_TARGET_ANDROID:
            break;
        case OF_TARGET_LINUX:
            project = new CBLinuxProject;
            if (useDataFolderTemplates) project->setTemplatePath(ofToDataPath("templates/linux/"));
            target = "linux";
            break;
        case OF_TARGET_LINUX64:
            project = new CBLinuxProject;
            if (useDataFolderTemplates) project->setTemplatePath(ofToDataPath("templates/linux64/"));
            target = "linux64";
            break;
    }
    
    project->setup(target);
    
    return target;
}






//--------------------------------------------------------------
void testApp::setup(){
    
    
    
    mode = 0;
    bInited = false;
    project = NULL;
    sketchName = "mySketch";
    
    
    //-------------------------------------
    // get settings
    //-------------------------------------
    
    XML.loadFile("projectGeneratorSettings.xml");
    appToRoot = XML.getValue("appToRoot", "../../../../");
    defaultLoc = XML.getValue("defaultNewProjectLocation", "apps/myApps");
    useDataFolderTemplates = XML.getValue("useDataFolderTemplates", "true") == "true" ? true : false;
    
    //-------------------------------------
    // calculate the bin path (../../../ on osx) and the sketch path (bin -> root - > defaultLoc)
    //-------------------------------------
    
    // if appToRoot is wrong, we have alot of issues.  all these paths are used in this project: 
    
#ifdef TARGET_OSX
    string binPath = ofFilePath::getAbsolutePath(ofFilePath::join(ofFilePath::getCurrentWorkingDirectory(), "../../../"));
#else 
    string binPath = ofFilePath::getCurrentWorkingDirectory();
#endif
    
    setOFRoot(ofFilePath::getAbsolutePath(ofFilePath::join(binPath, appToRoot)));
    
    addonsPath = ofFilePath::getAbsolutePath(ofFilePath::join(binPath, ofFilePath::join(appToRoot,"addons")));    
    sketchPath = ofFilePath::getAbsolutePath(ofFilePath::join(binPath,  ofFilePath::join(appToRoot, defaultLoc)));    
    
    
    
    
    
    //-------------------------------------
    // get settings
    //-------------------------------------
    
        
    //-------------------------------------
    // load font and setup the buttons
    font.loadFont("frabk.ttf", 12, false, false);
    
    // sketch button
    button.font = &font;
    button.prefix = "name: ";
    button.setText(sketchName);
    buttons.push_back(button);
    
    // path button
    button.deliminater = "/";
    button.prefix = "path: ";
    button.setText(sketchPath);
    buttons.push_back(button);
    
    button.deliminater = ", ";
    button.prefix = "platforms: ";
    button.bSelectable = false;
    button.setText(platform);
    
    button.topLeftAnchor.set(button.topLeftAnchor.x, button.topLeftAnchor.y + button.rect.height + 20);
    buttons.push_back(button);
    
    
    button.deliminater = ", ";
    button.prefix = "addons: ";
    button.bSelectable = true;
    button.setText(addons);
    
    button.topLeftAnchor.set(button.topLeftAnchor.x, button.topLeftAnchor.y + button.rect.height + 20);
    buttons.push_back(button);
    
    button.deliminater = ",";
    button.prefix = "generate";
    button.bSelectable = true;
    button.setText("");
    button.topLeftAnchor.set(50,ofGetHeight()-80);
    buttons.push_back(button);
    
    addonButton = button;
    addonButton.prefix = "< back";
    addonButton.setText("");
    
     for (int i = 0; i < buttons.size(); i++){
         buttons[i].calculateRect();
     }
    
    addonButton.calculateRect();
    
    //-------------------------------------
    // addons panels: 
    //-------------------------------------
    
    panelCoreAddons.setup();
    panelOtherAddons.setup();
    
    ofDirectory addons(addonsPath);

    addons.listDir();
    for(int i=0;i<(int)addons.size();i++){
    	string addon = addons.getName(i);
    	if(addon.find("ofx")==0){
    		
            if (isAddonCore(addon)){
                ofxToggle * toggle = new ofxToggle();
                panelCoreAddons.add(toggle->setup(addon,false,300));
            } else {
                ofxToggle * toggle = new ofxToggle();
                panelOtherAddons.add(toggle->setup(addon,false,300));
            }
            
            
    	}
    }
    
    //-------------------------------------
    // platform panel (not used, really, but here just in case)
    //-------------------------------------
    
    panelPlatforms.setup();
    panelPlatforms.add(wincbToggle.setup("windows (codeblocks)",ofGetTargetPlatform()==OF_TARGET_WINGCC));
	panelPlatforms.add(winvsToggle.setup("windows (visualStudio)", ofGetTargetPlatform()==OF_TARGET_WINVS));
	panelPlatforms.add(linuxcbToggle.setup("linux (codeblocks)",ofGetTargetPlatform()==OF_TARGET_LINUX));
	panelPlatforms.add(linux64cbToggle.setup("linux64 (codeblocks)",ofGetTargetPlatform()==OF_TARGET_LINUX64));
	panelPlatforms.add(osxToggle.setup("osx (xcode)",ofGetTargetPlatform()==OF_TARGET_OSX));
	panelPlatforms.add(iosToggle.setup("ios (xcode)",ofGetTargetPlatform()==OF_TARGET_IPHONE));
    
    // update the platforms text in the platform button
    string platforms = "";
    for (int i = 0; i < panelPlatforms.getNumControls(); i++){
        if (*((ofxToggle *)panelPlatforms.getControl(i))){
            if (platforms.length() > 0) platforms+=", ";
            platforms += ((ofxToggle *)panelPlatforms.getControl(i))->getName();
            
        };
    }
    buttons[2].setText(platforms);
    
    
    panelPlatforms.setPosition(300,0);
    panelCoreAddons.setPosition(300,0);
    panelOtherAddons.setPosition(750,0);

    
  
    
}






//--------------------------------------------------------------
void testApp::update(){


    //-------------------------------------
    // if we are in addon mode check 
    //-------------------------------------
    
    if (mode == MODE_ADDON) addonButton.checkMousePressed(ofPoint(mouseX, mouseY));
    
    
    //-------------------------------------
    // layout our normal buttons, check the mouse
    //-------------------------------------
    
    for (int i = 0; i < buttons.size(); i++){
        buttons[i].calculateRect();
        buttons[i].checkMousePressed(ofPoint(mouseX, mouseY));
    }

    for (int i = 0; i < buttons.size(); i++){
        if (i == 0){
            buttons[i].topLeftAnchor.set(200,80);
        } else {
            if (i != buttons.size()-1)
             buttons[i].topLeftAnchor.set(buttons[i-1].topLeftAnchor.x, buttons[i-1].topLeftAnchor.y +buttons[i-1].rect.height + 20);
        }
    }
    
    
    //-------------------------------------
    // addons panels can be really long, so use the mouse pos to move them if we need to
    //-------------------------------------
    
    if (panelCoreAddons.getShape().height > ofGetHeight()){
        float pct = ofMap(ofGetMouseY(), 0,ofGetHeight(), 0,1,true);
        float diff = panelCoreAddons.getShape().height - ofGetHeight();
        panelCoreAddons.setPosition(300,-diff * pct);
    }
    
    if (panelOtherAddons.getShape().height > ofGetHeight()){
        float pct = ofMap(ofGetMouseY(), 0,ofGetHeight(), 0,1,true);
        float diff = panelOtherAddons.getShape().height - ofGetHeight();
        panelOtherAddons.setPosition(750,-diff * pct);
    }
  
}

//--------------------------------------------------------------
void testApp::draw(){

    
    ofBackgroundGradient(ofColor(190,190,190), ofColor(130,130,130), OF_GRADIENT_LINEAR);
    
    if (mode == 0){
    
    for (int i = 0; i < buttons.size(); i++){
        buttons[i].draw();
    }
    
    } else if (mode == 1){
    
        panelCoreAddons.draw();
        panelOtherAddons.draw();
    } else if (mode == 2){
        panelPlatforms.draw();
    }
    //cout << panelAddons.getShape().height << endl;
    
    
    if (mode == 0){
        ofFill();
        ofSetColor(0,0,0);
        ofRect(0,ofGetHeight(), ofGetWidth(), -25);
        ofSetColor(255,255,255);
        ofDrawBitmapString(status, 10,ofGetHeight()-8);
        
    }
    if (mode == 1){
        addonButton.draw();
    }
}

//--------------------------------------------------------------
void testApp::keyPressed(int key){

    if (key == ' '){
        
        //printf("%s -------- \n", ResultBuffer);
        //std::exit(0);
    }
    
    
}

void testApp::generateProject(){
    
    vector <int> targetsToMake;
	if( osxToggle )		targetsToMake.push_back(OF_TARGET_OSX);
	if( iosToggle )		targetsToMake.push_back(OF_TARGET_IPHONE);
	if( wincbToggle )	targetsToMake.push_back(OF_TARGET_WINGCC);
	if( winvsToggle )	targetsToMake.push_back(OF_TARGET_WINVS);
	if( linuxcbToggle )	targetsToMake.push_back(OF_TARGET_LINUX);
	if( linux64cbToggle )	targetsToMake.push_back(OF_TARGET_LINUX64);
    
	if( targetsToMake.size() == 0 ){
		cout << "Error: makeNewProjectViaDialog - must specifiy a project to generate " <<endl;
		ofSystemAlertDialog("Error: makeNewProjectViaDialog - must specifiy a project platform to generate");
        return;
	}
    
    if (buttons[0].myText.size() == 0){
        ofSystemAlertDialog("Error: project must have a name");
        return;
    }
    
    
    
    printf("start with project generation \n");
    
    string path = buttons[1].myText + "/" + buttons[0].myText;
    
	for(int i = 0; i < (int)targetsToMake.size(); i++){
		string target = setupForTarget(targetsToMake[i]);
        if(project->create(path)){
            vector<string> addonsToggles = panelCoreAddons.getControlNames();
            for (int i = 0; i < (int) addonsToggles.size(); i++){
                ofxToggle toggle = panelCoreAddons.getToggle(addonsToggles[i]);
                if(toggle){
                    ofAddon addon;
                    addon.pathToOF = getOFRelPath(path);
                    addon.fromFS(ofFilePath::join(addonsPath, addonsToggles[i]),target);
                    project->addAddon(addon);
                    
                }
            }
            
            
            addonsToggles = panelOtherAddons.getControlNames();
            for (int i = 0; i < (int) addonsToggles.size(); i++){
                ofxToggle toggle = panelOtherAddons.getToggle(addonsToggles[i]);
                if(toggle){
                    ofAddon addon;
                    addon.pathToOF = getOFRelPath(path);
                    addon.fromFS(ofFilePath::join(addonsPath, addonsToggles[i]),target);
                    project->addAddon(addon);
                    
                }
            }
            
            project->save(true);
        }
	}
    
    
    printf("done with project generation \n");
    status = "generated: " + buttons[1].myText + "/" + buttons[0].myText;

    // go through the control panels, do stuff
}

//--------------------------------------------------------------
void testApp::keyReleased(int key){

    
    
}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){
    
    if (mode == MODE_NORMAL){
        
        
        // check the mouse for press
        
        for (int i = 0; i < buttons.size(); i++){
            buttons[i].checkMousePressed(ofPoint(x, y));
        }
        
        //-------------------------------------
        // 4 = genearate 
        //-------------------------------------
        
        if (buttons[4].bMouseOver == true){
            generateProject();
        }
        
        //-------------------------------------
        // 0 = sketch name 
        //-------------------------------------
        
        if (buttons[0].bMouseOver == true){
            string text = ofSystemTextBoxDialog("choose sketch name", buttons[0].myText);
            fixStringCharacters(text);
            status = "sketch name set to: " + text;
            buttons[0].setText(text);
        }
        
        //-------------------------------------
        // 1 = sketch path 
        //-------------------------------------
        
        if (buttons[1].bMouseOver == true){
            
             printf("here2? \n");
            
            string command;
            
            
            ofDirectory dir(ofFilePath::join(getOFRoot(),defaultLoc));
            
            //cout << dir.getAbsolutePath() << endl;
            
            if (!dir.exists()){
                dir.create();
            }
            
#ifdef TARGET_OSX
            
            printf("here? \n");
            
            char * MessageBuffer = "please select sketch folder";
            char * FileExtension = "";
            char ResultBuffer[1024];
            memset(ResultBuffer, 0,1024);
            
            string defaultStr = ofFilePath::join(getOFRoot(),defaultLoc);
            cout << defaultStr << endl;
            if (__g2ShowOpenDialog((char *)defaultStr.c_str(), MessageBuffer, FileExtension, ResultBuffer, 1024)){
            
                string res = string(ResultBuffer);
                buttons[1].setText(res);
                status = "sketch path set to: " + res;
                
            }
#elif TARGET_LINUX
            
            /*ofFileDialogResult results;
            if(bFolderSelection){
                
                .set_current_folder_uri(get_current_folder_uri());
                results.filePath = gtkFileDialog(GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,windowTitle);
            }*/
            
            // TODO: implement folder preselection for the choose directory dialog. 
            // gtk_file_chooser_set_current_folder_uri	
            
#elif TARGET_WINDOWS    
            
            // TODO: implement folder preselection for the choose directory dialog. 
#endif
        
        
        }

        
        //-------------------------------------
        // 2 = platform  (disabled)
        //-------------------------------------
        
        
        if (buttons[2].bMouseOver == true){
            // platform is diabled for now
            // mode = 2;
        }
        
        //-------------------------------------
        // 3 = addon 
        //-------------------------------------
        
        if (buttons[3].bMouseOver == true){
            mode = MODE_ADDON;
            
        }
    }
    
    //-------------------------------------
    // handle addon mode
    //-------------------------------------
    
    if (mode == MODE_ADDON){
        
        //-------------------------------------
        // if we hit he back button, collect the addons for display
        //-------------------------------------
    
        if (addonButton.bMouseOver){
            
            string addons = "";
            
            for (int i = 0; i < panelCoreAddons.getNumControls(); i++){
                if (*((ofxToggle *)panelCoreAddons.getControl(i))){
                   if (addons.length() > 0) addons+=", ";
                    addons += ((ofxToggle *)panelCoreAddons.getControl(i))->getName();
                    
                };
                
            }
            for (int i = 0; i < panelOtherAddons.getNumControls(); i++){
                if (*((ofxToggle *)panelOtherAddons.getControl(i))){
                    if (addons.length() > 0) addons+=", ";
                    addons += ((ofxToggle *)panelOtherAddons.getControl(i))->getName();
                    
                };
                
            }
            buttons[3].setText(addons);
            
            status = "addons set to: " + addons;
            
            addonButton.bMouseOver = false;
            mode = MODE_NORMAL;
        }
    }
    
    if (mode == MODE_PLATFORM){
        
        
        // we don't have platform selection enabled
        
        
//        //if (x < 100){ mode = MODE_NORMAL;
//        
//        
//        string platforms = "";
//        
//        for (int i = 0; i < panelPlatforms.getNumControls(); i++){
//            if (*((ofxToggle *)panelPlatforms.getControl(i))){
//                if (platforms.length() > 0) platforms+=", ";
//                platforms += ((ofxToggle *)panelPlatforms.getControl(i))->getName();
//                
//            };
//        }
//       
//        buttons[2].setText(platforms);
//        mode = 0;
//        }
        
        
    }
}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h){
    
}

//--------------------------------------------------------------
void testApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void testApp::dragEvent(ofDragInfo dragInfo){

}
