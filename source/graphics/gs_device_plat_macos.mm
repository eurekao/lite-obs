#include "lite-obs/graphics/gs_device.h"
#include "lite-obs/util/log.h"

#if TARGET_PLATFORM == PLATFORM_MAC

#include <AppKit/AppKit.h>

struct gl_platform
{
    NSOpenGLContext *ctx{nullptr};
    bool release_on_destroy = true;
    ~gl_platform() {
        if (release_on_destroy) {

        }
    }
};

void *gs_device::gs_create_platform_rc()
{
    return nullptr;
}

void gs_device::gs_destroy_platform_rc(void *plat)
{
    (void)plat;
}

void *gs_device::gl_platform_create(void *)
{
    NSOpenGLContext *current_ctx = [NSOpenGLContext currentContext];

    NSOpenGLPixelFormatAttribute pixelFormatAttributes[] = {
      NSOpenGLPFANoRecovery,    NSOpenGLPFAAccelerated,
      NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion3_2Core,
      NSOpenGLPFAColorSize,     (NSOpenGLPixelFormatAttribute)32,
      NSOpenGLPFAAlphaSize,     (NSOpenGLPixelFormatAttribute)8,
      NSOpenGLPFADepthSize,     (NSOpenGLPixelFormatAttribute)24,
      NSOpenGLPFADoubleBuffer,  (NSOpenGLPixelFormatAttribute)0};

    NSOpenGLPixelFormat *pf = [[NSOpenGLPixelFormat alloc] initWithAttributes:pixelFormatAttributes];
    auto ctx = [[NSOpenGLContext alloc] initWithFormat:pf shareContext:current_ctx];

    [ctx makeCurrentContext];
    [ctx clearDrawable];

    gl_platform *plat = new gl_platform;
    plat->ctx = ctx;
    return plat;
}

gl_context_helper::gl_context_helper()
{
    platform = std::make_shared<gl_platform>();
    platform->release_on_destroy = false;
    platform->ctx = [NSOpenGLContext currentContext];
}

gl_context_helper::~gl_context_helper()
{
    if (platform->ctx) {
        [platform->ctx makeCurrentContext];
    }
}

void gs_device::device_enter_context_internal(void *param)
{
    gl_platform *plat = (gl_platform *)param;
    [plat->ctx makeCurrentContext];
}

void gs_device::device_leave_context_internal(void *param)
{
    (void)param;
    [NSOpenGLContext clearCurrentContext];
}

void gs_device::gl_platform_destroy(void *plat)
{
    gl_platform *p = (gl_platform *)plat;
    delete p;
}

#endif