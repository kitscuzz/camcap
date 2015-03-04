#ifndef __V4L2_HELPER_
#define __V4L2_HELPER_

#include <stdint.h>

void print_pix_formats(void);
const char* pix_fmt_to_str(uint32_t fmt);
uint32_t str_to_pix_fmt(const char *short_name);

void print_capabilities(uint32_t caps);
int enum_formats(int fd, enum v4l2_buf_type type, struct v4l2_fmtdesc **formats);
int enum_frame_size(int fd, int pixel_format, struct v4l2_frmsizeenum **frm_sz_enum);
#endif
