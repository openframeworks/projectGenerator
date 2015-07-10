#include "ofMain.h"
#include "ofApp.h"
#include "ofAppGlutWindow.h"
#include "ofAppNoWindow.h"


//========================================================================
int main(  int argc, char *argv[]  ){
    ofSetupOpenGL(1024, 610, OF_WINDOW);
    testApp * app = new testApp;
    //app->buildAllExamples = false;
    ofRunApp( app );    
}
