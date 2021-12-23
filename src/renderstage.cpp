#include "renderstage.h"

namespace tol {

RenderStage::RenderStage(const String& id, uint8_t fieldSize, uint16_t fieldCount,
                         uint8_t bufferCount, uint16_t portIndex, uint16_t targetHz):
  ChunkedContainer(fieldSize, fieldCount),
  Runnable(id, "Renderstage", targetHz), _portId(portIndex) {

  for(auto i=0; i<bufferCount; i++)
    _buffers.emplace_back(std::make_unique<Buffer>(id, fieldSize, fieldCount));

  init();
  printTo(tol::lg);
}

size_t RenderStage::printTo(Print& p) const {
  size_t n = Runnable::printTo(p)
            + p.print((String)buffers().size() + " ports:\n");
  for(auto& b: buffers()) n += b->printTo(p);
  return n;
}

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
    lg.err((String)"Out of range: " + oor.what());
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

      if(index == 0 && controlDataLength != 0) { // temp til real Patch structure, 0 ctrls, 1 data...
        auto [controls, data] = buffer(index).slice(controlDataLength); // should return shader_ptrs tho
        ipc::Publisher<PatchControls>::publish(PatchControls(controls, index));
        ipc::Publisher<PatchIn>::publish(PatchIn(data, index));
      } else {
        ipc::Publisher<PatchIn>::publish(PatchIn(buffer(index), index));
      }
      return true;
    }
  }
  return false;
}

}
