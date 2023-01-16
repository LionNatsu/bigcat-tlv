#include <cstdint>
#include <cstring>
#include <cstdio>
#include "bigcat_tlv.hpp"

using namespace bigcat;

enum MSG_TAG {
	HEARTBEAT,
	UPDATE_WHEEL_ENCODER,
	MCU_PANIC,
	UPDATE_TWIST,
	STOP_IMMEDIATELY
};

int wheel_encoder_value = 0;

void dispatch_message(tlv_t tlv) {
	unsigned char reason[tlv_max_length + 1] = {};
	switch(tlv.tag) {
	case HEARTBEAT:
		break;
	case UPDATE_WHEEL_ENCODER:
		wheel_encoder_value = *(uint32_t *)tlv.value;
		printf("\t[Wheel Encoder] %d\n", wheel_encoder_value);
		break;
	case MCU_PANIC:
		memcpy(reason, tlv.value, tlv.length);
		printf("\t[Panic] %s\n", reason);
		break;
	default:
		printf("\t[Unknown] Tag: %d\n", tlv.tag);
	}
}

int main() {
	uint8_t data[50];
	uint8_t *stream = data;
	tlv_t msg;

	// Leave three random stack bytes.
	stream += 3;

	// Add a wheel encoder message.
	uint32_t we = 1500;
	msg = tlv_t(UPDATE_WHEEL_ENCODER, sizeof(we), &we);
	stream += msg.serialize(stream);

	// Add a panic message.
	unsigned char panic_msg[] = "Shit happens. C'est la vie.";
	msg = tlv_t(MCU_PANIC, sizeof(panic_msg), panic_msg);
	stream += msg.serialize(stream);

	// Try to read them.
	tlv_reader reader;
	for(size_t i = 0; i < sizeof(data); i++) {
		printf("0x%x ", data[i]);
		bool completed = reader.update(data[i]);
		if(!completed) continue;
		puts("");
		tlv_t &tlv = reader.tlv;
		dispatch_message(tlv);
	}
	puts("");
}