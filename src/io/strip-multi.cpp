#include "esp_attr.h"

#include "strip-multi.h"

namespace tol {

// bool Strip::flush(int i) {
//   auto& out = output[i];
//   if(out->CanShow()) { // will def lead to skipped frames. Should publish something when ready hmm
//     if(out->Pixels() != *buffer(i)) {
//       // someone repointed our buffah!! booo
//       // memcpy(out->Pixels(), *buffer(i), out->PixelsSize()); // tho ideally I mean they're just pointing to the same spot nahmean
//     }
    
//     out->Dirty();
    
//     portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED; // well unlocked sounds bad is there another?
//     // portMUX_TRY_LOCK
//     // taskENTER_CRITICAL(&mux); // doesnt seem to help and somehow slows down other stuff??
//     portENTER_CRITICAL_SAFE(&mux); // seems to work
//     out->Show();
//     // taskEXIT_CRITICAL(&mux);
//     portEXIT_CRITICAL_SAFE(&mux);
//     return true;
//   }
//   flushAttempts++;
//   return false;
// }

}
