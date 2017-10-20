#include "ofxDmxUsbPro.h"

#define DMX_START_CODE 0x7E 
#define DMX_END_CODE 0xE7 

#define LABEL_GET_WIDGET_PARAMS		3
#define LABEL_SET_WIDGET_PARAMS		4
#define LABEL_PACKET_RECEIVED		5
#define LABEL_SEND_DMX				6
#define LABEL_SEND_RDM				7
#define LABEL_SET_DMX_CHANGE		8
#define LABEL_DMX_CHANGED			9
#define LABEL_GET_SERIAL			10
#define LABEL_SEND_RDM_DISC			11

ofxDmxUsbPro::ofxDmxUsbPro() {
	memset(&widgetParameters, 0, sizeof(widgetParameters));
	serialNumber = 0;
}

bool ofxDmxUsbPro::setup(int deviceNumber) {
	return serial.setup(deviceNumber, 57600);
}

bool ofxDmxUsbPro::setup(string portName) {
	return serial.setup(portName, 57600);
}

void ofxDmxUsbPro::update() {
	while(receiveMessage() > 0) {
		if (message[0] == DMX_START_CODE) {
			uint8_t label = message[1];
			uint16_t length = getLength();
			uint8_t * data = getData();
			if (label == LABEL_GET_WIDGET_PARAMS && length >= sizeof(widgetParameters)) {
				memcpy(&widgetParameters, data, sizeof(widgetParameters));
			}
			if (label == LABEL_PACKET_RECEIVED) {
				uint8_t status = data[0];
				uint8_t startCode = data[1];

				if (startCode == 0) { // DMX
					DmxData dmx;
					dmx.data = data + 2;
					dmx.size = length - 2;
					ofNotifyEvent(dmxReceived, dmx, this);
				}
				if (startCode == SC_RDM) { // RDM
					RdmMessage rdm(data + 1, length - 1);
					ofNotifyEvent(rdmReceived, rdm, this);
				}
				if (startCode == 0xFE && length == 25) { // RDM DISC_UNIQUE_BRANCH response
					RdmUid uid = RdmDecodeUid(data + 9);
					ofNotifyEvent(rdmDiscovered, uid, this);
				}
			}
			if (label == LABEL_GET_SERIAL && length == sizeof(serialNumber)) {
				memcpy(&serialNumber, data, sizeof(serialNumber));
			}
		}
	}
}

void ofxDmxUsbPro::requestWidgetParameters() {
	prepareMessage(LABEL_GET_WIDGET_PARAMS, 2);
	uint8_t * data = getData();
	data[0] = 0;
	data[1] = 0;
	sendMessage();
}

void ofxDmxUsbPro::setWidgetParameters(uint8_t breakTime, uint8_t mabTime, uint8_t refreshRate) {
	prepareMessage(LABEL_SET_WIDGET_PARAMS, 5);
	uint8_t * data = getData();
	data[0] = 0;
	data[1] = 0;
	data[2] = breakTime;
	data[3] = mabTime;
	data[4] = refreshRate;
	sendMessage();
}

void ofxDmxUsbPro::requestSerialNumber() {
	prepareMessage(LABEL_GET_SERIAL, 0);
	sendMessage();
}

void ofxDmxUsbPro::sendDmx(uint8_t * dmx, size_t length) {
	size_t size = length;
	if (length < 24)
		size = 24;
	if (length > 512)
		size = 512;

	prepareMessage(LABEL_SEND_DMX, size + 1);
	uint8_t * data = getData();
	data[0] = 0;
	memcpy(data + 1, dmx, length);
	sendMessage();
}

void ofxDmxUsbPro::sendRdm(uint8_t * rdm, size_t length) {
	prepareMessage(LABEL_SEND_RDM, length);
	uint8_t * data = getData();
	memcpy(data, rdm, length);
	sendMessage();
}

void ofxDmxUsbPro::sendRdm(RdmMessage & rdm) {
	rdm.setSource(getUid());
	rdm.updateChecksum();
	sendRdm(rdm.getPacket(), rdm.getPacketSize());
}

void ofxDmxUsbPro::setReceiveDmxOnChange(bool dmxChangeOnly) {
	prepareMessage(LABEL_SET_DMX_CHANGE, 1);
	uint8_t * data = getData();
	data[0] = dmxChangeOnly ? 1 : 0;
	sendMessage();
}

