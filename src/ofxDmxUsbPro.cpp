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
#define LABEL_SET_API_KEY			13
#define LABEL_HARDWARE_VERSION		14


ofxDmxUsbPro::ofxDmxUsbPro() {
	memset(&widgetParameters, 0, sizeof(widgetParameters));
	serialNumber = 0;
	hardwareVersion = 0;
	rdmTransactionNumber = 0;
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
					if (rdm.validateChecksum())
						ofNotifyEvent(rdmReceived, rdm, this);
				}
				if (startCode == 0xFE || startCode == 0xAA) { // RDM DISC_UNIQUE_BRANCH response
					RdmUid uid;
					if (RdmDecodeUid(data + 1, uid))
						ofNotifyEvent(rdmDiscovered, uid, this);
				}
			}
			if (label == LABEL_DMX_CHANGED) {
				cosData.resize(512);
				uint8_t start_changed_byte_number = data[0];
				uint8_t * changed_bit_array = data + 1;
				uint8_t * changed_dmx_data_array = data + 6;
				uint8_t changed_byte_index = 0;
				for (uint8_t byte_index=0; byte_index<5; byte_index++) {
					for (uint8_t bit_index=0; bit_index<8; bit_index++) {
						if ((changed_bit_array[byte_index] >> bit_index) & 0x1) {
							uint16_t i = start_changed_byte_number * 8 + byte_index * 8 + bit_index;
							if (i < cosData.size())
								cosData[i] = changed_dmx_data_array[changed_byte_index];
							changed_byte_index ++;
						}
					}
				}
				DmxData dmx;
				dmx.data = cosData.data();
				dmx.size = cosData.size();
				ofNotifyEvent(dmxReceived, dmx, this);
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
	rdm.setTransactionNumber(rdmTransactionNumber++);
	rdm.updateChecksum();
	sendRdm(rdm.getPacket(), rdm.getPacketSize());
}

void ofxDmxUsbPro::setReceiveDmxOnChange(bool dmxChangeOnly) {
	prepareMessage(LABEL_SET_DMX_CHANGE, 1);
	uint8_t * data = getData();
	data[0] = dmxChangeOnly ? 1 : 0;
	sendMessage();
}

void ofxDmxUsbPro::sendRdmDiscovery(const RdmUid & from, const RdmUid & to) {
	RdmMessage msg;
	RdmDiscovery(msg, from, to);
	msg.setSource(getUid());
	msg.setTransactionNumber(rdmTransactionNumber++);
	msg.updateChecksum();
	prepareMessage(LABEL_SEND_RDM_DISC, msg.getPacketSize());
	memcpy(getData(), msg.getPacket(), msg.getPacketSize());
	sendMessage();
}

bool ofxDmxUsbPro::getRdm(RdmMessage & send, RdmMessage & reply) {
	sendRdm(send);
	ofSleepMillis(1);
	if (waitForReply(LABEL_PACKET_RECEIVED)) {
		uint8_t * data = getData();
		uint8_t status = data[0];
		uint8_t startCode = data[1];
		if (status == 0 && startCode == SC_RDM) {
			reply.setPacket(data + 1, getLength() - 1);
			return reply.validateChecksum() && reply.getTransactionNumber() == send.getTransactionNumber() && reply.getParameterID() == send.getParameterID();
		}
	}
	else
		ofLogWarning() << "REPLY TIMED OUT";

	return false;
}

bool ofxDmxUsbPro::getRdm(RdmUid & uid, uint16_t pid, RdmMessage & reply) {
	RdmMessage send(uid, GET_COMMAND, pid);
	return getRdm(send, reply);
}

bool ofxDmxUsbPro::getRdmDiscovery(const RdmUid & from, const RdmUid & to, vector<RdmUid>& deviceUids) {
	sendRdmDiscovery(from, to);
	vector<RdmUid> uids;
	while (waitForReply(LABEL_PACKET_RECEIVED)) {
		uint8_t * data = getData();
		if (data[0] == 0) {
			RdmUid uid;
			if (RdmDecodeUid(data + 1, uid)) {
				uids.push_back(uid);
			}
		}
	}
	if (uids.size() == 1) {
		RdmMessage mute(uids[0], DISCOVERY_COMMAND, DISC_MUTE);
		RdmMessage reply;
		if (getRdm(mute, reply)) {
			uint16_t control = reply.getDataAsUint16();
		}
		deviceUids.push_back(reply.getSource());
		return true;
	}
	else if (uids.size() > 1) {
		uint64_t mid = (RdmUidToUint64(from) + RdmUidToUint64(to)) / 2;
		RdmUid midLow = RdmUidFromUint64(mid);
		RdmUid midHigh = RdmUidFromUint64(mid + 1);
		getRdmDiscovery(from, midLow, deviceUids);
		getRdmDiscovery(midHigh, to, deviceUids);
	}
	return false;
}

bool ofxDmxUsbPro::getRdmDiscoveryFull(vector<RdmUid>& deviceUids) {
	RdmMessage unmute(RdmAllDevicesUid(), DISCOVERY_COMMAND, DISC_UN_MUTE);
	sendRdm(unmute);
	ofSleepMillis(1);
	waitForReply(LABEL_PACKET_RECEIVED);
	return getRdmDiscovery(RdmZeroUid(), RdmAllDevicesUid(), deviceUids);
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
