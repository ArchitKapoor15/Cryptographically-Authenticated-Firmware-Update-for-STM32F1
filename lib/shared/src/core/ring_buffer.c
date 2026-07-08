#include "core/ring_buffer.h"

void ring_buffer_setup(ring_buffer_t* rb, uint8_t* buffer, uint32_t len){
    rb->buffer = buffer;
    rb->mask = len-1;
    rb->read = 0;
    rb->write = 0;
}

bool ring_buffer_empty(ring_buffer_t* rb){
    return rb->read == rb->write;
}

bool ring_buffer_read(ring_buffer_t* rb, uint8_t* val){
    uint32_t locread = rb->read;
    uint32_t locwrite = rb->write;
    if(locread == locwrite){
        return false;
    }
    *val = rb->buffer[locread];
    locread = (locread + 1) & rb->mask;
    rb->read = locread;
    return true;
}

bool ring_buffer_write(ring_buffer_t* rb, uint8_t val){
    uint32_t locread = rb->read;
    uint32_t locwrite = rb->write;
    uint32_t nxtwrite = (locwrite + 1) & rb->mask;
    if(nxtwrite == locread){
        return false;
    }
    rb->buffer[locwrite] = val;
    rb->write = nxtwrite;
    return true;
}
