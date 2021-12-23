#include "field.h"


template<>
void iField<uint8_t, int16_t>::blend(const iField<uint8_t, int16_t>& target, uint8_t progress) {
  uint16_t targetWeight = progress + 1,
            ourWeight = 257 - targetWeight;
  for(uint8_t p = 0; p < _size; p++) {
    data[p] = ((data[p] * ourWeight) + (target[p] * targetWeight)) >> 8; // works because rolls over I guess
  }
}

template class iField<uint8_t, int16_t>; //was this for againj
template class iField<uint16_t, int32_t>;
template class iField<int16_t, int32_t>;
template class iField<float>;
