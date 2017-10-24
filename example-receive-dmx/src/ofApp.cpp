#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){

	ofSetLogLevel(OF_LOG_NOTICE);

	dmxpro.setup();
	dmxpro.setReceiveDmxOnChange(false);
	ofAddListener(dmxpro.dmxReceived, this, &ofApp::dmxReceived);
}

//--------------------------------------------------------------
void ofApp::update(){

	dmxpro.update();
}

//--------------------------------------------------------------
void ofApp::draw(){

}

//--------------------------------------------------------------
void ofApp::dmxReceived(ofxDmxUsbPro::DmxData & data) {
	uint8_t r = data.data[0];
	uint8_t g = data.data[1];
	uint8_t b = data.data[2];
	ofColor color(r, g, b);
	ofBackground(color);
}