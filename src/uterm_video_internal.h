/*
 * uterm - Linux User-Space Terminal
 *
 * Copyright (c) 2011-2012 David Herrmann <dh.herrmann@googlemail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/* Internal definitions */

#ifndef UTERM_VIDEO_INTERNAL_H
#define UTERM_VIDEO_INTERNAL_H

#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include "eloop.h"
#include "shl_hook.h"
#include "uterm_video.h"

/* backend-operations */

struct mode_ops {
	int (*init) (struct uterm_mode *mode);
	void (*destroy) (struct uterm_mode *mode);
	const char *(*get_name) (const struct uterm_mode *mode);
	unsigned int (*get_width) (const struct uterm_mode *mode);
	unsigned int (*get_height) (const struct uterm_mode *mode);
};

struct display_ops {
	int (*init) (struct uterm_display *display);
	void (*destroy) (struct uterm_display *display);
	int (*activate) (struct uterm_display *disp, struct uterm_mode *mode);
	void (*deactivate) (struct uterm_display *disp);
	int (*set_dpms) (struct uterm_display *disp, int state);
	int (*use) (struct uterm_display *disp);
	int (*swap) (struct uterm_display *disp);
	int (*blit) (struct uterm_display *disp,
		     const struct uterm_video_buffer *buf,
		     unsigned int x, unsigned int y);
	int (*fake_blendv) (struct uterm_display *disp,
			    const struct uterm_video_blend_req *req,
			    size_t num);
	int (*fill) (struct uterm_display *disp,
		     uint8_t r, uint8_t g, uint8_t b, unsigned int x,
		     unsigned int y, unsigned int width, unsigned int height);
};

struct video_ops {
	int (*init) (struct uterm_video *video, const char *node);
	void (*destroy) (struct uterm_video *video);
	void (*segfault) (struct uterm_video *video);
	int (*use) (struct uterm_video *video);
	int (*poll) (struct uterm_video *video);
	void (*sleep) (struct uterm_video *video);
	int (*wake_up) (struct uterm_video *video);
};

struct uterm_video_module {
	const struct video_ops *ops;
};

#define VIDEO_CALL(func, els, ...) (func ? func(__VA_ARGS__) : els)

/* drm */

#ifdef BUILD_ENABLE_VIDEO_DRM

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <gbm.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include "static_gl.h"

struct drm_video {
	int fd;
	struct ev_fd *efd;
	struct gbm_device *gbm;
	EGLDisplay disp;
	EGLConfig conf;
	EGLContext ctx;

	unsigned int sinit;
	bool supports_rowlen;
	GLuint tex;

	struct gl_shader *fill_shader;
	GLuint uni_fill_proj;

	struct gl_shader *blend_shader;
	GLuint uni_blend_proj;
	GLuint uni_blend_tex;
	GLuint uni_blend_fgcol;
	GLuint uni_blend_bgcol;

	struct gl_shader *blit_shader;
	GLuint uni_blit_proj;
	GLuint uni_blit_tex;
};

#else /* !BUILD_ENABLE_VIDEO_DRM */

struct drm_video {
	int unused;
};

#endif /* BUILD_ENABLE_VIDEO_DRM */

/* dumb drm */

#ifdef BUILD_ENABLE_VIDEO_DUMB

struct dumb_video {
	int fd;
	struct ev_fd *efd;
};

#else /* !BUILD_ENABLE_VIDEO_DUMB */

struct dumb_video {
	int unused;
};

#endif /* BUILD_ENABLE_VIDEO_DUMB */

/* uterm_mode */

struct uterm_mode {
	unsigned long ref;
	struct uterm_mode *next;

	const struct mode_ops *ops;
	void *data;
};

int mode_new(struct uterm_mode **out, const struct mode_ops *ops);

/* uterm_display */

#define DISPLAY_ONLINE		0x01
#define DISPLAY_VSYNC		0x02
#define DISPLAY_AVAILABLE	0x04
#define DISPLAY_OPEN		0x08
#define DISPLAY_DBUF		0x10
#define DISPLAY_DITHERING	0x20

struct uterm_display {
	unsigned long ref;
	unsigned int flags;
	struct uterm_display *next;
	struct uterm_video *video;

	struct shl_hook *hook;
	struct uterm_mode *modes;
	struct uterm_mode *default_mode;
	struct uterm_mode *current_mode;
	int dpms;

	bool vblank_scheduled;
	struct itimerspec vblank_spec;
	struct ev_timer *vblank_timer;

	const struct display_ops *ops;
	void *data;
};

int display_new(struct uterm_display **out, const struct display_ops *ops,
		struct uterm_video *video);
void display_set_vblank_timer(struct uterm_display *disp,
			      unsigned int msecs);
int display_schedule_vblank_timer(struct uterm_display *disp);

#define DISPLAY_CB(disp, act) shl_hook_call((disp)->hook, (disp), \
		&(struct uterm_display_event){ \
			.action = (act), \
		})

static inline bool display_is_conn(const struct uterm_display *disp)
{
	return disp->video;
}

static inline bool display_is_online(const struct uterm_display *disp)
{
	return display_is_conn(disp) && (disp->flags & DISPLAY_ONLINE);
}

/* uterm_video */

#define VIDEO_AWAKE		0x01
#define VIDEO_HOTPLUG		0x02

struct uterm_video {
	unsigned long ref;
	unsigned int flags;
	struct ev_eloop *eloop;

	struct uterm_display *displays;
	struct shl_hook *hook;

	const struct uterm_video_module *mod;
	const struct video_ops *ops;
	void *data;
	union {
		struct drm_video drm;
		struct dumb_video dumb;
	};
};

static inline bool video_is_awake(const struct uterm_video *video)
{
	return video->flags & VIDEO_AWAKE;
}

static inline bool video_need_hotplug(const struct uterm_video *video)
{
	return video->flags & VIDEO_HOTPLUG;
}

#define VIDEO_CB(vid, disp, act) shl_hook_call((vid)->hook, (vid), \
		&(struct uterm_video_hotplug){ \
			.display = (disp), \
			.action = (act), \
		})

static inline int video_do_use(struct uterm_video *video)
{
	return VIDEO_CALL(video->ops->use, -EOPNOTSUPP, video);
}

#if defined(BUILD_ENABLE_VIDEO_DRM) || defined(BUILD_ENABLE_VIDEO_DUMB)

#include <xf86drm.h>

static inline bool video_drm_available(void)
{
	return drmAvailable();
}

#else

static inline bool video_drm_available(void)
{
	return false;
}

#endif

#endif /* UTERM_VIDEO_INTERNAL_H */
