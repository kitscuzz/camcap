#ifndef __V4L2_HELPER_
#define __V4L2_HELPER_

#include <stdint.h>

struct mmaped_buffer {
    void *start;
    size_t length;
};

void print_pix_formats(void);
const char* pix_fmt_to_str(uint32_t fmt);
uint32_t str_to_pix_fmt(const char *short_name);

void print_capabilities(uint32_t caps);

int get_device_capabilities(int fd, struct v4l2_capability *caps);

int enum_pixel_formats(int fd, enum v4l2_buf_type type, struct v4l2_fmtdesc **formats);
int enum_frame_size(int fd, int pixel_format, struct v4l2_frmsizeenum **frm_sz_enum);
int pixel_format_valid(int fd, enum v4l2_buf_type type, uint32_t pixel_format);
int frame_size_valid(int fd, uint32_t pixel_format, uint32_t width, uint32_t height);

int set_stream_format(int fd, struct v4l2_format *fmt);
int init_mmap_buffers(int fd, struct mmaped_buffer *bufs, int count);
int start_mmap_streaming(int fd, int buf_count);
int stop_streaming(int fd);

int read_frame(int fd, struct v4l2_buffer *buf);
int enqueue_frame(int fd, struct v4l2_buffer *buf);
#endif
