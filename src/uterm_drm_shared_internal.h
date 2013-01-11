/*
 * uterm - Linux User-Space Terminal
 *
 * Copyright (c) 2011-2013 David Herrmann <dh.herrmann@googlemail.com>
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

#ifndef UTERM_DRM_SHARED_INTERNAL_H
#define UTERM_DRM_SHARED_INTERNAL_H

#include <stdlib.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include "uterm_video.h"
#include "uterm_video_internal.h"

/* drm mode */

struct uterm_drm_mode {
	drmModeModeInfo info;
};

int uterm_drm_mode_init(struct uterm_mode *mode);
void uterm_drm_mode_destroy(struct uterm_mode *mode);
const char *uterm_drm_mode_get_name(const struct uterm_mode *mode);
unsigned int uterm_drm_mode_get_width(const struct uterm_mode *mode);
unsigned int uterm_drm_mode_get_height(const struct uterm_mode *mode);
void uterm_drm_mode_set(struct uterm_mode *mode, drmModeModeInfo *info);

static inline drmModeModeInfo *uterm_drm_mode_get_info(struct uterm_mode *m)
{
	struct uterm_drm_mode *mode = m->data;

	return &mode->info;
}

extern const struct mode_ops uterm_drm_mode_ops;

/* drm dpms */

int uterm_drm_set_dpms(int fd, uint32_t conn_id, int state);
int uterm_drm_get_dpms(int fd, drmModeConnector *conn);

/* drm display */

struct uterm_drm_display {
	uint32_t conn_id;
	int crtc_id;
	drmModeCrtc *saved_crtc;
	void *data;
};

int uterm_drm_display_init(struct uterm_display *disp, void *data);
void uterm_drm_display_destroy(struct uterm_display *disp);
int uterm_drm_display_activate(struct uterm_display *disp, int fd);
void uterm_drm_display_deactivate(struct uterm_display *disp, int fd);
int uterm_drm_display_bind(struct uterm_video *video,
			   struct uterm_display *disp, drmModeRes *res,
			   drmModeConnector *conn, int fd);
void uterm_drm_display_unbind(struct uterm_display *disp);

static inline void *uterm_drm_display_get_data(struct uterm_display *disp)
{
	struct uterm_drm_display *d = disp->data;

	return d->data;
}

/* drm video */

int uterm_drm_video_find_crtc(struct uterm_video *video, drmModeRes *res,
			      drmModeEncoder *enc);

#endif /* UTERM_DRM_SHARED_INTERNAL_H */
