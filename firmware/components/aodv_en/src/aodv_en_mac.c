#include "aodv_en_mac.h"

#include <string.h>

bool aodv_en_mac_equal(const uint8_t lhs[AODV_EN_MAC_ADDR_LEN],
                       const uint8_t rhs[AODV_EN_MAC_ADDR_LEN])
{
    if (lhs == NULL || rhs == NULL)
    {
        return false;
    }

    return memcmp(lhs, rhs, AODV_EN_MAC_ADDR_LEN) == 0;
}

bool aodv_en_mac_is_zero(const uint8_t mac[AODV_EN_MAC_ADDR_LEN])
{
    uint8_t zero_mac[AODV_EN_MAC_ADDR_LEN] = {0};

    if (mac == NULL)
    {
        return true;
    }

    return memcmp(mac, zero_mac, AODV_EN_MAC_ADDR_LEN) == 0;
}

bool aodv_en_mac_is_broadcast(const uint8_t mac[AODV_EN_MAC_ADDR_LEN])
{
    uint8_t broadcast_mac[AODV_EN_MAC_ADDR_LEN] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

    if (mac == NULL)
    {
        return false;
    }

    return memcmp(mac, broadcast_mac, AODV_EN_MAC_ADDR_LEN) == 0;
}

void aodv_en_mac_copy(uint8_t dst[AODV_EN_MAC_ADDR_LEN],
                      const uint8_t src[AODV_EN_MAC_ADDR_LEN])
{
    if (dst == NULL || src == NULL)
    {
        return;
    }

    memcpy(dst, src, AODV_EN_MAC_ADDR_LEN);
}

void aodv_en_mac_clear(uint8_t mac[AODV_EN_MAC_ADDR_LEN])
{
    if (mac == NULL)
    {
        return;
    }

    memset(mac, 0, AODV_EN_MAC_ADDR_LEN);
}
