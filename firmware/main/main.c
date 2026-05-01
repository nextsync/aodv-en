#include "sdkconfig.h"

#include "app_demo.h"
#include "app_proto_example.h"

void app_main(void)
{
#ifdef CONFIG_AODV_EN_APP_USE_APP_DEMO
    app_demo_run();
#else
    app_proto_example_run();
#endif
}
