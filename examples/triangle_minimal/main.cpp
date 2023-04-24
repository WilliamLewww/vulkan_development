#include <string>
#include <android/log.h>

void android_main(struct android_app* state)
{
    __android_log_print(ANDROID_LOG_ERROR, "[WLEW]", "Hello World!");
}
