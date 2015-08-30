#pragma once

//#define COMMAND_LINE_ONLY

#include "ofMain.h"
#include "CBLinuxProject.h"
#include "CBWinProject.h"
#include "visualStudioProject.h"
#include "xcodeProject.h"
#include <Poco/Path.h>

#ifndef COMMAND_LINE_ONLY
#include "ofxGui.h"
#endif

class ofApp : public ofBaseApp{

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
 
		void setupForTarget(ofTargetPlatform targ);
        void generateExamplesCB();
		void generateExamples();
		
        ofFileDialogResult makeNewProjectViaDialog();
        ofFileDialogResult updateProjectViaDialog();

        void createProjectPressed();
        void updateProjectPressed();
        void createAndOpenPressed();
        void changeOFRootPressed();
		
		void setupDrawableOFPath();
		
		std::unique_ptr<baseProject> project;
    
        std::string projectPath;
        std::string target;
        std::vector <ofTargetPlatform> targetsToMake;
		bool buildAllExamples;

#ifndef COMMAND_LINE_ONLY
		std::string drawableOfPath;
		ofRectangle ofPathRect;
		ofPoint ofPathDrawPoint;

        ofxPanel panelAddons, panelOptions;
        ofxButton createProject, updateProject, createAndOpen, changeOFRoot;

		ofxPanel examplesPanel;
		ofxToggle osxToggle, iosToggle, wincbToggle, winvsToggle, linuxcbToggle, linux64cbToggle,linuxarmv6lcbToggle,linuxarmv7lcbToggle;
		ofxButton generateButton;
#endif
};
