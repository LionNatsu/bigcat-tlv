#include <cstdint>
#include <cstring>
#include <cstdio>

const unsigned char preamble[] = {0x55, 0xaa};

const int datagram_max_length = 255;

class tlv_t {
public:
	tlv_t() = default;
	tlv_t(uint8_t t, uint8_t l, const void *v);

	size_t serialize(uint8_t *output);
	uint8_t checksum();

	uint8_t tag;
	uint8_t length;
	uint8_t value[datagram_max_length];
};


class tlv_reader {
public:
	tlv_reader();

	[[nodiscard]]
	bool update(uint8_t);

	tlv_t tlv;

private:
	enum class waiting {
		PREAMBLE,
		TAG,
		LENGTH,
		VALUE,
		CHECKSUM
	};
	void reset();
	void update_checksum(uint8_t);
	waiting state;
	int preamble_counter;
	int value_counter;
};

tlv_t::tlv_t(uint8_t t, uint8_t l, const void *v) {
	this->tag = t;
	this->length = l;
	memcpy(this->value, v, this->length);
}

size_t tlv_t::serialize(uint8_t *output) {
	uint8_t *ptr = output;
	auto chksum = this->checksum();
	auto append = [&](const void *addr, size_t len){
		memcpy(ptr, addr, len);
		ptr += len;
	};
	append(preamble, sizeof(preamble));
	append(&this->tag, sizeof(this->tag));
	append(&this->length, sizeof(this->length));
	append(this->value, this->length);
	append(&chksum, sizeof(chksum));
	return ptr - output;
}


uint8_t tlv_t::checksum() {
	uint8_t sum = 0;
	for(size_t i = 0; i < this->length; i++) {
		sum += this->value[i];
	}
	return sum;
}

tlv_reader::tlv_reader() {this->reset();}

void tlv_reader::reset() {
	this->state = waiting::PREAMBLE;
	this->preamble_counter = 0;
}

bool tlv_reader::update(uint8_t b) {
	switch(this->state) {
	case waiting::PREAMBLE:
		if(b == preamble[this->preamble_counter])
			preamble_counter++;
		if(preamble_counter == sizeof(preamble))
			this->state = waiting::TAG;
		return false;
	case waiting::TAG:
		this->tlv.tag = b;
		this->state = waiting::LENGTH;
		return false;
	case waiting::LENGTH:
		this->tlv.length = b;
		if(this->tlv.length == 0) {
			this->state = waiting::CHECKSUM;
		} else {
			this->value_counter = 0;
			this->state = waiting::VALUE;
		}
		return false;
	case waiting::VALUE:
		this->tlv.value[this->value_counter] = b;
		this->value_counter++;
		if(this->value_counter == this->tlv.length) {
			this->state = waiting::CHECKSUM;
		}
		return false;
	case waiting::CHECKSUM:
		bool is_valid = b == this->tlv.checksum();
		this->reset();
		return is_valid;
	}
}

/////
// Example

enum MSG_TAG {
	HEARTBEAT,
	UPDATE_WHEEL_ENCODER,
	MCU_PANIC,
	UPDATE_SERVO,
	UPDATE_MOTOR,
	STOP_IMMEDIATELY
};

int wheel_encoder_value = 0;

// Body
void dispatch_message(tlv_t tlv);

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

void dispatch_message(tlv_t tlv) {
	unsigned char reason[datagram_max_length + 1] = {};
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
