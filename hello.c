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
#include "usbkeyboard.h"
#include <stdlib.h>
#include <math.h>

int vga_ball_fd;
uint8_t endpoint_address;
struct libusb_device_handle *keyboard;
/* Read and print the background color */
vga_ball_color_t print_background_color() {
  vga_ball_arg_t vla;
  
  if (ioctl(vga_ball_fd, VGA_BALL_READ_BACKGROUND, &vla)) {
      perror("ioctl(VGA_BALL_READ_BACKGROUND) failed");
      return vla.background;
  }
  printf("%02x %02x %02x\n",
	 vla.background.red, vla.background.green, vla.background.blue);
   return vla.background;
}

/* Set the background color */
void set_background_color(const vga_ball_color_t *c)
{
  vga_ball_arg_t vla;
  vla.background = *c;
  if (ioctl(vga_ball_fd, VGA_BALL_WRITE_BACKGROUND, &vla)) {
      perror("ioctl(VGA_BALL_SET_BACKGROUND) failed");
      return;
  }
}
float cal_joint_angle(float x, float y, float z, float l1, float l2) {
    z = z - 0.0955;
    float theta1 = 202 - acosf((powf(l1, 2) + powf(l2, 2) - powf(y, 2) - powf(z, 2)) / (2 * l1 * l2)) - atanf(z / (y + 0.000001)) - acosf((powf(y, 2) + powf(z, 2) + powf(l2, 2) - powf(l1, 2)) / (2 * l2 * sqrtf(powf(y, 2) + powf(z, 2))));

    float theta2 = atanf(z / y) + acosf((powf(y, 2) + powf(z, 2) + powf(l2, 2) - powf(l1, 2)) / (2 * l2 * sqrtf(powf(y, 2) + powf(z, 2))));

    float theta3 = atanf(y / (x + 0.000001));

    return theta1, theta2, theta3;
}

void cal_angleInput(float x, float y, float z, float theta1_0, float theta2_0, float theta3_0, float initial_angle, uint8_t *output_theta1, uint8_t *output_theta2, uint8_t * output_theta3) {
    float theta1, theta2, theta3,theta1_input,theta2_input,theta3_input;
    theta1, theta2, theta3 = cal_joint_angle(x, y, z, 0.135, 0.209);
    float delta_theta1 = theta1 - theta1_0;
    float delta_theta2 = theta2 - theta2_0;
    float delta_theta3;
    if (theta3 > 0) {
        delta_theta3 = theta3 - theta3_0;
    }
    else {
        delta_theta3 = theta3 + theta3_0;
    }
    theta1_input = initial_angle - delta_theta1;
    theta2_input = initial_angle - delta_theta2;
    theta3_input = initial_angle + delta_theta3;
    if (theta3_input > 255){
      theta3_input = 255;
    }
   
    *output_theta1 = round(theta1_input);
    *output_theta2 = round(theta2_input);
    *output_theta3 = round(theta3_input);


}

int main()
{
     /* Initialize USB KeyBoard */
   struct usb_keyboard_packet packet;
   int transferred;
   char keystate[12] = "00 00 00";
   char prev_keystate[12] = "00 00 00";
   vga_ball_color_t prepos;
   vga_ball_color_t curpos;

  /* Open the keyboard */
   if ( (keyboard = openkeyboard(&endpoint_address)) == NULL ) 
   {
      fprintf(stderr, "Did not find a keyboard\n");
      exit(1);
   }
  vga_ball_arg_t vla;
  int i;
  static const char filename[] = "/dev/vga_ball";


  printf("Robot arm Userspace program started\n");

  if ( (vga_ball_fd = open(filename, O_RDWR)) == -1) {
    fprintf(stderr, "could not open %s\n", filename);
    return -1;
  }

  for (;;) 
   {
      libusb_interrupt_transfer(keyboard, endpoint_address, (unsigned char *) &packet, sizeof(packet), &transferred, 0);
      if (transferred == sizeof(packet)) 
      {
         strcpy(prev_keystate, keystate);
         sprintf(keystate, "%02x %02x %02x", packet.modifiers, packet.keycode[0], packet.keycode[1]);
         printf("Keycode: %s\n", keystate);
         printf("Previous Keycode: %s\n", prev_keystate);

/////////////////////////////////////////////////////////////////////
// Set custom key here, triggered on press
/////////////////////////////////////////////////////////////////////
         prepos = print_background_color();
         if(packet.keycode[0]== 26) // Set wanted keycode w
         {
            if (prev_keystate[0] == '0' && prev_keystate[1] == '0')
            {
               // Do something here
               printf("Key %02x pressed\n",packet.keycode[0]);        
               curpos.red = prepos.red - 5;             
            }
             
         }
         else if(packet.keycode[0] == 22){//s
            if (prev_keystate[0] == '0' && prev_keystate[1] == '0')
            {
               // Do something here
               printf("Key %02x pressed\n",packet.keycode[0]);        
               curpos.red = prepos.red + 5;             
            }
         }
         else if(packet.keycode[0] == 4){//a
            if (prev_keystate[0] == '0' && prev_keystate[1] == '0')
            {
               // Do something here
               printf("Key %02x pressed\n",packet.keycode[0]);        
               curpos.blue = prepos.blue - 5;             
            }
         }
         else if(packet.keycode[0] == 7){//d
            if (prev_keystate[0] == '0' && prev_keystate[1] == '0')
            {
               // Do something here
               printf("Key %02x pressed\n",packet.keycode[0]);        
               curpos.blue = prepos.blue + 5;             
            }
         }
         else if(packet.keycode[0] == 20){//q
            if (prev_keystate[0] == '0' && prev_keystate[1] == '0')
            {
               // Do something here
               printf("Key %02x pressed\n",packet.keycode[0]);        
               curpos.green = prepos.green - 5;             
            }
         }
         else if(packet.keycode[0] == 8){//e
            if (prev_keystate[0] == '0' && prev_keystate[1] == '0')
            {
               // Do something here
               printf("Key %02x pressed\n",packet.keycode[0]);        
               curpos.green = prepos.green + 5;           
            }
         }
         else if(packet.keycode[0] == 44){//space
            if (prev_keystate[0] == '0' && prev_keystate[1] == '0')
            {
               // Do something here
               if (prepos.ot == 70){
                  curpos.ot = 20;
               }
               else if(prepos.ot == 20){
                  curpos.ot = 20;
               }
               printf("Key %02x pressed\n",packet.keycode[0]);                   
            }
         }
         if (curpos.red > 140){
          curpos.red = 140;
         }
         if(curpos.green > 190){
          curpos.green = 190;
         }
         if(curpos.ot > 70){
          curpos.ot = 70;
         }
         else if(curpos.ot < 40){
          curpos.ot = 40;
         }

         set_background_color(&curpos);
      }
   }
  
  printf("VGA BALL Userspace program terminating\n");
  return 0;
}
