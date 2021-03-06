#include <getopt.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#if defined(__i386__)
#define IS_INTELCE
#elif defined(HAVE_REFSW_NEXUS_CONFIG_H)
#define IS_BCM_NEXUS
#elif defined(__arm__)
#define IS_RPI
#else
#error NO KNOWN TARGET!
#endif

#ifdef IS_INTELCE
#include <libgdl.h>
#endif

#ifdef IS_RPI
#include <bcm_host.h>
#endif

#ifdef IS_INTELCE
#include <libgdl.h>
#endif

#ifdef IS_BCM_NEXUS
#include <refsw/nexus_config.h>
#include <refsw/default_nexus.h>
//#define IS_BCM_NEXUS_CLIENT
#define PLATFORMSERVER_CLIENT
#if defined(PLATFORMSERVER_CLIENT) || defined(IS_BCM_NEXUS_CLIENT)
#ifdef IS_BCM_NEXUS_CLIENT
#include <refsw/nxclient.h>
#endif
#include <refsw/nexus_platform_client.h>
#else
#include <refsw/nexus_core_utils.h>
#include <refsw/nexus_display.h>
#include <refsw/nexus_platform.h>
#endif
#endif

#ifdef IS_RPI
extern DISPMANX_DISPLAY_HANDLE_T dispman_display;
#endif

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "gles2egl.h"
#include "gles2math.h"
#include "gles2text.h"
#include "gles2texture.h"

static int Stop = 0;

static void signal_handler(int sig_num) {
  if (sig_num == SIGINT || sig_num == SIGTERM || sig_num == SIGHUP) {
    Stop = 1;
  }
}

typedef unsigned long long _u64;

static _u64 timestamp_usec(void) {
  struct timeval mytime;
  gettimeofday(&mytime, 0);
  return ((_u64)((_u64)mytime.tv_usec + (_u64)mytime.tv_sec * (_u64)1000000));
}

#ifdef IS_INTELCE

// Plane size and position
#define ORIGIN_X 0
#define ORIGIN_Y 0
#define WIDTH 1280
#define HEIGHT 720
#define ASPECT ((GLfloat)WIDTH / (GLfloat)HEIGHT)

// Initializes a plane for the graphics to be rendered to
static gdl_ret_t setup_plane(gdl_plane_id_t plane) {
  gdl_pixel_format_t pixelFormat = GDL_PF_ARGB_32;
  gdl_color_space_t colorSpace = GDL_COLOR_SPACE_RGB;
  gdl_rectangle_t srcRect;
  gdl_rectangle_t dstRect;
  gdl_ret_t rc = GDL_SUCCESS;

  dstRect.origin.x = ORIGIN_X;
  dstRect.origin.y = ORIGIN_Y;
  dstRect.width = WIDTH;
  dstRect.height = HEIGHT;

  srcRect.origin.x = 0;
  srcRect.origin.y = 0;
  srcRect.width = WIDTH;
  srcRect.height = HEIGHT;

  rc = gdl_plane_reset(plane);
  if (GDL_SUCCESS == rc) {
    rc = gdl_plane_config_begin(plane);
  }

  if (GDL_SUCCESS == rc) {
    rc = gdl_plane_set_attr(GDL_PLANE_SRC_COLOR_SPACE, &colorSpace);
  }

  if (GDL_SUCCESS == rc) {
    rc = gdl_plane_set_attr(GDL_PLANE_PIXEL_FORMAT, &pixelFormat);
  }

  if (GDL_SUCCESS == rc) {
    rc = gdl_plane_set_attr(GDL_PLANE_DST_RECT, &dstRect);
  }

  if (GDL_SUCCESS == rc) {
    rc = gdl_plane_set_attr(GDL_PLANE_SRC_RECT, &srcRect);
  }

  if (GDL_SUCCESS == rc) {
    rc = gdl_plane_config_end(GDL_FALSE);
  } else {
    gdl_plane_config_end(GDL_TRUE);
  }

  if (GDL_SUCCESS != rc) {
    fprintf(stderr, "GDL configuration failed! GDL error code is 0x%x\n", rc);
  }

  return rc;
}
#endif

#ifdef IS_BCM_NEXUS

#if defined(PLATFORMSERVER_CLIENT) || defined(IS_BCM_NEXUS_CLIENT)

