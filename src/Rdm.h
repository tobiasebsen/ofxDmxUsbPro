#pragma once

#include <inttypes.h>
#include <iostream>
#include <vector>

using namespace std;

#define SC_RDM						0xCC
#define SC_SUB_MESSAGE				0x01

#define DISCOVERY_COMMAND			0x10
#define DISCOVERY_COMMAND_RESPONSE	0x11
#define GET_COMMAND					0x20
#define GET_COMMAND_RESPONSE		0x21
#define SET_COMMAND					0x30
#define SET_COMMAND_RESPONSE		0x31

#define RESPONSE_TYPE_ACK			0x00
#define RESPONSE_TYPE_ACK_TIMER		0x01
#define RESPONSE_TYPE_NACK_REASON	0x02
#define RESPONSE_TYPE_ACK_OVERFLOW	0x03

#define DISC_UNIQUE_BRANCH			0x0001
#define DISC_MUTE					0x0002
#define DISC_UN_MUTE				0x0003
#define QUEUED_MESSAGE				0x0020
#define STATUS_MESSAGES				0x0030
#define SUPPORTED_PARAMETERS		0x0050
#define PARAMETER_DESCRIPTION		0x0051

#define STATUS_NONE					0x00
#define STATUS_GET_LAST_MESSAGE		0x01
#define STATUS_ADVISORY				0x02
#define STATUS_WARNING				0x03
#define STATUS_ERROR				0x04

#pragma pack(1)
typedef struct {
	union {
		uint8_t		uid[6];
		struct {
			uint16_t manufacturerId;
			uint32_t deviceId;
		};
	};
} RdmUid;

typedef struct {
	uint8_t		startCode;
	uint8_t		subStartCode;
	uint8_t		length;
	RdmUid		destUid;
	RdmUid		srcUid;
	uint8_t		tn;
	uint8_t		responseType;
	uint8_t		messageCount;
	uint16_t	subDevice;
	uint8_t		cc;
	uint16_t	pid;
	uint8_t		pdl;
} RdmHeader;
#pragma pack()

class RdmMessage {
public:
	RdmMessage();
	RdmMessage(uint8_t * msg, size_t size);
	RdmMessage(std::vector<uint8_t> & msg);
	RdmMessage(RdmUid uid, uint8_t cc, uint16_t pid);

	void setDestination(RdmUid & uid);
	void setSource(RdmUid & uid);
	void setResponseType(uint8_t rtype);
	void setPortID(uint8_t portid);
	void setCommandClass(uint8_t cc);
	void setParameterID(uint16_t pid);
	void setDataLength(uint8_t length);
	void setData(void * data);
	void setData(void * data, uint8_t length);
	void copyData(void * data, uint8_t length, uint8_t offset = 0);

	RdmUid getSource();
	uint8_t getCommandClass();
	uint16_t getParameterID();
	uint8_t getDataLength();
	void *	getData();
	string  getDataAsString();

	uint16_t calcChecksum();
	uint16_t getChecksum();
	bool validateChecksum();
	void updateChecksum();

	uint8_t * getPacket();
	size_t getPacketSize();
	void setPacket(uint8_t * packet, size_t size);

protected:
	RdmHeader * getHeader();
	std::vector<unsigned char> message;
};

RdmUid RdmDecodeUid(uint8_t * euid);
std::string RdmUidToString(RdmUid & uid);

void RdmDiscovery(RdmMessage & msg);
RdmUid RdmAllDevicesUid();
RdmUid RdmZeroUid();