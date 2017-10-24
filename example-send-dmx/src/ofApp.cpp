#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){

	ofSetLogLevel(OF_LOG_NOTICE);
	ofSetFrameRate(30);

	dmxpro.setup();
}

//--------------------------------------------------------------
void ofApp::update(){

	for (int i=0; i<24; i++)
		channels[i] = (unsigned char)ofGetFrameNum();

	dmxpro.sendDmx(channels, 24);
}

//--------------------------------------------------------------
void ofApp::draw(){

}
