#ifndef _VGA_BALL_H
#define _VGA_BALL_H

#include <linux/ioctl.h>

typedef struct {
	unsigned char s0, s1, s2, ot;
} servos_t;
  

typedef struct {
  servos_t all_servos;
} control_servo_t;

#define SERVOS_MAGIC 'q'

/* ioctls and their arguments */
#define WRITE_to_SERVO _IOW(SERVOS_MAGIC, 1, control_servo_t *)
#define READ_SERVO _IOR(SERVOS_MAGIC, 2, control_servo_t *)

#endif