#ifdef __cplusplus
extern "C" {
#endif

static NEXUS_DisplayHandle gs_nexus_display = 0;
NEXUS_SurfaceClient *gs_native_window = 0;
// static void* gs_native_window = 0;
static NXPL_PlatformHandle nxpl_handle = 0;

#ifdef IS_BCM_NEXUS_CLIENT
static NxClient_AllocResults allocResults;
#endif

static unsigned int gs_screen_wdt = 1280;
static unsigned int gs_screen_hgt = 720;

bool InitPlatform(void) {
#ifdef IS_BCM_NEXUS_CLIENT
  NxClient_AllocSettings allocSettings;
  NxClient_JoinSettings joinSettings;

  NxClient_GetDefaultJoinSettings(&joinSettings);
  snprintf(joinSettings.name, NXCLIENT_MAX_NAME, "%s", "qtbrowser");
  fprintf(stderr, "NxClient_Join...\n");
  if (NxClient_Join(&joinSettings)) {
    fprintf(stderr, "Err: NxClient_Join() failed");
    return false;
  }

  NxClient_GetDefaultAllocSettings(&allocSettings);
  allocSettings.surfaceClient = 1;
  if (NxClient_Alloc(&allocSettings, &allocResults)) {
    fprintf(stderr, "Err: NxClient_Alloc() failed");
    return false;
  }
#else
  NEXUS_Platform_Join();
#endif

  NXPL_RegisterNexusDisplayPlatform(&nxpl_handle, gs_nexus_display);
  return true;
}

void DeInitPlatform(void) {
  NXPL_UnregisterNexusDisplayPlatform(nxpl_handle);
#ifdef IS_BCM_NEXUS_CLIENT
  NEXUS_SurfaceClient_Release(gs_native_window);
  NxClient_Free(&allocResults);
  NxClient_Uninit();
#endif
}
#ifdef __cplusplus
}
#endif

#else

#ifdef __cplusplus
extern "C" {
#endif

unsigned int gs_screen_wdt = 1280;
unsigned int gs_screen_hgt = 720;

static NEXUS_DisplayHandle gs_nexus_display = 0;
void *gs_native_window = 0;

static NXPL_PlatformHandle nxpl_handle = 0;

#if NEXUS_NUM_HDMI_OUTPUTS && !NEXUS_DTV_PLATFORM

static void hotplug_callback(void *pParam, int iParam) {
  NEXUS_HdmiOutputStatus status;
  NEXUS_HdmiOutputHandle hdmi = (NEXUS_HdmiOutputHandle)pParam;
  NEXUS_DisplayHandle display = (NEXUS_DisplayHandle)iParam;

  NEXUS_HdmiOutput_GetStatus(hdmi, &status);

  printf("HDMI hotplug event: %s\n",
         status.connected ? "connected" : "not connected");

  /* the app can choose to switch to the preferred format, but it's not
   * required. */
  if (status.connected) {
    NEXUS_DisplaySettings displaySettings;
    NEXUS_Display_GetSettings(display, &displaySettings);

    if (!status.videoFormatSupported[displaySettings.format]) {
      fprintf(stderr,
              "\nCurrent format not supported by attached "
              "monitor. Switching to preferred format %d\n",
              status.preferredVideoFormat);
      displaySettings.format = status.preferredVideoFormat;
      NEXUS_Display_SetSettings(display, &displaySettings);
    }
  }
}

#endif

void InitHDMIOutput(NEXUS_DisplayHandle display) {

#if NEXUS_NUM_HDMI_OUTPUTS && !NEXUS_DTV_PLATFORM

  NEXUS_HdmiOutputSettings hdmiSettings;
  NEXUS_PlatformConfiguration platform_config;

  NEXUS_Platform_GetConfiguration(&platform_config);

  if (platform_config.outputs.hdmi[0]) {
    NEXUS_Display_AddOutput(display, NEXUS_HdmiOutput_GetVideoConnector(
                                         platform_config.outputs.hdmi[0]));

    /* Install hotplug callback -- video only for now */
    NEXUS_HdmiOutput_GetSettings(platform_config.outputs.hdmi[0],
                                 &hdmiSettings);

    hdmiSettings.hotplugCallback.callback = hotplug_callback;
    hdmiSettings.hotplugCallback.context = platform_config.outputs.hdmi[0];
    hdmiSettings.hotplugCallback.param = (int)display;

    NEXUS_HdmiOutput_SetSettings(platform_config.outputs.hdmi[0],
                                 &hdmiSettings);

    /* Force a hotplug to switch to a supported format if necessary */
    hotplug_callback(platform_config.outputs.hdmi[0], (int)display);
  }

#else

  UNUSED(display);

#endif
}

bool InitPlatform(void) {
  bool succeeded = true;
  NEXUS_Error err;

  NEXUS_PlatformSettings platform_settings;

  /* Initialise the Nexus platform */
  NEXUS_Platform_GetDefaultSettings(&platform_settings);
  platform_settings.openFrontend = false;

  /* Initialise the Nexus platform */
  err = NEXUS_Platform_Init(&platform_settings);

  if (err) {
    printf("Err: NEXUS_Platform_Init() failed\n");
    succeeded = false;
  } else {
    NEXUS_DisplayHandle display = NULL;
    NEXUS_DisplaySettings display_settings;

    NEXUS_Display_GetDefaultSettings(&display_settings);

    display_settings.format = NEXUS_VideoFormat_e720p;

    display = NEXUS_Display_Open(0, &display_settings);

    if (display == NULL) {
      printf("Err: NEXUS_Display_Open() failed\n");
      succeeded = false;
    } else {
      NEXUS_VideoFormatInfo video_format_info;
      NEXUS_GraphicsSettings graphics_settings;
      NEXUS_Display_GetGraphicsSettings(display, &graphics_settings);

      graphics_settings.horizontalFilter = NEXUS_GraphicsFilterCoeffs_eBilinear;
      graphics_settings.verticalFilter = NEXUS_GraphicsFilterCoeffs_eBilinear;

      /* Disable blend with video plane */
      graphics_settings.sourceBlendFactor = NEXUS_CompositorBlendFactor_eOne;
      graphics_settings.destBlendFactor = NEXUS_CompositorBlendFactor_eZero;

      NEXUS_Display_SetGraphicsSettings(display, &graphics_settings);

      InitHDMIOutput(display);

      NEXUS_Display_GetSettings(display, &display_settings);
      NEXUS_VideoFormat_GetInfo(display_settings.format, &video_format_info);

      gs_nexus_display = display;
      gs_screen_wdt = video_format_info.width;
      gs_screen_hgt = video_format_info.height;

      printf("Screen width %d, Screen height %d\n", gs_screen_wdt,
             gs_screen_hgt);
    }
  }

  if (succeeded == true) {
    NXPL_RegisterNexusDisplayPlatform(&nxpl_handle, gs_nexus_display);
  }

  return succeeded;
}

void DeInitPlatform(void) {
  NXPL_DestroyNativeWindow(gs_native_window);

  if (gs_nexus_display != 0) {
    NXPL_UnregisterNexusDisplayPlatform(nxpl_handle);
    // NEXUS_SurfaceClient_Release ( gs_native_window );
  }
  NEXUS_Platform_Uninit();
}

#ifdef __cplusplus
}
#endif
#endif
#endif

