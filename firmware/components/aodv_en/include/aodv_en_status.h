#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum
    {
        AODV_EN_OK = 0,
        AODV_EN_NOOP = 1,
        AODV_EN_ERR_ARG = -1,
        AODV_EN_ERR_FULL = -2,
        AODV_EN_ERR_NOT_FOUND = -3,
        AODV_EN_ERR_EXISTS = -4,
        AODV_EN_ERR_NO_ROUTE = -5,
        AODV_EN_ERR_SIZE = -6,
        AODV_EN_ERR_PARSE = -7,
        AODV_EN_ERR_STATE = -8,
    } aodv_en_status_t;

#ifdef __cplusplus
}
#endif
