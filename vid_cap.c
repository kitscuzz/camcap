#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <linux/videodev2.h>

static int xioctl(int fd, int request, void *arg) {
    int ret;
    do {
        ret = ioctl(fd, request, arg);
    } while(ret == -1 && errno == EINTR);
    return ret;
}

void print_capabilities(uint32_t caps) {
    if (caps & V4L2_CAP_VIDEO_CAPTURE) { fprintf(stdout, "V4L2_CAP_VIDEO_CAPTURE, "); }
    if (caps & V4L2_CAP_VIDEO_OUTPUT) { fprintf(stdout, "V4L2_CAP_VIDEO_OUTPUT, "); }
    if (caps & V4L2_CAP_VIDEO_OVERLAY) { fprintf(stdout, "V4L2_CAP_VIDEO_OVERLAY, "); }
    if (caps & V4L2_CAP_VBI_CAPTURE) { fprintf(stdout, "V4L2_CAP_VBI_CAPTURE, "); }
    if (caps & V4L2_CAP_VBI_OUTPUT) { fprintf(stdout, "V4L2_CAP_VBI_OUTPUT, "); }
    if (caps & V4L2_CAP_SLICED_VBI_CAPTURE) { fprintf(stdout, "V4L2_CAP_SLICED_VBI_CAPTURE, "); }
    if (caps & V4L2_CAP_SLICED_VBI_OUTPUT) { fprintf(stdout, "V4L2_CAP_SLICED_VBI_OUTPUT, "); }
    if (caps & V4L2_CAP_RDS_CAPTURE) { fprintf(stdout, "V4L2_CAP_RDS_CAPTURE, "); }
    if (caps & V4L2_CAP_VIDEO_OUTPUT_OVERLAY) { fprintf(stdout, "V4L2_CAP_VIDEO_OUTPUT_OVERLAY, "); }
    if (caps & V4L2_CAP_HW_FREQ_SEEK) { fprintf(stdout, "V4L2_CAP_HW_FREQ_SEEK, "); }
    if (caps & V4L2_CAP_RDS_OUTPUT) { fprintf(stdout, "V4L2_CAP_RDS_OUTPUT, "); }
    if (caps & V4L2_CAP_VIDEO_CAPTURE_MPLANE) { fprintf(stdout, "V4L2_CAP_VIDEO_CAPTURE_MPLANE, "); }
    if (caps & V4L2_CAP_VIDEO_OUTPUT_MPLANE) { fprintf(stdout, "V4L2_CAP_VIDEO_OUTPUT_MPLANE, "); }
    if (caps & V4L2_CAP_VIDEO_M2M_MPLANE) { fprintf(stdout, "V4L2_CAP_VIDEO_M2M_MPLANE, "); }
    if (caps & V4L2_CAP_VIDEO_M2M) { fprintf(stdout, "V4L2_CAP_VIDEO_M2M, "); }
    if (caps & V4L2_CAP_TUNER) { fprintf(stdout, "V4L2_CAP_TUNER, "); }
    if (caps & V4L2_CAP_AUDIO) { fprintf(stdout, "V4L2_CAP_AUDIO, "); }
    if (caps & V4L2_CAP_RADIO) { fprintf(stdout, "V4L2_CAP_RADIO, "); }
    if (caps & V4L2_CAP_MODULATOR) { fprintf(stdout, "V4L2_CAP_MODULATOR, "); }
    if (caps & V4L2_CAP_READWRITE) { fprintf(stdout, "V4L2_CAP_READWRITE, "); }
    if (caps & V4L2_CAP_ASYNCIO) { fprintf(stdout, "V4L2_CAP_ASYNCIO, "); }
    if (caps & V4L2_CAP_STREAMING) { fprintf(stdout, "V4L2_CAP_STREAMING, "); }

    if (caps & V4L2_CAP_DEVICE_CAPS) { fprintf(stdout, "V4L2_CAP_DEVICE_CAPS, "); }
    fprintf(stdout, "\n");
}

