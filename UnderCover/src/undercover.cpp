#include <cstdlib>
#include <unistd.h>
#include <string>
#include <vector>
#include <fcntl.h>
#include <android/log.h>
#include <sys/system_properties.h>
#include <dlfcn.h>
#include <cstring>
#include <sys/mman.h>
#include "zygisk.hpp"

using zygisk::Api;
using zygisk::AppSpecializeArgs;
using zygisk::ServerSpecializeArgs;

using property_get_t = int (*)(const char *, char *);
static property_get_t original_property_get = nullptr;

class undercover : public zygisk::ModuleBase
{
public:
    void onLoad(Api *api, JNIEnv *env) override
    {
        this->api = api;
        this->env = env;
    }

    void preAppSpecialize(AppSpecializeArgs *args) override
    {
        const char *process = env->GetStringUTFChars(args->nice_name, nullptr);
        preSpecialize(process);
        env->ReleaseStringUTFChars(args->nice_name, process);
    }

    void preServerSpecialize(ServerSpecializeArgs *args) override
    {
        preSpecialize("system_server");
    }

    static int property_get_hook(const char *name, char *value)
    {
        if (strcmp(name, "init.svc.adbd") == 0){
            strcpy(value, "stopped");
            return strlen(value);
        }
        if (strcmp(name, "sys.usb.state") == 0){
            strcpy(value, "mtp");
            return strlen(value);
        }
        return original_property_get(name, value);
    }

    void hookSystemProperty()
    {
        void *handle = dlopen("libc.so", RTLD_NOW);
        if (handle)
        {
            original_property_get = (property_get_t)dlsym(handle, "__system_property_get");
            if (original_property_get)
            {
                mprotect((void *)((uintptr_t)original_property_get & ~0xFFF), 0x1000, PROT_READ | PROT_WRITE | PROT_EXEC);
                *(void **)(&original_property_get) = (void *)property_get_hook;
            }
            dlclose(handle);
        }
    }

private:
    Api *api;
    JNIEnv *env;

    void preSpecialize(const char *process)
    {
        unsigned r = 0;
        int fd = api->connectCompanion();
        read(fd, &r, sizeof(r));
        close(fd);
        int need_p6 = 0;
        std::string package_name = process;
        //add conditions
        hookSystemProperty();
        api->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
    }
};

static int urandom = -1;

static void companion_handler(int i)
{
    if (urandom < 0)
    {
        urandom = open("/dev/urandom", O_RDONLY);
    }
    unsigned r;
    read(urandom, &r, sizeof(r));
    write(i, &r, sizeof(r));
}

REGISTER_ZYGISK_MODULE(undercover)
REGISTER_ZYGISK_COMPANION(companion_handler)
