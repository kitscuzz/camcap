#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>
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
 * pixel_format_valid - returns a non-zero value if the pixel format is valid
 */
static int pixel_format_valid(int fd, uint32_t pixel_format) {
    // Check to see if they've selected a valid pixel format
    struct v4l2_fmtdesc *format = NULL;

    int fmt_cnt = enum_formats(fd, V4L2_BUF_TYPE_VIDEO_CAPTURE, &format);
    if (fmt_cnt < -1) {
        fprintf(stderr, "Unable to enumerate video capture formats\n");
        return 0;
    }

    int valid = 0;
    for (int i = 0; !valid && (i < fmt_cnt); i++) {
        if (format[i].pixelformat == pixel_format) {
            valid = 1;
        }
    }

    if (!valid) {
        fprintf(stdout, "No/invalid pixel format specified! Please select from the following:\n");
        for (int i = 0; i < fmt_cnt; i++) {
            fprintf(stdout, "%s\n", pix_fmt_to_str(format[i].pixelformat));
        }
    }

    free(format);
    format = NULL;

    return valid;
}

static int frame_size_valid(int fd, uint32_t pixel_format, int width, int height) {
    // Check to see if the frame size is valid for the given pixel format
    struct v4l2_frmsizeenum *fsze = NULL;
    int frm_sz_cnt = enum_frame_size(fd, pixel_format, &fsze);
    if (frm_sz_cnt < 0) {
        perror("Error enumerating frame sizes for pixel format");
        return 0;
    }

    int valid = 0;
    for (int i = 0; !valid && (i < frm_sz_cnt); i++) {
        if (fsze[i].type == V4L2_FRMSIZE_TYPE_DISCRETE) {
            if (width == fsze[i].discrete.width && height == fsze[i].discrete.height) {
                valid = 1;
            }
        } else {
            int cur_width = fsze[i].stepwise.min_width;
            int cur_height = fsze[i].stepwise.min_height;
            while (!valid && (cur_width < fsze[i].stepwise.max_width) && (cur_height < fsze[i].stepwise.max_height)) {
                if (cur_width == width && cur_height == height) {
                    valid = 1;
                }

                cur_width += fsze[i].stepwise.step_width;
                cur_height += fsze[i].stepwise.step_height;
            }
        }
    }

    if (!valid) {
        fprintf(stdout, "Not all/invalid frame size dimensions specified! Please select from the following:\n");

        for (int i = 0; i < frm_sz_cnt; i++) {
            if (fsze[i].type == V4L2_FRMSIZE_TYPE_DISCRETE) {
                fprintf(stdout, "%dx%d\n", fsze[i].discrete.width, fsze[i].discrete.height);
            } else {
                fprintf(stdout, "From %dx%d to %dx%d by %dx%d\n",
                        fsze[i].stepwise.min_width, fsze[i].stepwise.min_height,
                        fsze[i].stepwise.max_width, fsze[i].stepwise.max_height,
                        fsze[i].stepwise.step_width, fsze[i].stepwise.step_height);
            }
        }
    }

    free(fsze);
    fsze = NULL;

    return valid;
}

