#pragma once

#include "ofMain.h"
#include "Rdm.h"

class ofxDmxUsbPro {
public:
	ofxDmxUsbPro();

	bool setup(int deviceNumber = 0);
	bool setup(string portName);
	void update();

	// Non-blocking functions

	void requestWidgetParameters();
	void setWidgetParameters(uint8_t breakTime = 9, uint8_t mabTime = 1, uint8_t refreshRate = 40);
	void requestSerialNumber();
	void sendDmx(uint8_t * dmx, size_t length);
	void sendRdm(uint8_t * rdm, size_t length);
	void sendRdm(RdmMessage & rdm);
	void setReceiveDmxOnChange(bool dmxChangeOnly);
	void sendRdmDiscovery(const RdmUid & from, const RdmUid & to);


	// Blocking functions

	void getWidgetParameters();
	uint32_t getSerialNumber();
	string getSerialString();
	RdmUid getUid();
	bool getRdm(RdmMessage & send, RdmMessage & reply);
	bool getRdm(RdmUid & uid, uint16_t pid, RdmMessage & reply);
	bool getRdmDiscovery(const RdmUid & from, const RdmUid & to, vector<RdmUid> & deviceUids);
	bool getRdmDiscoveryFull(vector<RdmUid> & deviceUids);

	struct {
		unsigned char FirmwareLSB;
		unsigned char FirmwareMSB;
		unsigned char BreakTime;
		unsigned char MaBTime;
		unsigned char RefreshRate;
	} widgetParameters;
	uint32_t serialNumber;
	uint8_t hardwareVersion;

	typedef struct {
		uint8_t *	data;
		size_t		size;
	} DmxData;

	ofEvent<DmxData> dmxReceived;
	ofEvent<RdmMessage> rdmReceived;
	ofEvent<RdmUid> rdmDiscovered;

protected:

	void prepareMessage(uint8_t label, size_t length);
	uint8_t * getData();
	uint16_t getLength();
	void sendMessage();
	int receiveMessage();
	bool waitForReply(uint8_t label, uint64_t timeOutMicros = 1000000);

	ofSerial serial;
	vector<unsigned char> message;
	vector<unsigned char> cosData;
	uint8_t rdmTransactionNumber;
};