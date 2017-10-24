#pragma once

#include "ofMain.h"
#include "ofxDmxUsbPro.h"

class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();

		void dmxReceived(ofxDmxUsbPro::DmxData & data);
		
		ofxDmxUsbPro dmxpro;
};
