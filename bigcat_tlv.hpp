#ifndef _BIGCAT_TLV_HPP_
#define _BIGCAT_TLV_HPP_

#include <cstdint>

namespace bigcat {
	const int tlv_max_length = 255;

	class tlv_t {
	public:
		tlv_t() = default;
		tlv_t(uint8_t t, uint8_t l, const void *v);

		size_t serialize(uint8_t *output);
		uint8_t checksum();

		uint8_t tag;
		uint8_t length;
		uint8_t value[tlv_max_length];
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
}

#endif // !_BIGCAT_TLV_HPP_