void ofxDmxUsbPro::sendRdmDiscovery() {
	RdmMessage msg;
	RdmDiscovery(msg);
	prepareMessage(LABEL_SEND_RDM_DISC, msg.getPacketSize());
	memcpy(getData(), msg.getPacket(), msg.getPacketSize());
	sendMessage();
}

bool ofxDmxUsbPro::getRdm(RdmUid & uid, uint16_t pid, RdmMessage & msg) {
	RdmMessage gmsg(uid, GET_COMMAND, pid);
	sendRdm(gmsg);
	ofSleepMillis(1);
	if (waitForReply(LABEL_PACKET_RECEIVED)) {
		uint8_t * data = getData();
		uint8_t status = data[0];
		uint8_t startCode = data[1];
		if (status == 0 && startCode == SC_RDM) {
			msg.setPacket(data + 1, getLength() - 1);
			return msg.getCommandClass() == GET_COMMAND_RESPONSE && msg.getParameterID() == pid;
		}
	}
	else
		ofLogWarning() << "REPLY TIMED OUT";

	return false;
}

void ofxDmxUsbPro::getWidgetParameters() {
	requestWidgetParameters();
	if (waitForReply(LABEL_GET_WIDGET_PARAMS)) {
		memcpy(&widgetParameters, getData(), sizeof(widgetParameters));
		ofLogVerbose() << "Firmware version: " << (int)widgetParameters.FirmwareMSB << "." << (int)widgetParameters.FirmwareLSB;
		ofLogVerbose() << "DMX output break time: " << (widgetParameters.BreakTime *  10.67f) << " micros (" << (int)widgetParameters.BreakTime << ")";
		ofLogVerbose() << "DMX output Mark After Break time: " << (widgetParameters.MaBTime * 10.67f) << " micros (" << (int)widgetParameters.MaBTime << ")";
		ofLogVerbose() << "DMX output rate: " << (int)widgetParameters.RefreshRate << " Hz";
	}
}

uint32_t ofxDmxUsbPro::getSerialNumber() {
	requestSerialNumber();
	if (waitForReply(LABEL_GET_SERIAL)) {
		memcpy(&serialNumber, getData(), sizeof(serialNumber));
		ofLogVerbose() << "Serial number: " << serialNumber;
		return serialNumber;
	}
	return 0;
}

string ofxDmxUsbPro::getSerialString() {
	if (serialNumber == 0)
		getSerialNumber();

	char sz[9];
	sprintf(sz, "%X", serialNumber);
	return string(sz);
}

RdmUid ofxDmxUsbPro::getUid() {
	if (serialNumber == 0)
		getSerialNumber();
	RdmUid uid;
	uid.uid[0] = 0x45;
	uid.uid[1] = 0x4E;
	uid.uid[2] = (serialNumber >> 24) & 0xFF;
	uid.uid[3] = (serialNumber >> 16) & 0xFF;
	uid.uid[4] = (serialNumber >> 8) & 0xFF;
	uid.uid[5] = (serialNumber >> 0) & 0xFF;
	return uid;
}

void ofxDmxUsbPro::prepareMessage(uint8_t label, size_t length) {
	message.resize(length + 5);
	message[0] = DMX_START_CODE;
	message[1] = label;
	message[2] = length & 0xFF;
	message[3] = (length >> 8) & 0xFF;
	message[length+4] = DMX_END_CODE;
}

uint8_t * ofxDmxUsbPro::getData() {
	return &message[4];
}

uint16_t ofxDmxUsbPro::getLength() {
	return message[2] | (message[3] << 8);
}

void ofxDmxUsbPro::sendMessage() {
	serial.writeBytes(message.data(), message.size());
}

int ofxDmxUsbPro::receiveMessage() {
	int n = serial.available();
	if (n > 0) {
		message.resize(n);
		int r = serial.readBytes(message.data(), message.size());
		if (r == n) {
			return r;
		}
	}
	return 0;
}

bool ofxDmxUsbPro::waitForReply(uint8_t label, uint64_t timeOutMicros) {
	uint64_t now = ofGetElapsedTimeMicros();
	uint64_t time = 0;
	int n = receiveMessage();
	while (n == 0 && time < timeOutMicros) {
		ofSleepMillis(1);
		n = receiveMessage();
		if (n > 0 && message[1] != label)
			n = 0;
		time = ofGetElapsedTimeMicros() - now;
	}
	return n > 0;
}
