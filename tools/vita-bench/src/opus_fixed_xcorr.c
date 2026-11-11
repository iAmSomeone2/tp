/* Fixed-point libopus with NEON presumed calls celt_pitch_xcorr_neon, which exists
 * only as ARM assembly (celt_pitch_xcorr_arm.s) that libopus's CMake build does not
 * assemble. The decoder calls it only for packet-loss concealment, which a clean
 * stream never reaches, so libopus's C version stands in.
 * ponytail: C fallback; assemble celt_pitch_xcorr_arm.s (via arm2gnu.pl) if the
 * port ships fixed point and needs fast PLC or encoding. */
#include <stdint.h>

int32_t celt_pitch_xcorr_c(const int16_t *x, const int16_t *y, int32_t *xcorr, int len, int max_pitch, int arch);

int32_t celt_pitch_xcorr_neon(const int16_t *x, const int16_t *y, int32_t *xcorr, int len, int max_pitch, int arch) {
    return celt_pitch_xcorr_c(x, y, xcorr, len, max_pitch, arch);
}
