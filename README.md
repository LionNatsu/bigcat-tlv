# bigcat-tlv
A rather simple ser/des library, with a classic tag-length-value pattern design.

## History

It was once designed for a specific purpose.

I wrote the library for a very specialized communicating between a relatively high-performance host computer
and a ARM Cortex-M4 microcontroller.

The file `simple_tlv - コピー (2).cpp` was likely the oldest one which is last modified at `2019-05-26T01:38:07` (midnight!).
I think I later split it into two files, the library `bigcat_tlv.cpp` and the example `tlv_comm.cpp`.

It was a pleasure to write the same C++ code running both on embedded system and on my Linux computer.

## Design

It was designed for a lossy low-speed serial port connection scenario, so it has to be error-tolerant. It has...

- preamble bytes (or syncword, aka the magic sequence used to find the start of packet) before a packet;

- the tag, has one byte for each packet;

- the length is also using only one byte;

- the value part cannot contain more than `const int tlv_max_length = 255;` bytes;

- a checksum byte after a packet.

Therefore, the size of underlying buffer is fixed and the buffer does not use any dynamic memory allocation.

The deserialize process consumes one byte at once and return if a valid packet is fully received.

## Usage

### Serialize
```cpp
tlv_t msg;
uint8_t *cursor = data;

// Add a wheel encoder message.
uint32_t we = 1500;
msg = tlv_t(UPDATE_WHEEL_ENCODER, sizeof(we), &we);  // UPDATE_WHEEL_ENCODER, MCU_PANIC are constants.
cursor += msg.serialize(cursor);

// Add a panic message.
unsigned char panic_msg[] = "Shit happens. C'est la vie.";
msg = tlv_t(MCU_PANIC, sizeof(panic_msg), panic_msg);
cursor += msg.serialize(cursor);

// Send [data, cursor), which contains two serialized messages...
```

### Deserialize
```cpp
// Try to read them.
tlv_reader reader;
for(size_t i = 0; i < sizeof(data); i++) {
    bool completed = reader.update(data[i]);
    if(!completed) continue;
    
    tlv_t &tlv = reader.tlv;
    dispatch_message(tlv);  // Dispatch the packet by 'tlv.tag'
}
```