typedef struct {
  int textureWidth;
  int textureHeight;
  int nTextures;
  int swapinterval;
  bool usebuffers;
  bool usesyncobjects;
  bool usesyncobjectswait;
} Config;

Config config;

typedef EGLSyncKHR(EGLAPIENTRYP PFNEGLCREATESYNCKHRPROC)(
    EGLDisplay dpy, EGLenum type, const EGLint *attrib_list);
typedef EGLBoolean(EGLAPIENTRYP PFNEGLDESTROYSYNCKHRPROC)(EGLDisplay dpy,
                                                          EGLSyncKHR sync);
typedef EGLint(EGLAPIENTRYP PFNEGLCLIENTWAITSYNCKHRPROC)(EGLDisplay dpy,
                                                         EGLSyncKHR sync,
                                                         EGLint flags,
                                                         EGLTimeKHR timeout);
typedef EGLBoolean(EGLAPIENTRYP PFNEGLGETSYNCATTRIBKHRPROC)(EGLDisplay dpy,
                                                            EGLSyncKHR sync,
                                                            EGLint attribute,
                                                            EGLint *value);

static PFNEGLCREATESYNCKHRPROC __eglCreateSyncKHR;
static PFNEGLDESTROYSYNCKHRPROC __eglDestroySyncKHR;
static PFNEGLCLIENTWAITSYNCKHRPROC __eglClientWaitSyncKHR;
static PFNEGLGETSYNCATTRIBKHRPROC __eglGetSyncAttribKHR;

