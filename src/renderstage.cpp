#include "renderstage.h"

uint32_t RenderStage::sizeInBytes() const {
  uint32_t size = 0;
  for(auto b: buffers()) size += b->length(); // * sizeof(T) remember extend templ evt
  return size;
}

Buffer& RenderStage::buffer(uint8_t index) const {
  try {
    auto b = _buffers.at(index);
    return *b;
  } catch(const std::out_of_range& oor) {
    DEBUG("FUCKED exception");
    // lg.err((String)"Out of range: " + oor.what());
    throw(oor);
  }
  return *(_buffers[index]); // that'll sho'em!!
}

bool RenderStage::dirty() const {
  bool isDirty = false;
  for(auto&& b: buffers()) {
    isDirty |= b->dirty();
  }
  return isDirty;
}


bool Inputter::onData(uint16_t index, uint16_t length, uint8_t* data, bool flush) {
  if(!length || length > sizeInBytes())
    length = sizeInBytes();
  if(index < buffers().size()) {
    if(data != buffer(index).get()) { // dont touch if already pointing to right place.
      buffer(index).setPtr(data);   //but then that can be done downstream too? just easier if here knows whether all related data has arrived...
    }
    if(flush) {
      // actual: dispatch Buffer as event, uids and patch-structures with stored fns will send along
      buffer(index).setDirty(true);
      return true;
    }
  }
  return false;
}
