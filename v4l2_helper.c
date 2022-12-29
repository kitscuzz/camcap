#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
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
 * get_device_capabilities - query the driver for its capabilities
 */
int get_device_capabilities(int fd, struct v4l2_capability *caps) {
    if (fd < 0 || caps == NULL) {
        errno = EINVAL;
        return -1;
    }

    return xioctl(fd, VIDIOC_QUERYCAP, caps);
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
int enum_pixel_formats(int fd, enum v4l2_buf_type type, struct v4l2_fmtdesc **format) {
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

/**
 * pixel_format_valid - returns a non-zero value if the pixel format is valid
 */
int pixel_format_valid(int fd, enum v4l2_buf_type type, uint32_t pixel_format) {
    if (fd < 0) {
        errno = EINVAL;
        return -1;
    }

    struct v4l2_fmtdesc fmt = {0};

    for(int i = 0; -1 != get_nth_format(fd, type, i, &fmt); i++) {
        if (fmt.pixelformat == pixel_format) {
            return 1;
        }
    }

    return 0;
}

static int get_nth_frame_size(int fd, int pixel_format, int n, struct v4l2_frmsizeenum *frm_sz_enum) {
    frm_sz_enum->index = n;
    frm_sz_enum->pixel_format = pixel_format;
    return xioctl(fd, VIDIOC_ENUM_FRAMESIZES, frm_sz_enum);
}

static int get_framesize_count(int fd, int pixel_format) {
    struct v4l2_frmsizeenum fsze = {0};
    int frm_sz_cnt = 0;
    while (get_nth_frame_size(fd, pixel_format, frm_sz_cnt, &fsze) >= 0) {
        frm_sz_cnt++;
    }

    if (errno != EINVAL) {
        return -1;
    }

    return frm_sz_cnt;
}

/**
 * enum_frame_sizes - allocate space for a 
 */
int enum_frame_size(int fd, int pixel_format, struct v4l2_frmsizeenum **frm_sz_enum) {
    if (fd < 0 || frm_sz_enum == NULL) {
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
        if (get_nth_frame_size(fd, pixel_format, i, &fsze[i]) < 0) {
            free(fsze);
            fsze = NULL;
            return -1;
        }
    }

    *frm_sz_enum = fsze;

    return frm_sz_cnt;
}

/**
 * frame_size_valid - returns a non-zero value if the frame size is valid
 */
int frame_size_valid(int fd, uint32_t pixel_format, uint32_t width, uint32_t height) {
    if (fd < 0) {
        errno = EINVAL;
        return -1;
    }

    // Check to see if the frame size is valid for the given pixel format
    struct v4l2_frmsizeenum fsze = {0};

    uint32_t cur_width, cur_height;
    for (int i = 0; -1 != get_nth_frame_size(fd, pixel_format, i, &fsze) ; i++) {
        switch(fsze.type) {
            case V4L2_FRMSIZE_TYPE_DISCRETE:
                if (width == fsze.discrete.width && height == fsze.discrete.height) {
                    return 1;
                }
                break;

            case V4L2_FRMSIZE_TYPE_CONTINUOUS:
                if ((fsze.stepwise.min_width <= width) && (width <= fsze.stepwise.max_width)
                        && (fsze.stepwise.min_height <= height) && (height <= fsze.stepwise.max_height)) {
                    return 1;
                }
                break;

            case V4L2_FRMSIZE_TYPE_STEPWISE:
                cur_width = fsze.stepwise.min_width;
                cur_height = fsze.stepwise.min_height;
                while ((cur_width <= fsze.stepwise.max_width) && (cur_height <= fsze.stepwise.max_height)) {
                    if (cur_width == width && cur_height == height) {
                        return 1;
                    }

                    cur_width += fsze.stepwise.step_width;
                    cur_height += fsze.stepwise.step_height;
                }
                break;

            default:
                // This is an unknown type of frame specification. Abort!
                return 0;
        }
    }

    return 0;
}

/**
 * set_stream_format - apply a given format to a given device
 */
int set_stream_format(int fd, struct v4l2_format *fmt) {
    if (fd < 0 || fmt == NULL) {
        errno = EINVAL;
        return -1;
    }

    return xioctl(fd, VIDIOC_S_FMT, fmt);
}

/**
 * init_mmap_buffers - get a set of mmaped buffers related between the driver and the program
 */
int init_mmap_buffers(int fd, struct mmaped_buffer *bufs, int count) {
    if (fd < 0 || bufs == NULL || count < 0) {
        errno = EINVAL;
        return -1;
    }

    struct v4l2_requestbuffers req = {0};
    req.count = count;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
        return -1;
    }

    struct v4l2_buffer buf;
    for (int i = 0; i < count; i++) {
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        if (-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf)) {
            return -1;
        }

        bufs[i].length = buf.length;
        bufs[i].start = mmap(NULL, buf.length, (PROT_READ | PROT_WRITE), MAP_SHARED, fd, buf.m.offset);
        if (MAP_FAILED == bufs[i].start) {
            return -1;
        }
    }

    return 0;
}

/**
 * start_mmap_streaming - queue all buffers and tell the driver to start
 */
int start_mmap_streaming(int fd, int buf_count) {
    if (fd < 0 || buf_count < 0) {
        errno = EINVAL;
        return -1;
    }

    struct v4l2_buffer buf;

    for (int i = 0; i < buf_count; i++) {
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (-1 == xioctl(fd, VIDIOC_QBUF, &buf)) {
            return -1;
        }
    }

    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    return xioctl(fd, VIDIOC_STREAMON, &type);
}

/**
 * stop_streaming - tell the driver to stop streaming
 */
int stop_streaming(int fd) {
    if (fd < 0) {
        errno = EINVAL;
        return -1;
    }

    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    return xioctl(fd, VIDIOC_STREAMOFF, &type);
}

/**
 * read_frame - after sucessfully returning from a select, get the data from the buffer
 */
int read_frame(int fd, struct v4l2_buffer *buf) {
    if (fd < 0 || buf == NULL) {
        errno = EINVAL;
        return -1;
    }

    memset (buf, 0, sizeof(struct v4l2_buffer));
    buf->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf->memory = V4L2_MEMORY_MMAP;

    return xioctl(fd, VIDIOC_DQBUF, buf);
}

/**
 * enqueue_frame - tell the driver the buffer is ready to accept another frame
 */
int enqueue_frame(int fd, struct v4l2_buffer *buf) {
    if (fd < 0 || buf == NULL) {
        errno = EINVAL;
        return -1;
    }

    return xioctl(fd, VIDIOC_QBUF, buf);
}

