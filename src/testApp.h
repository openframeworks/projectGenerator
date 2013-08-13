#pragma once

//#define COMMAND_LINE_ONLY

#include "ofMain.h"
#include "CBLinuxProject.h"
#include "CBWinProject.h"
#include "visualStudioProject.h"
#include "xcodeProject.h"
#include <Poco/Path.h>

#include "ofxGui.h"
#include "ofAddon.h"
#include "ofxXmlSettings.h"
#include "textButton.h"



class testApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();

		void keyPressed  (int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);
 
    
        string sketchName;
        string sketchPath;
        string addons;
        string platform;
    
        ofxPanel panelCoreAddons;
        ofxPanel panelOtherAddons;
        bool bHaveNonCoreAddons;
    
        ofxPanel panelPlatforms;
    
        ofxToggle osxToggle, iosToggle, wincbToggle, winvsToggle, linuxcbToggle, linux64cbToggle;

        ofTrueTypeFont font;
        ofTrueTypeFont titleFont;
        ofTrueTypeFont secondFont;
    
        int mode;
        enum { MODE_NORMAL, MODE_ADDON, MODE_PLATFORM };
    
        baseProject * project;
    
        string setupForTarget(int targ);
    
        void generateProject();
    
        string addonsPath;
        string status;
    
        ofxXmlSettings XML;
        string appToRoot;
        string defaultLoc;
		
		float uiLeftX; 
    
    
        textButton  button;
        textButton  generateButton;
        textButton  addonButton;
        vector < textButton > buttons;

        bool isAddonCore(string addon);
        bool bInited;
        vector < string  > coreAddons;
    
        
        float statusSetTime;
        float statusEnergy;
        void setStatus(string newStatus);
        
        ofImage logo;
		void addAddon(string addon);
    
};