static const char* get_pix_fmt(uint32_t fmt) {
    if (fmt == V4L2_PIX_FMT_RGB332) { return "RGB332"; }
    if (fmt == V4L2_PIX_FMT_RGB444) { return "RGB444"; }
    if (fmt == V4L2_PIX_FMT_RGB555) { return "RGB555"; }
    if (fmt == V4L2_PIX_FMT_RGB565) { return "RGB565"; }
    if (fmt == V4L2_PIX_FMT_RGB555X) { return "RGB555X"; }
    if (fmt == V4L2_PIX_FMT_RGB565X) { return "RGB565X"; }
    if (fmt == V4L2_PIX_FMT_BGR666) { return "BGR666"; }
    if (fmt == V4L2_PIX_FMT_BGR24) { return "BGR24"; }
    if (fmt == V4L2_PIX_FMT_RGB24) { return "RGB24"; }
    if (fmt == V4L2_PIX_FMT_BGR32) { return "BGR32"; }
    if (fmt == V4L2_PIX_FMT_RGB32) { return "RGB32"; }
    if (fmt == V4L2_PIX_FMT_GREY) { return "GREY"; }
    if (fmt == V4L2_PIX_FMT_Y4) { return "Y4"; }
    if (fmt == V4L2_PIX_FMT_Y6) { return "Y6"; }
    if (fmt == V4L2_PIX_FMT_Y10) { return "Y10"; }
    if (fmt == V4L2_PIX_FMT_Y12) { return "Y12"; }
    if (fmt == V4L2_PIX_FMT_Y16) { return "Y16"; }
    if (fmt == V4L2_PIX_FMT_Y10BPACK) { return "Y10BPACK"; }
    if (fmt == V4L2_PIX_FMT_PAL8) { return "PAL8"; }
    if (fmt == V4L2_PIX_FMT_UV8) { return "UV8"; }
    if (fmt == V4L2_PIX_FMT_YVU410) { return "YVU410"; }
    if (fmt == V4L2_PIX_FMT_YVU420) { return "YVU420"; }
    if (fmt == V4L2_PIX_FMT_YUYV) { return "YUYV"; }
    if (fmt == V4L2_PIX_FMT_YYUV) { return "YYUV"; }
    if (fmt == V4L2_PIX_FMT_YVYU) { return "YVYU"; }
    if (fmt == V4L2_PIX_FMT_UYVY) { return "UYVY"; }
    if (fmt == V4L2_PIX_FMT_VYUY) { return "VYUY"; }
    if (fmt == V4L2_PIX_FMT_YUV422P) { return "YUV422P"; }
    if (fmt == V4L2_PIX_FMT_YUV411P) { return "YUV411P"; }
    if (fmt == V4L2_PIX_FMT_Y41P) { return "Y41P"; }
    if (fmt == V4L2_PIX_FMT_YUV444) { return "YUV444"; }
    if (fmt == V4L2_PIX_FMT_YUV555) { return "YUV555"; }
    if (fmt == V4L2_PIX_FMT_YUV565) { return "YUV565"; }
    if (fmt == V4L2_PIX_FMT_YUV32) { return "YUV32"; }
    if (fmt == V4L2_PIX_FMT_YUV410) { return "YUV410"; }
    if (fmt == V4L2_PIX_FMT_YUV420) { return "YUV420"; }
    if (fmt == V4L2_PIX_FMT_HI240) { return "HI240"; }
    if (fmt == V4L2_PIX_FMT_HM12) { return "HM12"; }
    if (fmt == V4L2_PIX_FMT_M420) { return "M420"; }
    if (fmt == V4L2_PIX_FMT_NV12) { return "NV12"; }
    if (fmt == V4L2_PIX_FMT_NV21) { return "NV21"; }
    if (fmt == V4L2_PIX_FMT_NV16) { return "NV16"; }
    if (fmt == V4L2_PIX_FMT_NV61) { return "NV61"; }
    if (fmt == V4L2_PIX_FMT_NV24) { return "NV24"; }
    if (fmt == V4L2_PIX_FMT_NV42) { return "NV42"; }
    if (fmt == V4L2_PIX_FMT_NV12M) { return "NV12M"; }
    if (fmt == V4L2_PIX_FMT_NV21M) { return "NV21M"; }
    if (fmt == V4L2_PIX_FMT_NV16M) { return "NV16M"; }
    if (fmt == V4L2_PIX_FMT_NV61M) { return "NV61M"; }
    if (fmt == V4L2_PIX_FMT_NV12MT) { return "NV12MT"; }
    if (fmt == V4L2_PIX_FMT_NV12MT_16X16) { return "NV12MT_16X16"; }
    if (fmt == V4L2_PIX_FMT_YUV420M) { return "YUV420M"; }
    if (fmt == V4L2_PIX_FMT_YVU420M) { return "YVU420M"; }
    if (fmt == V4L2_PIX_FMT_SBGGR8) { return "SBGGR8"; }
    if (fmt == V4L2_PIX_FMT_SGBRG8) { return "SGBRG8"; }
    if (fmt == V4L2_PIX_FMT_SGRBG8) { return "SGRBG8"; }
    if (fmt == V4L2_PIX_FMT_SRGGB8) { return "SRGGB8"; }
    if (fmt == V4L2_PIX_FMT_SBGGR10) { return "SBGGR10"; }
    if (fmt == V4L2_PIX_FMT_SGBRG10) { return "SGBRG10"; }
    if (fmt == V4L2_PIX_FMT_SGRBG10) { return "SGRBG10"; }
    if (fmt == V4L2_PIX_FMT_SRGGB10) { return "SRGGB10"; }
    if (fmt == V4L2_PIX_FMT_SBGGR12) { return "SBGGR12"; }
    if (fmt == V4L2_PIX_FMT_SGBRG12) { return "SGBRG12"; }
    if (fmt == V4L2_PIX_FMT_SGRBG12) { return "SGRBG12"; }
    if (fmt == V4L2_PIX_FMT_SRGGB12) { return "SRGGB12"; }
    if (fmt == V4L2_PIX_FMT_SBGGR10ALAW8) { return "SBGGR10ALAW8"; }
    if (fmt == V4L2_PIX_FMT_SGBRG10ALAW8) { return "SGBRG10ALAW8"; }
    if (fmt == V4L2_PIX_FMT_SGRBG10ALAW8) { return "SGRBG10ALAW8"; }
    if (fmt == V4L2_PIX_FMT_SRGGB10ALAW8) { return "SRGGB10ALAW8"; }
    if (fmt == V4L2_PIX_FMT_SBGGR10DPCM8) { return "SBGGR10DPCM8"; }
    if (fmt == V4L2_PIX_FMT_SGBRG10DPCM8) { return "SGBRG10DPCM8"; }
    if (fmt == V4L2_PIX_FMT_SGRBG10DPCM8) { return "SGRBG10DPCM8"; }
    if (fmt == V4L2_PIX_FMT_SRGGB10DPCM8) { return "SRGGB10DPCM8"; }
    if (fmt == V4L2_PIX_FMT_SBGGR16) { return "SBGGR16"; }
    if (fmt == V4L2_PIX_FMT_MJPEG) { return "MJPEG"; }
    if (fmt == V4L2_PIX_FMT_JPEG) { return "JPEG"; }
    if (fmt == V4L2_PIX_FMT_DV) { return "DV"; }
    if (fmt == V4L2_PIX_FMT_MPEG) { return "MPEG"; }
    if (fmt == V4L2_PIX_FMT_H264) { return "H264"; }
    if (fmt == V4L2_PIX_FMT_H264_NO_SC) { return "H264_NO_SC"; }
    if (fmt == V4L2_PIX_FMT_H264_MVC) { return "H264_MVC"; }
    if (fmt == V4L2_PIX_FMT_H263) { return "H263"; }
    if (fmt == V4L2_PIX_FMT_MPEG1) { return "MPEG1"; }
    if (fmt == V4L2_PIX_FMT_MPEG2) { return "MPEG2"; }
    if (fmt == V4L2_PIX_FMT_MPEG4) { return "MPEG4"; }
    if (fmt == V4L2_PIX_FMT_XVID) { return "XVID"; }
    if (fmt == V4L2_PIX_FMT_VC1_ANNEX_G) { return "VC1_ANNEX_G"; }
    if (fmt == V4L2_PIX_FMT_VC1_ANNEX_L) { return "VC1_ANNEX_L"; }
    if (fmt == V4L2_PIX_FMT_VP8) { return "VP8"; }
    if (fmt == V4L2_PIX_FMT_CPIA1) { return "CPIA1"; }
    if (fmt == V4L2_PIX_FMT_WNVA) { return "WNVA"; }
    if (fmt == V4L2_PIX_FMT_SN9C10X) { return "SN9C10X"; }
    if (fmt == V4L2_PIX_FMT_SN9C20X_I420) { return "SN9C20X_I420"; }
    if (fmt == V4L2_PIX_FMT_PWC1) { return "PWC1"; }
    if (fmt == V4L2_PIX_FMT_PWC2) { return "PWC2"; }
    if (fmt == V4L2_PIX_FMT_ET61X251) { return "ET61X251"; }
    if (fmt == V4L2_PIX_FMT_SPCA501) { return "SPCA501"; }
    if (fmt == V4L2_PIX_FMT_SPCA505) { return "SPCA505"; }
    if (fmt == V4L2_PIX_FMT_SPCA508) { return "SPCA508"; }
    if (fmt == V4L2_PIX_FMT_SPCA561) { return "SPCA561"; }
    if (fmt == V4L2_PIX_FMT_PAC207) { return "PAC207"; }
    if (fmt == V4L2_PIX_FMT_MR97310A) { return "MR97310A"; }
    if (fmt == V4L2_PIX_FMT_JL2005BCD) { return "JL2005BCD"; }
    if (fmt == V4L2_PIX_FMT_SN9C2028) { return "SN9C2028"; }
    if (fmt == V4L2_PIX_FMT_SQ905C) { return "SQ905C"; }
    if (fmt == V4L2_PIX_FMT_PJPG) { return "PJPG"; }
    if (fmt == V4L2_PIX_FMT_OV511) { return "OV511"; }
    if (fmt == V4L2_PIX_FMT_OV518) { return "OV518"; }
    if (fmt == V4L2_PIX_FMT_STV0680) { return "STV0680"; }
    if (fmt == V4L2_PIX_FMT_TM6000) { return "TM6000"; }
    if (fmt == V4L2_PIX_FMT_CIT_YYVYUY) { return "CIT_YYVYUY"; }
    if (fmt == V4L2_PIX_FMT_KONICA420) { return "KONICA420"; }
    if (fmt == V4L2_PIX_FMT_JPGL) { return "JPGL"; }
    if (fmt == V4L2_PIX_FMT_SE401) { return "SE401"; }
    if (fmt == V4L2_PIX_FMT_S5C_UYVY_JPG) { return "S5C_UYVY_JPG"; }
    return "\0";
}

