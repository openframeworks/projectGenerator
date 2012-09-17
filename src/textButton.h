//
//  textButton.h
//  projectGenerator
//
//  Created by molmol on 9/17/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef projectGenerator_textButton_h
#define projectGenerator_textButton_h

class textButton{
public: 
    
    ofPoint topLeftAnchor;
    string myDisplayText;
    string myText;
    string prefix;
    ofTrueTypeFont * font;
    ofRectangle rect;
    bool bSelectable;
    
    string deliminater;
    int maxWidth;
    
    
    bool bMouseOver;
    ofColor off;
    ofColor on;
    
    
    void setText(string newText){
        
        myText = newText;
        
        newText = prefix + newText;
        
        cout << newText << endl;
        vector < string > breakUp;
        breakUp = ofSplitString(newText, deliminater);
        
        ofPoint pos;
        pos.set(0,0);
        
        myDisplayText = "";
        
        for (int i = 0; i < breakUp.size(); i++){
            string text = breakUp[i];
            if (i != breakUp.size() -1) text += deliminater;
            //if (breakUp[i].length() == 0) text += deliminater;
            
            cout << text << endl;
            ofRectangle rect = font->getStringBoundingBox(text, pos.x, pos.y);
            if ((pos.x + rect.width) > 300){
                
                myDisplayText += "\n";
                myDisplayText += text;
                pos.x = rect.width;
            } else {
                
                myDisplayText+= text;
                pos.x += rect.width;
            }
            
            
        }
        
        
        
    }
    
    
    textButton(){
        bSelectable = true;
        bMouseOver = false;
        //off.set(ofColor::magenta.r, ofColor::magenta.g, ofColor::magenta.b);
        //on.set(ofColor::blue.r, ofColor::blue.g, ofColor::blue.b);
        //cout << ofColor::magenta<< endl;
        
        deliminater = "/";
        
        on.set(50,130,70);
        //off = on;
        off.setHsb(on.getHue()* 0.95, on.getSaturation()*1.3, on.getBrightness()*1.2);
        
    }
    
    void calculateRect() {
        rect = font->getStringBoundingBox( myDisplayText, topLeftAnchor.x,topLeftAnchor.y);
        
        rect.x -= 10;
        rect.y -= 10;
        rect.width += 20;
        rect.height += 20;
        
    }
    
    void checkMousePressed(ofPoint mouse){
        if (bSelectable == false){ bMouseOver = false; return;}
        if (rect.inside(mouse)){
            bMouseOver = true;
        } else {
            bMouseOver = false;
        }
    }
    
    void update();
    void draw() {
        
        //cout << off<< endl;
        
        if (bMouseOver == true) ofSetColor(on);
        else ofSetColor(off);
        ofRect(rect);
        
        
        ofSetColor(ofColor::gray);
        //font->drawString(myText, topLeftAnchor.x+1, topLeftAnchor.y+1);
        
        ofSetColor(ofColor::white);
        font->drawString(myDisplayText, topLeftAnchor.x, topLeftAnchor.y);
        
    }
    
    ofRectangle bounds;
    
    
};

#endif
