#include <cstdint>
#include <cstring>
#include "bigcat_tlv.hpp"

using namespace bigcat;

const unsigned char preamble[] = {0x55, 0xaa};

tlv_t::tlv_t(uint8_t t, uint8_t l, const void *v) {
	this->tag = t;
	this->length = l;
	memcpy(this->value, v, this->length);
}

size_t tlv_t::serialize(uint8_t *output) {
	uint8_t *ptr = output;
	auto chksum = this->checksum();
	auto append = [&](const void *addr, size_t len) {
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
	this->reset();
	return false;
}