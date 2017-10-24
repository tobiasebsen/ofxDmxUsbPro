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

void RdmMessage::setDestination(RdmUid & uid) {
	RdmHeader * header = getHeader();
	memcpy(header->destUid.uid, uid.uid, sizeof(RdmUid));
}

void RdmMessage::setSource(RdmUid & uid) {
	RdmHeader * header = getHeader();
	memcpy(header->srcUid.uid, uid.uid, sizeof(RdmUid));
}

void RdmMessage::setTransactionNumber(uint8_t tn) {
	RdmHeader * header = getHeader();
	header->tn = tn;
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

void RdmMessage::copyDataFrom(const void * data, uint8_t length, uint8_t offset) {
	memcpy(message.data() + sizeof(RdmHeader) + offset, data, length);
}

RdmUid RdmMessage::getSource() {
	RdmHeader * header = getHeader();
	return header->srcUid;
}

uint8_t RdmMessage::getTransactionNumber() {
	RdmHeader * header = getHeader();
	return header->tn;
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

uint8_t * RdmMessage::getDataBytes() {
	return message.data() + sizeof(RdmHeader);
}

string RdmMessage::getDataAsString(uint8_t offset) {
	return string((char*)getData() + offset, getDataLength() - offset);
}

uint16_t RdmMessage::getDataAsUint16(uint8_t offset) {
	uint8_t * data = getDataBytes() + offset;
	return data[1] << 8 | data[0];
}

RdmUid RdmMessage::getDataAsUid(uint8_t offset) {
	RdmUid uid;
	memcpy(uid.uid, getDataBytes() + offset, sizeof(RdmUid));
	return uid;
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

bool RdmDecodeUid(uint8_t * euid, RdmUid & uid) {
	uint8_t j = 0;
	while (j < 8 && euid[j] == 0xFE)
		j++;
	if (euid[j] != 0xAA) {
		return false;
	}
	j++;
	uint16_t csEuid = 0;
	for (int i=0; i<6; i++) {
		uid.uid[i] = euid[j] & euid[j+1];
		csEuid += euid[j];
		csEuid += euid[j+1];
		j += 2;
	}
	uint16_t csValid = ((euid[j+0] & euid[j+1]) << 8) | (euid[j+2] & euid[j+3]);

	if (csEuid != csValid) {
		return false;
	}
	return true;
}

std::string RdmUidToString(const RdmUid & uid) {
	char sz[13];
	sprintf(sz, "%02X%02X%02X%02X%02X%02X", uid.uid[0], uid.uid[1], uid.uid[2], uid.uid[3], uid.uid[4], uid.uid[5]);
	return std::string(sz);
}

uint64_t RdmUidToUint64(const RdmUid & uid) {
	return uid.uid[0] << 40 | uid.uid[1] << 32 | uid.uid[2] << 24 | uid.uid[3] << 16 | uid.uid[4] << 8 | uid.uid[5];
}

RdmUid RdmUidFromUint64(uint64_t i) {
	RdmUid uid;
	uid.uid[0] = (i >> 40) & 0xFF;
	uid.uid[1] = (i >> 32) & 0xFF;
	uid.uid[2] = (i >> 24) & 0xFF;
	uid.uid[3] = (i >> 16) & 0xFF;
	uid.uid[4] = (i >> 8) & 0xFF;
	uid.uid[5] = (i >> 0) & 0xFF;
	return uid;
}

void RdmDiscovery(RdmMessage & msg, const RdmUid & lowerBound, const RdmUid & upperBound) {
	RdmUid dst = RdmAllDevicesUid();
	msg.setDestination(dst);
	msg.setSource(dst);
	msg.setPortID(1);
	msg.setCommandClass(DISCOVERY_COMMAND);
	msg.setParameterID(DISC_UNIQUE_BRANCH);
	msg.setDataLength(12);
	msg.copyDataFrom(lowerBound.uid, sizeof(RdmUid), 0);
	msg.copyDataFrom(upperBound.uid, sizeof(RdmUid), 6);
	msg.updateChecksum();
}

void RdmUnMute(RdmMessage & msg) {
	RdmUid dst = RdmAllDevicesUid();
	msg.setDestination(dst);
	msg.setSource(dst);
	msg.setPortID(1);
	msg.setCommandClass(DISCOVERY_COMMAND);
	msg.setParameterID(DISC_UN_MUTE);
	msg.setDataLength(0);
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