#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <sys/ioctl.h>
#include <linux/videodev2.h>

#include "v4l2_helper.h"

static int xioctl(int fd, int request, void *arg) {
    int ret;
    do {
        ret = ioctl(fd, request, arg);
    } while(ret == -1 && errno == EINTR);
    return ret;
}

/**
 * print_pix_fmt - Prints a list of all known pixel formats, their fourcc value, and a description
 */
void print_pix_formats(void) {
#define V4L2T_PIX_FMT(d, sn, des) \
    fprintf(stdout, "%s (0x%08x) - \"%s\"\n", #sn, d, des);
#include "v4l2_tbl.h"
#undef V4L2T_PIX_FMT
}

/**
 * get_pix_fmt_str - Returns a string describing the fourcc pixel format
 *                   Returns "UNKNOWWN" for an invalid or unknown pixel format
 */
const char* pix_fmt_to_str(uint32_t fmt) {
#define V4L2T_PIX_FMT(d, sn, des) \
    if (fmt == d) { return #sn; }
#include "v4l2_tbl.h"
#undef V4L2T_PIX_FMT
    return "UNKNOWN";
}

/**
 * str_to_pix_fmt - Returns a pixel format based on the short name of a pixel format
 *                  Returns 0 if the name is not found
 */
uint32_t str_to_pix_fmt(const char *short_name) {
#define V4L2T_PIX_FMT(d, sn, des) \
    if (strcmp(#sn, short_name) == 0) { return (d); }
#include "v4l2_tbl.h"
#undef V4L2T_PIX_FMT
    return 0;
}

/**
 * print_capabilities - print out a list of all capabilities and description for a given caps
 */
void print_capabilities(uint32_t caps) {
#define V4L2T_CAP(d, sn, des) \
    if ((caps & d) != 0) { fprintf(stdout, "%s - %s\n", #sn, des); }
#include "v4l2_tbl.h"
#undef V4L2T_CAP
}

/**
 * get_nth_format - get format at index n, store it in "format"
 */
static int get_nth_format (int fd, enum v4l2_buf_type type, int n, struct v4l2_fmtdesc *format) {
    format->index = n;
    format->type = type;
    return xioctl(fd, VIDIOC_ENUM_FMT, format);
}

/**
 * count_formats - run through enumerating the formats, and return when the syscalls begin failing
 */
static int count_formats (int fd, enum v4l2_buf_type type) {
    struct v4l2_fmtdesc f = {0};
    int fmt_cnt = 0;
    while (get_nth_format(fd, type, fmt_cnt, &f) >= 0) {
        fmt_cnt++;
    }

    if (errno != EINVAL) {
        return -1;
    }

    return fmt_cnt;
}

/**
 * enum_formats - enumerate the formats available for a given device type
 *
 * This will allocate memory and save the address in "format"
 * @returns the number of elements in the formats array on sucess
 *          -1 on failure, with errno set appropriately
 */
int enum_formats(int fd, enum v4l2_buf_type type, struct v4l2_fmtdesc **format) {
    if (fd < 0 || format == NULL) {
        errno = EINVAL;
        return -1;
    }

    int fmt_cnt = count_formats(fd, type);
    if (fmt_cnt <= 0) {
        return fmt_cnt;
    }

    struct v4l2_fmtdesc *fmt = malloc(fmt_cnt * sizeof(struct v4l2_fmtdesc));
    if (fmt == NULL) {
        errno = ENOMEM;
        return -1;
    }

    for (int i = 0; i < fmt_cnt; i++) {
        if (get_nth_format(fd, type, i, &fmt[i]) == -1) {
            free(fmt);
            return -1;
        }
    }

    *format = fmt;

    return fmt_cnt;
}

static int get_nth_frame_sz(int fd, int pixel_format, int n, struct v4l2_frmsizeenum *frm_sz_enum) {
    frm_sz_enum->index = n;
    frm_sz_enum->pixel_format = pixel_format;
    return xioctl(fd, VIDIOC_ENUM_FRAMESIZES, frm_sz_enum);
}

static int get_framesize_count(int fd, int pixel_format) {
    struct v4l2_frmsizeenum fsze = {0};
    int frm_sz_cnt = 0;
    while (get_nth_frame_sz(fd, pixel_format, frm_sz_cnt, &fsze) >= 0) {
        frm_sz_cnt++;
    }

    if (errno != EINVAL) {
        return -1;
    }

    return frm_sz_cnt;
}

int enum_frame_size(int fd, int pixel_format, struct v4l2_frmsizeenum **frm_sz_enum) {
    if (frm_sz_enum == NULL) {
        errno = EINVAL;
        return -1;
    }

    int frm_sz_cnt = get_framesize_count(fd, pixel_format);
    if (frm_sz_cnt < 0) {
        return frm_sz_cnt;
    }

    struct v4l2_frmsizeenum *fsze = malloc(frm_sz_cnt * sizeof(struct v4l2_frmsizeenum));
    if (fsze == NULL) {
        errno = ENOMEM;
        return -1;
    }

    for (int i = 0; i < frm_sz_cnt; i++) {
        if (get_nth_frame_sz(fd, pixel_format, i, &fsze[i]) < 0) {
            free(fsze);
            return -1;
        }
    }

    *frm_sz_enum = fsze;

    return frm_sz_cnt;
}

