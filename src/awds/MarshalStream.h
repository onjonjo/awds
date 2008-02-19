#ifndef _MARSHALSTREAM_H__
#define _MARSHALSTREAM_H__

#include <awds/UnicastPacket.h>
#include <awds/Flood.h>
#include <awds/toArray.h>
#include <awds/NodeId.h>

#include <stdint.h>
#include <utility>
#include <cstring>

#include <cassert>

struct ReadMarshalStream {
    const char  * const start;
    size_t size;

    ReadMarshalStream(void * _start) :
	start(static_cast<char *>(_start)),
	size(0)
    { }

    explicit ReadMarshalStream(awds::Flood& flood) :
	start(flood.packet.buffer + awds::Flood::FloodHeaderEnd),
	size(0)
    {  }

    explicit ReadMarshalStream(awds::UnicastPacket& uni) :
	start(uni.packet.buffer + awds::UnicastPacket::UnicastPacketEnd),
	size(0)
    {  }

    inline ReadMarshalStream& operator >>(unsigned char& c) {
	c = static_cast<unsigned char>(start[size++]);
	return *this;
    }

    inline ReadMarshalStream& operator >>(char& c) {
	c = start[size++];
	return *this;
    }

    inline ReadMarshalStream& operator >>(uint32_t& v) {
	v = fromArray<uint32_t>(start + size);
	size += 4;
	return *this;
    }

    inline ReadMarshalStream& operator >>(uint16_t& v) {
	v = fromArray<uint16_t>(start + size);
	size += 2;
	return *this;
    }

    inline ReadMarshalStream& operator >>(awds::NodeId& id) {
	id.fromArray(start + size);
	size += awds::NodeId::size;
	return *this;
    }

    inline ReadMarshalStream& operator >>(gea::AbsTime& t) {
	t.fromArray( (void *)(start + size) );
	size += gea::AbsTime::size;
	return *this;
    }

    inline ReadMarshalStream& operator >>(gea::Duration& t) {
	t.fromArray( (void *)(start + size) );
	size += gea::Duration::size;
	return *this;
    }



    ReadMarshalStream& operator >> (std::pair<void *, unsigned> v) {
	memcpy( v.first, start+size, v.second);
	size += v.second;
	return *this;
    }


};

class WriteMarshalStream  {

    awds::BasePacket * const packet;
    const size_t startPacketOffset;

    char * const start;
    size_t size;

public:

    WriteMarshalStream(void * _start) :
	packet(0),
	startPacketOffset(0),
	start(static_cast<char *>(_start)),
	size(0)
    { }

    explicit WriteMarshalStream(awds::Flood flood) :
	packet(&flood.packet),
	startPacketOffset(awds::Flood::FloodHeaderEnd),
	start(flood.packet.buffer + startPacketOffset),
	size(0)
    {  }

    explicit WriteMarshalStream(awds::UnicastPacket uni) :
	packet(&uni.packet),
	startPacketOffset(awds::UnicastPacket::UnicastPacketEnd),
	start(uni.packet.buffer + startPacketOffset),
	size(0)
    {  }


    inline WriteMarshalStream& operator <<(unsigned char c) {
	start[size++] = static_cast<char>(c);
	return *this;
    }

    inline WriteMarshalStream& operator <<( uint32_t v ) {
	toArray<uint32_t>(v, start + size);
	size += 4;
	return *this;
    }

    inline WriteMarshalStream& operator <<( uint16_t v ) {
	toArray<uint16_t>(v, start + size);
	size += 2;
	return *this;
    }

    inline WriteMarshalStream& operator <<(const awds::NodeId& id) {
	id.toArray(start + size);
	size += awds::NodeId::size;
	return *this;
    }

    inline WriteMarshalStream& operator <<(const gea::AbsTime& t) {
	t.toArray( (void *)(start + size) );
	size += gea::AbsTime::size;
	return *this;
    }

    inline WriteMarshalStream& operator <<(const gea::Duration& t) {
	t.toArray( (void *)(start + size) );
	size += gea::Duration::size;
	return *this;
    }

    inline WriteMarshalStream& operator <<(std::pair<const void *, unsigned> v) {
	memcpy(start+size, v.first, v.second);
	size += v.second;
	return *this;
    }

    size_t getStreamSize() const { return this->size; }

    void storePacketSize() {
	assert(this->packet);
	this->packet->size = this->startPacketOffset + this->getStreamSize();
    }


};


#endif //MARSHALSTREAM_H__
/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
