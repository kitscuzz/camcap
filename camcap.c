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

#define BUFFER_COUNT 4

static void print_pixel_formats(int fd) {
    // Check to see if they've selected a valid pixel format
    struct v4l2_fmtdesc *format = NULL;

    int fmt_cnt = enum_pixel_formats(fd, V4L2_BUF_TYPE_VIDEO_CAPTURE, &format);
    if (fmt_cnt < -1) {
        fprintf(stderr, "Unable to enumerate video capture formats\n");
        return;
    }

    fprintf(stdout, "No/invalid pixel format specified! Please select from the following:\n");
    for (int i = 0; i < fmt_cnt; i++) {
        fprintf(stdout, "%s\n", pix_fmt_to_str(format[i].pixelformat));
    }

    free(format);
    format = NULL;
}

static void print_frame_sizes(int fd, uint32_t pixel_format) {
    struct v4l2_frmsizeenum *fsze = NULL;
    int frm_sz_cnt = enum_frame_size(fd, pixel_format, &fsze);
    if (frm_sz_cnt < 0) {
        perror("Error enumerating frame sizes for pixel format");
        return;
    }

    fprintf(stdout, "Not all/invalid frame sizes specified! Please select from the following:\n");

    for (int i = 0; i < frm_sz_cnt; i++) {
        switch(fsze[i].type) {
            case V4L2_FRMSIZE_TYPE_DISCRETE:
                fprintf(stdout, "%dx%d\n", fsze[i].discrete.width, fsze[i].discrete.height);
                break;

            case V4L2_FRMSIZE_TYPE_STEPWISE:
                fprintf(stdout, "From %dx%d to %dx%d by %dx%d\n",
                        fsze[i].stepwise.min_width, fsze[i].stepwise.min_height,
                        fsze[i].stepwise.max_width, fsze[i].stepwise.max_height,
                        fsze[i].stepwise.step_width, fsze[i].stepwise.step_height);
                break;
                
            case V4L2_FRMSIZE_TYPE_CONTINUOUS:
                fprintf(stdout, "Any dimension between %dx%d and %dx%d\n",
                        fsze[i].stepwise.min_width, fsze[i].stepwise.min_height,
                        fsze[i].stepwise.max_width, fsze[i].stepwise.max_height);
                break;

            default:
                fprintf(stdout, "Unable to show frame formats, this program does not know how to "
                        "utilize this device. Got V4L2_FRMSIZE_TYPE: %d\n", fsze[i].type);
                break;
        }
    }

    free(fsze);
    fsze = NULL;
}

void print_usage(const char *argv0) {
    fprintf(stderr, "Usage: %s --device=/dev/video0 [options]\n\n"
            "-d | --device  The video capture device to use\n"
            "-f | --format  The pixel format of the video (YUYV, MJPEG, etc)\n"
            "-w | --width   The frame width, in pixels\n"
            "-h | --height  The frame height, in pixels\n"
            "-c | --count   The number of frames to grab from the camera\n"
            "-o | --output  The filename to output data to (stdout normally)\n",
            argv0);
}

