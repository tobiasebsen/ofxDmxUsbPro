#include "Rdm.h"

RdmMessage::RdmMessage() {
}

RdmMessage::RdmMessage(uint8_t * msg, size_t size) {
	message.insert(message.end(), msg, msg + size);
}

RdmMessage::RdmMessage(std::vector<uint8_t> & msg) {
	message.insert(message.end(), msg.begin(), msg.end());
}

RdmMessage::RdmMessage(RdmUid uid, uint8_t cc, uint16_t pid) {
	setDestination(uid);
	setPortID(1);
	setCommandClass(cc);
	setParameterID(pid);
	updateChecksum();
}

void RdmMessage::setDestination(const RdmUid & uid) {
	RdmHeader * header = getHeader();
	memcpy(header->destUid.uid, uid.uid, sizeof(RdmUid));
}

void RdmMessage::setSource(const RdmUid & uid) {
	RdmHeader * header = getHeader();
	memcpy(header->srcUid.uid, uid.uid, sizeof(RdmUid));
}

void RdmMessage::setResponseType(uint8_t rtype) {
	RdmHeader * header = getHeader();
	header->responseType = rtype;
}

void RdmMessage::setPortID(uint8_t portid) {
	RdmHeader * header = getHeader();
	header->responseType = portid;
}

void RdmMessage::setCommandClass(uint8_t cc) {
	RdmHeader * header = getHeader();
	header->cc = cc;
}

void RdmMessage::setParameterID(uint16_t pid) {
	RdmHeader * header = getHeader();
	header->pid = ((pid >> 8) & 0xFF) | ((pid << 8) & 0xFF00);
}

void RdmMessage::setDataLength(uint8_t length) {
	message.resize(sizeof(RdmHeader) + length + 2);
	RdmHeader * header = (RdmHeader*)message.data();
	header->startCode = SC_RDM;
	header->subStartCode = SC_SUB_MESSAGE;
	header->length = sizeof(RdmHeader) + length;
	header->pdl = length;
}

void RdmMessage::setData(void * data) {
	if (message.size() < sizeof(RdmHeader))
		return;
	RdmHeader * header = (RdmHeader*)message.data();
	memcpy(message.data() + sizeof(RdmHeader), data, header->pdl);
}

void RdmMessage::setData(void * data, uint8_t length) {
	setDataLength(length);
	setData(data);
}

void RdmMessage::copyDataFrom(void * data, uint8_t length, uint8_t offset) {
	memcpy(message.data() + sizeof(RdmHeader) + offset, data, length);
}

RdmUid RdmMessage::getSource() {
	RdmHeader * header = getHeader();
	return header->srcUid;
}

uint8_t RdmMessage::getCommandClass() {
	RdmHeader * header = getHeader();
	return header->cc;
}

uint16_t RdmMessage::getParameterID() {
	RdmHeader * header = getHeader();
	return ((header->pid >> 8) & 0xFF) | ((header->pid << 8) & 0xFF00);
}

uint8_t RdmMessage::getDataLength() {
	RdmHeader * header = getHeader();
	return header->pdl;
}

void * RdmMessage::getData() {
	return message.data() + sizeof(RdmHeader);
}

string RdmMessage::getDataAsString() {
	return string((char*)getData(), getDataLength());
}

uint16_t RdmMessage::calcChecksum() {
	if (message.size() < sizeof(RdmHeader) + 2)
		message.resize(sizeof(RdmHeader) + 2);
	uint16_t cs = 0;
	int i=0;
	for (; i<message.size()-2; i++) {
		cs += message[i];
	}
	return cs;
}

uint16_t RdmMessage::getChecksum() {
	if (message.size() < sizeof(RdmHeader) + 2)
		message.resize(sizeof(RdmHeader) + 2);
	int i = message.size()-2;
	return message[i+1] | (message[i+0] << 8);
}

bool RdmMessage::validateChecksum() {
	return getChecksum() == calcChecksum();
}

void RdmMessage::updateChecksum() {
	if (message.size() < sizeof(RdmHeader) + 2)
		message.resize(sizeof(RdmHeader) + 2);

	uint16_t cs = calcChecksum();
	int i = message.size()-2;
	message[i+0] = (cs >> 8) & 0xFF;
	message[i+1] = cs & 0xFF;
}

unsigned char * RdmMessage::getPacket() {
	return message.data();
}

size_t RdmMessage::getPacketSize() {
	return message.size();
}

void RdmMessage::setPacket(uint8_t * packet, size_t size) {
	message.clear();
	message.insert(message.end(), packet, packet + size);
}

RdmHeader * RdmMessage::getHeader() {
	if (message.size() < sizeof(RdmHeader) + 2) {
		message.resize(sizeof(RdmHeader) + 2);
		RdmHeader * header = (RdmHeader*)message.data();
		header->startCode = SC_RDM;
		header->subStartCode = SC_SUB_MESSAGE;
		header->length = sizeof(RdmHeader);
		header->pdl = 0;
	}
	return (RdmHeader*)message.data();
}

RdmUid RdmDecodeUid(uint8_t * euid) {
	RdmUid uid;
	for (int i=0; i<6; i++) {
		uid.uid[i] = (euid[0] ^ 0xAA) | (euid[1] ^ 0x55);
		euid += 2;
	}
	return uid;
}

std::string RdmUidToString(RdmUid & uid) {
	char sz[13];
	sprintf(sz, "%02X%02X%02X%02X%02X%02X", uid.uid[0], uid.uid[1], uid.uid[2], uid.uid[3], uid.uid[4], uid.uid[5]);
	return std::string(sz);
}

void RdmDiscovery(RdmMessage & msg) {
	RdmUid dst = RdmAllDevicesUid();
	msg.setDestination(dst);
	msg.setSource(dst);
	msg.setPortID(1);
	msg.setCommandClass(DISCOVERY_COMMAND);
	msg.setParameterID(DISC_UNIQUE_BRANCH);
	msg.setDataLength(12);
	msg.copyDataFrom(RdmZeroUid().uid, sizeof(RdmUid), 0);
	msg.copyDataFrom(RdmAllDevicesUid().uid, sizeof(RdmUid), 6);
	msg.updateChecksum();
}

RdmUid RdmAllDevicesUid() {
	RdmUid uid;
	memset(uid.uid, 0xFF, sizeof(RdmUid));
	return uid;
}

RdmUid RdmZeroUid() {
	RdmUid uid;
	memset(uid.uid, 0x00, sizeof(RdmUid));
	return uid;
}