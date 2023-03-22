/*
 * Userspace program that communicates with the vga_ball device driver
 * through ioctls
 *
 * Stephen A. Edwards
 * Columbia University
 */

#include <stdio.h>
#include "vga_ball.h"
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

int vga_ball_fd;

/* Read and print the background color */
void print_background_color() {
  vga_ball_arg_t vla;
  
  if (ioctl(vga_ball_fd, VGA_BALL_READ_BACKGROUND, &vla)) {
      perror("ioctl(VGA_BALL_READ_BACKGROUND) failed");
      return;
  }
  printf("%02x %02x %02x\n %hu %hu\n",
	 vla.background.red, vla.background.green, vla.background.blue, vla.pos.x, vla.pos.y );
}

/* Set the background color */
void set_background_color(const vga_ball_color_t *c, vga_ball_position_t *p)
{
  vga_ball_arg_t vla;
  vla.background = *c;
  vla.pos = *p;
  if (ioctl(vga_ball_fd, VGA_BALL_WRITE_BACKGROUND, &vla)) {
      perror("ioctl(VGA_BALL_SET_BACKGROUND) failed");
      return;
  }
}

int main()
{
  vga_ball_arg_t vla;
  int i = 0;
  static const char filename[] = "/dev/vga_ball";
  int r = 3;
  int x_speed = 1;
  int y_speed = -1;
  vga_ball_position_t ball_p = {320, 240};
  static const vga_ball_color_t colors[] = {
    { 0xff, 0x00, 0x00 }, /* Red */
    { 0x00, 0xff, 0x00 }, /* Green */
    { 0x00, 0x00, 0xff }, /* Blue */
    { 0xff, 0xff, 0x00 }, /* Yellow */
    { 0x00, 0xff, 0xff }, /* Cyan */
    { 0xff, 0x00, 0xff }, /* Magenta */
    { 0x80, 0x80, 0x80 }, /* Gray */
    { 0x00, 0x00, 0x00 }, /* Black */
    { 0xff, 0xff, 0xff }  /* White */
  };

# define COLORS 9
  
  printf("VGA ball Userspace program started\n");

  if ( (vga_ball_fd = open(filename, O_RDWR)) == -1) {
    fprintf(stderr, "could not open %s\n", filename);
    return -1;
  }

  printf("initial state: ");
  print_background_color();

 /* for (i = 0 ; i < 24 ; i++) {
    set_background_color(&colors[i % COLORS ]);
    print_background_color();
    usleep(400000);
  }*/

  for(;;i++){
	ball_p.x = ball_p.x + x_speed;
	ball_p.y = ball_p.y + y_speed;
	if(ball_p.x < r || ball_p.x > 640-r){
		x_speed = -x_speed;
	}
	if(ball_p.y < r || ball_p.y > 480 - r){
		y_speed = -y_speed;
	}
	set_background_color(&colors[i%COLORS], &ball_p);
	print_background_color();
	usleep(2);
  }
  
  
  printf("VGA BALL Userspace program terminating\n");
  return 0;
}