int main(int argc, char *argv[]) {
    int fd, ret = 0;
    fd = open("/dev/video0", O_RDWR);
    if (fd == -1) {
        perror("Error opening video device");
        ret = -1;
        goto fail;
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
    print_capabilities(caps.capabilities);
    fprintf(stdout, "Device caps = 0x%08x\n", caps.device_caps);

    int fmt_cnt = 0, format_size = 1;
    struct v4l2_fmtdesc * format = NULL;
    format = malloc(format_size * sizeof(struct v4l2_fmtdesc));
    if (format == NULL) {
        fprintf(stderr, "Unable to allocate space for format");
        ret = -1;
        goto fail;
    }

    while(1) {
        if (fmt_cnt >= format_size) {
            format_size *= 2;
            struct v4l2_fmtdesc * tmp = realloc(format, format_size * sizeof(struct v4l2_fmtdesc));
            if (tmp == NULL) {
                fprintf(stderr, "Unable to allocate space for format");
                ret = -1;
                goto fail;
            }
            format = tmp;
        }
        format[fmt_cnt].index = fmt_cnt;
        format[fmt_cnt].type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        int ioctl_ret = xioctl(fd, VIDIOC_ENUM_FMT, &format[fmt_cnt]);
        if (-1 == ioctl_ret && errno == EINVAL) {
            break;
        }

        fmt_cnt++;
    }

    fprintf(stdout, "Got %d formats of type V4L2_BUF_TYPE_VIDEO_CAPTURE:\n", fmt_cnt);
    for (int i = 0; i < fmt_cnt; i++) {
        char desc[sizeof(format[i].description) + 1] = {'\0'};
        memcpy(format[i].description, desc, sizeof(format[i].description));
        fprintf(stdout, "\tDescription: \"%s\"; Pixelformat: %s\n", desc, get_pix_fmt(format[i].pixelformat));

        fprintf(stdout, "\tFrame sizes:\n");
        int frmsz_cnt = 0, frmsz_size = 1;
        struct v4l2_frmsizeenum *frmsz = malloc(frmsz_size * sizeof(struct v4l2_frmsizeenum));
        if (frmsz == NULL) {
            fprintf(stderr, "Unable to allocate space for frmsz");
            ret = -1;
            goto fail;
        }

        while(1) {
            if (frmsz_cnt >= frmsz_size) {
                frmsz_size *= 2;
                struct v4l2_frmsizeenum *tmp = realloc(frmsz, frmsz_size * sizeof(struct v4l2_frmsizeenum));
                if (tmp == NULL) {
                    fprintf(stderr, "Unable to allocate space for frmsz");
                    free(frmsz);
                    ret = -1;
                    goto fail;
                }
                frmsz = tmp;
            }
            frmsz[frmsz_cnt].index = frmsz_cnt;
            frmsz[frmsz_cnt].pixel_format = format[i].pixelformat;
            int ioctl_ret = xioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frmsz[frmsz_cnt]);
            if (-1 == ioctl_ret && errno == EINVAL) {
                break;
            }

            frmsz_cnt++;
        }

        for(int j = 0; j < frmsz_cnt; j++) {
            if (frmsz[j].type == V4L2_FRMSIZE_TYPE_DISCRETE) {
                fprintf(stdout, "\t\t%dx%d\n", frmsz[j].discrete.width, frmsz[j].discrete.height);
            } else {
                fprintf(stdout, "\t\tFrom %dx%d to %dx%d by %dx%d\n",
                        frmsz[j].stepwise.min_width, frmsz[j].stepwise.min_height,
                        frmsz[j].stepwise.max_width, frmsz[j].stepwise.max_height,
                        frmsz[j].stepwise.step_width, frmsz[j].stepwise.step_height);
            }
        }

        free(frmsz);
    }

    struct v4l2_format fmt = {0};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = 640;
    fmt.fmt.pix.height = 480;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.field = V4L2_FIELD_NONE;

    fprintf(stdout, "Setting pixel format\n");
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
    if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req))
    {
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

    FILE *out = NULL;
    out = fopen("whoa.jpg", "w");
    if (out == NULL) {
        perror("Error opening output file");
        ret = -1;
        goto fail;
    }

    fwrite(buffer, sizeof(*buffer), buf.length, out);

fail:
    if (out != NULL) {
        if (EOF == fclose(out)) {
            perror("Error closing file");
        }
    }

    if (buffer != NULL && buffer != MAP_FAILED) {
        munmap(buffer, buf.length);
    }

    free(format);
    format = NULL;

    if ((fd >= 0) && (close(fd) == -1)) {
        perror("Error closing file descriptor");
    }

    return ret;
}

