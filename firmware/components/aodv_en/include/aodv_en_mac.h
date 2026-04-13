#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "aodv_en_limits.h"

#ifdef __cplusplus
extern "C"
{
#endif

    bool aodv_en_mac_equal(const uint8_t lhs[AODV_EN_MAC_ADDR_LEN],
                           const uint8_t rhs[AODV_EN_MAC_ADDR_LEN]);
    bool aodv_en_mac_is_zero(const uint8_t mac[AODV_EN_MAC_ADDR_LEN]);
    void aodv_en_mac_copy(uint8_t dst[AODV_EN_MAC_ADDR_LEN],
                          const uint8_t src[AODV_EN_MAC_ADDR_LEN]);
    void aodv_en_mac_clear(uint8_t mac[AODV_EN_MAC_ADDR_LEN]);

#ifdef __cplusplus
}
#endif
