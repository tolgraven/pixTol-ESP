#include "renderstage.h"

namespace tol {

RenderStage::RenderStage(const std::string& id, uint8_t fieldSize, uint16_t fieldCount,
                         uint8_t bufferCount, uint16_t portIndex, uint16_t targetHz):
  ChunkedContainer(fieldSize, fieldCount),
  Runnable(id, "Renderstage", targetHz), _portId(portIndex) {

  for(auto i=0; i<bufferCount; i++)
    _buffers.emplace_back(std::make_unique<Buffer>(id, fieldSize, fieldCount));

  init();
}

std::string RenderStage::toString() const {
  auto s = Runnable::toString() + std::to_string(buffers().size()) + " ports:\n";
  for(auto& b: buffers()) s += b->toString();
  return s;
}

uint32_t RenderStage::sizeInBytes() const {
  uint32_t size = 0;
  for(auto b: buffers()) size += b->lengthBytes(); // * sizeof(T) remember extend templ evt
  return size;
}

Buffer& RenderStage::buffer(uint8_t index) const {
  try {
    auto b = _buffers.at(index);
    return *b;
  } catch(const std::out_of_range& oor) {
    lg.err((std::string)"Out of range: " + oor.what());
    throw(oor);
  }
  return *(_buffers[index]); // that'll sho'em!!
}

bool RenderStage::dirty() const {
  bool isDirty = false;
  for(auto& b: buffers()) {
    isDirty |= b->dirty();
  }
  return isDirty;
}


bool Inputter::onData(uint16_t index, uint16_t length, uint8_t* data, bool flush) {
  bool isControls = index == 0 && controlsBuffer != nullptr;
  if(!isControls && controlsBuffer && index > 0) {
    index--; // since 0 taken up by controlsBuffer, tho it's not in RS bufs due to smaller size... 1 becomes 0 etc
  }
  if(index > buffers().size())
    return false; // and maybe log
  auto& buf = isControls? *controlsBuffer: buffer(index);
  if(!length || length > buf.lengthBytes()) // tho uhh length 0 means empty so prob log and bail?
    length = buf.lengthBytes(); // dont overflow on oversized input
  
  if(data != buf.get()) // dont touch if already pointing to right place.
    buf.setPtr(data);
  
  if(!flush)
    buf.setDirty(false); // since setPtr will dirty it...
  else {
    // actual: dispatch Buffer as event, uids and patch-structures with stored fns will send along
    buf.setDirty(true);

    if(isControls) { // temp til real Patch structure, 0 ctrls, 1 data...
      ipc::Publisher<PatchControls>::publish(PatchControls(buf, index));
    } else {
      ipc::Publisher<PatchIn>::publish(PatchIn(buf, index));
    }
    return true;
  }
  return false;
}

}