int main(int argc, char *argv[]) {
    int fd = -1, ret = 0;
    static struct option long_options[] = {
        {"device", required_argument, 0, 'd' },
        {"format", required_argument, 0, 'f' },
        {"width",  required_argument, 0, 'w' },
        {"height", required_argument, 0, 'h' },
        {"output", required_argument, 0, 'o' },
        {0,        0,                 0,  0  }
    };
    static char options[] = "d:f:w:h:o:";

    const char *dev_name = NULL;
    const char *out_name = NULL;
    uint32_t pixel_format = 0;
    int width = 0, height = 0;
    int opt;
    while (-1 != (opt = getopt_long(argc, argv, options, long_options, NULL))) {
        switch (opt) {
            case 'd':
                if (dev_name == NULL) {
                    dev_name = optarg;
                } else {
                    fprintf(stderr, "ERROR: Can only accept 1 device name per instantiation! "
                            "\"%s\" is already set, cannot accept \"%s\"\n", dev_name, optarg);
                    return -1;
                }
                break;

            case 'f':
                if (pixel_format == 0) {
                    pixel_format = str_to_pix_fmt(optarg);
                    if (pixel_format == 0) {
                        fprintf(stderr, "ERROR: Unable to parse pixel format \"%s\"!", optarg);
                        return -1;
                    }
                } else {
                    fprintf(stderr, "ERROR: Can only accept 1 pixel format per instantiation! "
                            "\"%s\" is already set, cannot accept \"%s\"\n",
                            pix_fmt_to_str(pixel_format), optarg);
                    return -1;
                }
                break;

            case 'h':
                if (height == 0) {
                    char *endptr = NULL;
                    long parsed = strtol(optarg, &endptr, 0);
                    if (parsed > INT_MAX || parsed <= 0 || *endptr != '\0') {
                        fprintf(stderr, "ERROR: Unable to parse given height: \"%s\"\n", optarg);
                    }
                    height = (int) parsed;
                } else {
                    fprintf(stderr, "ERROR: Can only accept 1 height per instantiation."
                            "%d is already the current height, cannot accept \"%s\"\n", height, optarg);
                    return -1;
                }
                break;

            case 'o':
                if (out_name == NULL) {
                    out_name = optarg;
                } else {
                    fprintf(stderr, "ERROR: Can only accept 1 output file name per instantiation! "
                            "\"%s\" is already set, cannot accept \"%s\"\n", out_name, optarg);
                    return -1;
                }
                break;

            case 'w':
                if (width == 0) {
                    char *endptr = NULL;
                    long parsed = strtol(optarg, &endptr, 0);
                    if (parsed > INT_MAX || parsed <= 0 || *endptr != '\0') {
                        fprintf(stderr, "ERROR: Unable to parse given width: \"%s\"\n", optarg);
                    }
                    width = (int) parsed;
                } else {
                    fprintf(stderr, "ERROR: Can only accept 1 width per instantiation."
                            "%d is already the current width, cannot accept \"%s\"\n", width, optarg);
                    return -1;
                }
                break;

            case '?':
            default:
                return -1;
        }
    }

    if (dev_name == NULL) {
        fprintf(stderr, "Please provide a valid device name (like /dev/video0)!\n");
        return -1;
    }

    fd = open(dev_name, O_RDWR);
    if (fd == -1) {
        perror("Error opening video device");
        fprintf(stderr, "Unable to open %s\n", dev_name);
        ret = -1;
        goto fail;
    }

    FILE *out = NULL;
    if (out_name != NULL) {
        out = fopen(out_name, "w");
        if (out == NULL) {
            perror("Error opening output file");
            ret = -1;
            goto fail;
        }
    }

    struct v4l2_capability caps;
    if (xioctl(fd, VIDIOC_QUERYCAP, &caps) < 0) {
        perror("Error querying capabilities");
        ret = -1;
        goto fail;
    }

    char driver[sizeof(caps.driver) + 1] = {'\0'};
    memcpy(caps.driver, driver, sizeof(caps.driver));
    fprintf(stdout, "Driver= \"%s\"\n", driver);

    char card[sizeof(caps.card) + 1] = {'\0'};
    memcpy(caps.card, card, sizeof(caps.card));
    fprintf(stdout, "Card = \"%s\"\n", card);

    char bus_info[sizeof(caps.bus_info) + 1] = {'\0'};
    memcpy(caps.bus_info, bus_info, sizeof(caps.bus_info));
    fprintf(stdout, "Bus info = \"%s\"\n", bus_info);

    fprintf(stdout, "V4L2 driver version = %d\n", caps.version);
    fprintf(stdout, "Capabilities = 0x%08x\n", caps.capabilities);
    if (caps.capabilities & V4L2_CAP_DEVICE_CAPS) {
        fprintf(stdout, "Device caps = 0x%08x\n", caps.device_caps);
        print_capabilities(caps.device_caps);
    } else {
        print_capabilities(caps.capabilities);
    }

    if (!pixel_format_valid(fd, pixel_format)) {
        goto fail;
    }

    if (!frame_size_valid(fd, pixel_format, width, height)) {
        goto fail;
    }

    struct v4l2_format fmt = {0};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = width;
    fmt.fmt.pix.height = height;
    fmt.fmt.pix.pixelformat = pixel_format;
    fmt.fmt.pix.field = V4L2_FIELD_NONE;

    if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt)) {
        perror("Error setting Pixel Format");
        ret = -1;
        goto fail;
    }

    struct v4l2_requestbuffers req = {0};
    req.count = 1;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    fprintf(stdout, "Requesting a buffer\n");
    if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
        perror("Error requesting buffer");
        ret = -1;
        goto fail;
    }

    struct v4l2_buffer buf = {0};
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = 0; // bufferindex?

    fprintf(stdout, "Querying buffer\n");
    if(-1 == xioctl(fd, VIDIOC_QBUF, &buf)) {
        perror("Error querying Buffer");
        ret = -1;
        goto fail;
    }

    uint8_t *buffer = NULL;
    fprintf(stdout, "Mmaping buffer\n");
    buffer = mmap (NULL, buf.length, (PROT_READ | PROT_WRITE), MAP_SHARED, fd, buf.m.offset);
    if (MAP_FAILED == buffer) {
        perror("Error mmaping buffer");
        ret = -1;
        goto fail;
    }

    fprintf(stdout, "Turning on streaming\n");
    if(-1 == xioctl(fd, VIDIOC_STREAMON, &buf.type)) {
        perror("Error starting capture");
        ret = -1;
        goto fail;
    }

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    struct timeval tv = {0};
    tv.tv_sec = 2;

    fprintf(stdout, "Calling select on fd\n");
    int r = select((fd + 1), &fds, NULL, NULL, &tv);
    if(-1 == r) {
        perror("Error waiting for frame");
        ret = -1;
        goto fail;
    }

    fprintf(stdout, "Retrieving frame");
    if(-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {
        perror("Error retrieving frame");
        ret = -1;
        goto fail;
    }

    if (out != NULL) {
        fwrite(buffer, sizeof(*buffer), buf.length, out);
    } else {
        write(STDOUT_FILENO, buffer, buf.length);
    }

fail:
    if (out != NULL) {
        if (EOF == fclose(out)) {
            perror("Error closing file");
        }
    }

    if (buffer != NULL && buffer != MAP_FAILED) {
        munmap(buffer, buf.length);
    }

    if ((fd >= 0) && (close(fd) == -1)) {
        perror("Error closing file descriptor");
    }

    return ret;
}

