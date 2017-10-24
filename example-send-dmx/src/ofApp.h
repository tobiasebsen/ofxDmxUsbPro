#pragma once

#include "ofMain.h"
#include "ofxDmxUsbPro.h"

class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();
		
		ofxDmxUsbPro dmxpro;
		unsigned char channels[24];
};
