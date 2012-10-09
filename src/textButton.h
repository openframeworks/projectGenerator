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
    ofTrueTypeFont * secondFont;
    ofRectangle rect;
    bool bSelectable;
    bool bDrawLong;

    string secondaryText;


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
            if ((pos.x + rect.width) > 500){

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
        bDrawLong = true;
        //off.set(ofColor::magenta.r, ofColor::magenta.g, ofColor::magenta.b);
        //on.set(ofColor::blue.r, ofColor::blue.g, ofColor::blue.b);
        //cout << ofColor::magenta<< endl;

        deliminater = "/";

        //setColor(ofColor(60,170,100));
    }

	void setColor( ofColor newOnColor ){
		on = newOnColor;
        on.set(0,0,0);
        off.set(0,0,0);
		//updateOffColor();
	}

	void updateOffColor(){
        off.setHsb(0,0,0);
	}

    void calculateRect() {
        rect = font->getStringBoundingBox( myDisplayText, topLeftAnchor.x, topLeftAnchor.y);

        rect.x -= 12;
        rect.y -= 12;
        rect.width += 22;
        rect.height += 23;

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
        ofPushStyle();
			ofFill();

            if (bDrawLong == true){
//                ofSetColor(220,220,220);

                ofSetColor(220,220,220);
                ofFill();
                ofRect(rect.x, rect.y, ofGetWidth() - rect.x*2, rect.height);

                ofRectangle rectString = secondFont->getStringBoundingBox(secondaryText, 0, 0);
                float h = rectString.height;
                float y = (rect.y + rect.height/2) + (rectString.height)/2;
                float x = (rect.x + ofGetWidth() - rect.x*2) - rectString.width - 10;

                ofFill();
                ofSetColor(0,0,0);
                secondFont->drawString(secondaryText, x,y);
                //button.secondaryText;

            }


			if( bSelectable ){
				if (bMouseOver == true) ofSetColor(140,140,140);
				else ofSetColor(0,0,0);
			}else{
				ofSetColor(160, 160, 160);
			}
			ofRect(rect);

			ofColor darkOutline = on;
			darkOutline *= 0.65;

			//ofSetColor(darkOutline);
			//ofNoFill();
			//ofRect(rect);

			//ofSetColor(ofColor::gray);
			//font->drawString(myText, topLeftAnchor.x+1, topLeftAnchor.y+1);

			ofSetColor(74,255,203);
			font->drawString(myDisplayText, topLeftAnchor.x, topLeftAnchor.y);


		ofPopStyle();
    }

    ofRectangle bounds;


};

#endif