int main(int argc, char *argv[]) {
    int fd = -1, ret = 0;
    static struct option long_options[] = {
        {"device", required_argument, 0, 'd' },
        {"format", required_argument, 0, 'f' },
        {"width",  required_argument, 0, 'w' },
        {"height", required_argument, 0, 'h' },
        {"count",  required_argument, 0, 'c' },
        {"output", required_argument, 0, 'o' },
        {0,        0,                 0,  0  }
    };
    static char options[] = "d:f:w:h:c:o:";

    const char *dev_name = NULL;
    const char *out_name = NULL;
    uint32_t pixel_format = 0;
    int width = 0, height = 0;
    int frame_count = 1;
    int opt;
    while (-1 != (opt = getopt_long(argc, argv, options, long_options, NULL))) {
        switch (opt) {
            case 'c':
                if (frame_count == 1) {
                    char *endptr = NULL;
                    long parsed = strtol(optarg, &endptr, 0);
                    if (parsed > INT_MAX || parsed <= 0 || *endptr != '\0') {
                        fprintf(stderr, "ERROR: Unable to parse given frame count: \"%s\"\n", optarg);
                    }
                    frame_count = (int) parsed;
                } else {
                    fprintf(stderr, "ERROR: Can only accept 1 frame count per instantiation."
                            "%d is already the current frame count, cannot accept \"%s\"\n", frame_count, optarg);
                    return -1;
                }
                break;

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
                print_usage(argv[0]);
                return -1;
        }
    }

    if (dev_name == NULL) {
        fprintf(stderr, "Please provide a valid device name (like /dev/video0)!\n");
        print_usage(argv[0]);
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
    if (get_device_capabilities(fd, &caps) < 0) {
        perror("Error querying capabilities");
        ret = -1;
        goto fail;
    }

    char driver[sizeof(caps.driver) + 1] = {'\0'};
    memcpy(driver, caps.driver, sizeof(caps.driver));
    fprintf(stdout, "Driver= \"%s\"\n", driver);

    char card[sizeof(caps.card) + 1] = {'\0'};
    memcpy(card, caps.card, sizeof(caps.card));
    fprintf(stdout, "Card = \"%s\"\n", card);

    char bus_info[sizeof(caps.bus_info) + 1] = {'\0'};
    memcpy(bus_info, caps.bus_info, sizeof(caps.bus_info));
    fprintf(stdout, "Bus info = \"%s\"\n", bus_info);

    fprintf(stdout, "V4L2 driver version = %d\n", caps.version);
    fprintf(stdout, "Capabilities = 0x%08x\n", caps.capabilities);
    if (caps.capabilities & V4L2_CAP_DEVICE_CAPS) {
        fprintf(stdout, "Device caps = 0x%08x\n", caps.device_caps);
        print_capabilities(caps.device_caps);
    } else {
        print_capabilities(caps.capabilities);
    }

    if (!(caps.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        fprintf(stderr, "Error: device does not support video capture!\n");
        ret = -1;
        goto fail;
    }

    if (!pixel_format_valid(fd, V4L2_BUF_TYPE_VIDEO_CAPTURE, pixel_format)) {
        print_pixel_formats(fd);
        goto fail;
    }

    if (!frame_size_valid(fd, pixel_format, width, height)) {
        print_frame_sizes(fd, pixel_format);
        goto fail;
    }

    fprintf(stdout, "Setting stream format\n");

    struct v4l2_format fmt = {0};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = width;
    fmt.fmt.pix.height = height;
    fmt.fmt.pix.pixelformat = pixel_format;
    fmt.fmt.pix.field = V4L2_FIELD_NONE;

    if (set_stream_format(fd, &fmt) != 0) {
        perror("Error setting format");
        ret = -1;
        goto fail;
    }

    fprintf(stdout, "Mmaping buffers\n");
    // Initialize all the buffers. We give a few spots so the camera can continue to stream
    // the next frame while our program processes the previous one.  The number of buffers
    // should be no less than 2 for streaming, the v4l2 docs example gives 4
    struct mmaped_buffer bufs[BUFFER_COUNT];
    if (-1 == init_mmap_buffers(fd, bufs, BUFFER_COUNT)) {
        perror("Error mmaping buffers");
        ret = -1;
        goto fail;
    }

    // Start streaming!
    if (-1 == start_mmap_streaming(fd, BUFFER_COUNT)) {
        perror("Error starting stream");
        ret = -1;
        goto fail;
    }

    // Select loop while we read frames
    int cur_frame = 0;
    while (cur_frame < frame_count) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(fd, &fds);

        struct timeval tv = {0};
        tv.tv_sec = 2;

        int r = select((fd + 1), &fds, NULL, NULL, &tv);
        if(-1 == r) {
            if (EINTR == errno)
                continue;

            perror("Error waiting for next frame");
            ret = -1;
            goto fail;
        }

        if (0 == r) {
            fprintf(stderr, "Timeout waiting for next frame\n");
            ret = -1;
            goto fail;
        }

        struct v4l2_buffer buf;
        if (-1 == read_frame(fd, &buf)) {
            perror("Error reading frame");
            ret = -1;
            goto fail;
        }

        if (NULL != out) {
            if (!fwrite((bufs[buf.index].start), buf.bytesused, 1, out)) {
                perror("Error writing output to file");
                ret = -1;
                goto fail;
            }
        } else {
            write(STDOUT_FILENO, bufs[buf.index].start, buf.bytesused);
        }

        enqueue_frame(fd, &buf);

        fprintf(stdout, "Written frame %d\n", cur_frame);
        cur_frame++;
    }

    if(-1 == stop_streaming(fd)) {
        perror("Error stopping stream");
        ret = 1;
    }

fail:
    if ((out != NULL) && (EOF == fclose(out))) {
        perror("Error closing file");
    }

    for (int i = 0; i < BUFFER_COUNT; i++) {
        if (bufs[i].start != NULL && bufs[i].start != MAP_FAILED) {
            munmap(bufs[i].start, bufs[i].length);
        }
    }

    if ((fd >= 0) && (close(fd) == -1)) {
        perror("Error closing file descriptor");
    }
    fd = -1;

    return ret;
}