int main(int argc, char *argv[]) {
  int i, x, y;

  config.textureWidth = 640;
  config.textureHeight = 480;
  config.nTextures = 100;
  config.usebuffers = false;
  config.swapinterval = -1;
  config.usesyncobjects = false;
  config.usesyncobjectswait = false;

  fprintf(stderr, "gles2app3 (%s)(%s)\n", __DATE__, __TIME__);

  int parse_err = 0;
  while (1) {
    static struct option long_options[] = {
        //{"verbose", no_argument,       &verbose_flag, 1},
        {"help", no_argument, 0, 0},     {"", required_argument, 0, 'n'},
        {"", required_argument, 0, 'w'}, {"", required_argument, 0, 'h'},
        {"", required_argument, 0, 'i'}, {"", no_argument, 0, 'b'},
        {"", no_argument, 0, 's'},       {0, 0, 0, 0}};
    /* getopt_long stores the option index here. */
    int option_index = 0;

    int c = getopt_long(argc, argv, "h:n:w:i:bst", long_options, &option_index);

    /* Detect the end of the options. */
    if (c == -1) break;

    switch (c) {
      case 0:
        printf("Usage: gles2app3 [OPTIONS] ...\n");
        printf("Options:\n");
        printf("--help    Display this help and exit\n");
        printf("-n        Number of textures\n");
        printf("-w        Texture width\n");
        printf("-h        Texture height\n");
        printf("-b        Use buffers\n");
        printf("-s        Use sync objects\n");
        printf("-t        Wait on sync objects\n");
        printf("-i        Swap interval\n");
        return 1;
        break;

      case 'n':
        config.nTextures = atoi(optarg);
        break;

      case 'w':
        config.textureWidth = atoi(optarg);
        break;

      case 'h':
        config.textureHeight = atoi(optarg);
        break;

      case 'i':
        config.swapinterval = atoi(optarg);
        break;

      case 'b':
        config.usebuffers = true;
        break;

      case 's':
        config.usesyncobjects = true;
        break;

      case 't':
        config.usesyncobjectswait = true;
        break;

      case '?':
        /* getopt_long already printed an error message. */
        parse_err = 1;
        break;

      default:
        abort();
    }
  }

  if (parse_err) {
    return 1;
  }

#ifdef IS_INTELCE
  gdl_plane_id_t plane = GDL_PLANE_ID_UPP_C;
#endif

  EGLDisplay display;
  EGLSurface surface;
  EGLContext context;

#ifdef IS_INTELCE
  gdl_init(0);

  setup_plane(plane);

  egl_init(&display, &surface, &context, 1280, 720, plane);
#else

#ifdef IS_BCM_NEXUS

  InitPlatform();

#endif

#ifdef IS_RPI

  bcm_host_init();

  dispman_display = vc_dispmanx_display_open(0 /* LCD */);

#endif

  egl_init(&display, &surface, &context, 1280, 720);
#endif

  fprintf(stderr, "EGL_CLIENT_APIS:\"%s\"\n",
          eglQueryString(display, EGL_CLIENT_APIS));
  fprintf(stderr, "EGL_VENDOR:\"%s\"\n", eglQueryString(display, EGL_VENDOR));
  fprintf(stderr, "EGL_VERSION:\"%s\"\n", eglQueryString(display, EGL_VERSION));
  fprintf(stderr, "EGL_EXTENSIONS:\"%s\"\n",
          eglQueryString(display, EGL_EXTENSIONS));

#if 1
  __eglCreateSyncKHR =
      (PFNEGLCREATESYNCKHRPROC)eglGetProcAddress("eglCreateSyncKHR");
  __eglDestroySyncKHR =
      (PFNEGLDESTROYSYNCKHRPROC)eglGetProcAddress("eglDestroySyncKHR");
  __eglClientWaitSyncKHR =
      (PFNEGLCLIENTWAITSYNCKHRPROC)eglGetProcAddress("eglClientWaitSyncKHR");
  __eglGetSyncAttribKHR =
      (PFNEGLGETSYNCATTRIBKHRPROC)eglGetProcAddress("eglGetSyncAttribKHR");
#endif

  signal(SIGINT, &signal_handler);
  signal(SIGTERM, &signal_handler);
  signal(SIGHUP, &signal_handler);

  // Texture textures[ W * H ];

  int rows = ceil(sqrt(config.nTextures));
  int columns = ceil(1.0 * config.nTextures / rows);

  Texture *textures = new Texture[rows * columns];

  for (y = 0; y < rows; y++) {
    for (x = 0; x < columns; x++) {
      textures[y * columns + x].Create(
          (1.0 * x) / columns, (1.0 * y) / rows, 1.0 / columns, 1.0 / rows,
          config.textureWidth, config.textureHeight, 0, config.usebuffers);

      fprintf(stdout, "Created %dx %dx%d checkerboard textures %s.\r",
              y * columns + x + 1, config.textureWidth, config.textureHeight,
              config.usebuffers ? "(using buffers)" : "(not using buffers)");
      fflush(stdout);
    }
  }

  fprintf(stdout, "\n");

  _u64 first_timestamp = timestamp_usec();

  _u64 prev_timestamp = 0;

  _u64 current;

  _u64 longstanding_longest = 0;
  _u64 longstanding_shortest = 0;

  _u64 longest = 0;
  _u64 shortest = 0;

  int count = 0;

  if (init_resources() == 0) {
    goto bail;
  }

  if (config.swapinterval != -1) {
    eglSwapInterval(display, config.swapinterval);
  } else {
    config.swapinterval = 1;
  }

  glClearColor(0.5, 0.0, 0.0, 1.0);

  fprintf(stdout, "Drawing %s %s (swap interval = %d)...\n",
          config.usebuffers ? "(using buffers)" : "(not using buffers)",
          config.usesyncobjects
              ? config.usesyncobjectswait ? "(using syncobjects wait)"
                                          : "(using syncobjects no wait)"
              : "(not using syncobjects)",
          config.swapinterval);

  static EGLSyncKHR ESK[4] = {0, 0, 0, 0};
  static int eglsyncid = 0;

  if (config.usesyncobjects) {
    ESK[0] = __eglCreateSyncKHR(display, EGL_SYNC_FENCE_KHR, 0);
    ESK[1] = __eglCreateSyncKHR(display, EGL_SYNC_FENCE_KHR, 0);
    ESK[2] = __eglCreateSyncKHR(display, EGL_SYNC_FENCE_KHR, 0);
    ESK[3] = __eglCreateSyncKHR(display, EGL_SYNC_FENCE_KHR, 0);
  }

  while (!Stop) {
    _u64 timestamp = timestamp_usec();

    if (config.usesyncobjects) {
      if (ESK[eglsyncid]) {
        __eglDestroySyncKHR(display, ESK[eglsyncid]);
        ESK[eglsyncid] = 0;
        glFlush();
      }
    }

    glClear(GL_COLOR_BUFFER_BIT);

    if (config.usesyncobjects && config.usesyncobjectswait) {
      /* wait on most recently created sync object */
      __eglClientWaitSyncKHR(display, ESK[(eglsyncid + 3) % 4],
                             0x01,        // EGL_SYNC_FLUSH_COMMANDS_BIT_KHR ,
                             1000000000); /* 1 second */
    }

    // draw odd textures
    for (i = 0; i < (rows * columns); i++) {
      if (i & 1) textures[i].Draw();
    }

    // draw even textures
    for (i = 0; i < (rows * columns); i++) {
      if (!(i & 1)) textures[i].Draw();
    }

    if (config.usesyncobjects) {
      if (!ESK[eglsyncid]) {
        ESK[eglsyncid] = __eglCreateSyncKHR(display, EGL_SYNC_FENCE_KHR, 0);
        glFlush();
        eglsyncid++;
        eglsyncid %= 4;
      }
    }

    if (prev_timestamp) {
      current = timestamp - prev_timestamp;

      if ((longstanding_shortest == 0) || (current < longstanding_shortest))
        longstanding_shortest = current;
      if ((longstanding_longest == 0) || (current > longstanding_longest))
        longstanding_longest = current;

      if ((shortest == 0) || (current < shortest)) shortest = current;
      if ((longest == 0) || (current > longest)) longest = current;

      render_text(0, 0, "%2.1f-%2.1f,%2.1f-%2.1f,%2.1f[%2.1f]",
                  1000000.0f / longstanding_longest,
                  1000000.0f / longstanding_shortest, 1000000.0f / longest,
                  1000000.0f / shortest, 1000000.0f / current,

                  count * 1000000.0f / (timestamp - first_timestamp)

                      );
    }
    prev_timestamp = timestamp;

    egl_swap(display, surface);

    count++;
    if ((count % 60) == 0) {
      longest = 0;
      shortest = 0;
    }
  }

bail:

  egl_exit(display, surface, context);

#ifdef IS_INTELCE
  gdl_close();
#endif

#ifdef IS_BCM_NEXUS

  DeInitPlatform();

#endif

#ifdef IS_RPI
// destroyDispmanxLayer(window);
#endif

  return 0;
}
