
#include <stdio.h>
#include "servo_control.h"
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include "usbkeyboard.h"
#include <stdlib.h>
#include <math.h>


int servos_fd;
uint8_t endpoint_address;
struct libusb_device_handle *keyboard;
/* Read and print the background color */
typedef struct {
	float t0, t1, t2;
} theta_t;

servos_t print_servo_angle() {
  control_servo_t vla;
  
  if (ioctl(servos_fd, READ_SERVO, &vla)) {
      perror("ioctl(READ_SERVO) failed");
      return vla.all_servos;
  }
  printf("%02x %02x %02x %02x\n",
	 vla.all_servos.s0, vla.all_servos.s1, vla.all_servos.s2, vla.all_servos.ot);
   return vla.all_servos;
}

/* Set the background color */
void set_servo_angle(const servos_t *c)
{
  control_servo_t vla;
  vla.all_servos = *c;
  if (ioctl(servos_fd, WRITE_to_SERVO, &vla)) {
      perror("ioctl(SERVOS_SET_BACKGROUND) failed");
      return;
  }
}
theta_t cal_joint_angle(float x, float y, float z) {
   theta_t thetas;
    float l1 = 0.135;
    float l2 = 0.209;
    z = z - 0.0955;
    float thetas.t0 = 202 - acosf((powf(l1, 2) + powf(l2, 2) - powf(y, 2) - powf(z, 2)) / (2 * l1 * l2)) - \
    atanf(z / (y + 0.000001)) - acosf((powf(y, 2) + powf(z, 2) + powf(l2, 2) - powf(l1, 2)) / (2 * l2 * sqrtf(powf(y, 2) + powf(z, 2))));

    float thetas.t1 = atanf(z / y) + acosf((powf(y, 2) + powf(z, 2) + powf(l2, 2) - powf(l1, 2)) / (2 * l2 * sqrtf(powf(y, 2) + powf(z, 2))));

    float thetas.t2 = atanf(y / (x + 0.000001));
    printf("before");
    printf("%f %f %f\n",thetas.t0, thetas.t1, theta.t0);
    return thetas;
}

servos_t cal_angleInput(float x, float y, float z) {
    float theta1_0 = 22;
    float theta2_0 = 113;
    float theta3_0 = 90;
    float initial_angle = 135;
    float theta1_input,theta2_input,theta3_input;
    theta_t thetas;
    thetas = cal_joint_angle(x, y, z);
    float delta_theta1 = thetas.t0 - theta1_0;
    float delta_theta2 = thetas.t1 - theta2_0;
    float delta_theta3;
    if (theta3 > 0) {
        delta_theta3 = thetas.t2 - theta3_0;
    }
    else {
        delta_theta3 = thetas.t2 + theta3_0;
    }
    printf("after");
    printf("%f %f %f\n",thetas.t0, thetas.t1, theta.t0);
    theta1_input = initial_angle - delta_theta1;
    theta2_input = initial_angle - delta_theta2;
    theta3_input = initial_angle + delta_theta3;
    if (theta3_input > 255){
      theta3_input = 255;
    }
   
    // *output_theta1 = round(theta1_input);
    // *output_theta2 = round(theta2_input);
    // *output_theta3 = round(theta3_input);
    servos_t angle;
    angle.s0 = round(theta1_input);
    angle.s1 = round(theta2_input);
    angle.s2 = round(theta3_input);
}

int main()
{
     /* Initialize USB KeyBoard */
   struct usb_keyboard_packet packet;
   int transferred;
   char keystate[12] = "00 00 00";
   char prev_keystate[12] = "00 00 00";
   servos_t prepos;
   servos_t curpos;
   char input[200];

  /* Open the keyboard */
   if ( (keyboard = openkeyboard(&endpoint_address)) == NULL ) 
   {
      fprintf(stderr, "Did not find a keyboard\n");
      exit(1);
   }
  control_servo_t vla;
  int i;
  static const char filename[] = "/dev/vga_ball";


  printf("Robot arm Userspace program started\n");

  if ( (servos_fd = open(filename, O_RDWR)) == -1) {
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

         prepos = print_servo_angle();
         if(packet.keycode[0] == 29)// z reset the position
         {
          if (prev_keystate[0] == '0' && prev_keystate[1] == '0'){
            printf("Key %02x pressed\n",packet.keycode[0]); 
            curpos.s0 = 135;
            curpos.s1 = 135;
            curpos.s2 = 135;
            curpos.ot = 20;
          }
         }

         else if(packet.keycode[0]== 26) // Set wanted keycode w
         {
            if (prev_keystate[0] == '0' && prev_keystate[1] == '0')
            {
               // Do something here
               printf("Key %02x pressed\n",packet.keycode[0]); 
               if(prepos.s0 == 0){
                curpos.s0 == prepos.s0;
               }
               else{
                curpos.s0 = prepos.s0 - 5;
               }       
                            
            }
             
         }
         else if(packet.keycode[0] == 22){//s
            if (prev_keystate[0] == '0' && prev_keystate[1] == '0')
            {
               // Do something here
               printf("Key %02x pressed\n",packet.keycode[0]);        
               curpos.s0 = prepos.s0 + 5;             
            }
         }
         else if(packet.keycode[0] == 4){//a
            if (prev_keystate[0] == '0' && prev_keystate[1] == '0')
            {
               // Do something here
               printf("Key %02x pressed\n",packet.keycode[0]); 
               if (prepos.s2 == 0){
                curpos.s2 = prepos.s2;
               }
               else{
                curpos.s2 = prepos.s2 - 5;
               }       
                            
            }
         }
         else if(packet.keycode[0] == 7){//d
            if (prev_keystate[0] == '0' && prev_keystate[1] == '0')
            {
               // Do something here
               printf("Key %02x pressed\n",packet.keycode[0]);        
               curpos.s2 = prepos.s2 + 5;             
            }
         }
         else if(packet.keycode[0] == 20){//q
            if (prev_keystate[0] == '0' && prev_keystate[1] == '0')
            {
               // Do something here
               printf("Key %02x pressed\n",packet.keycode[0]);
               if(prepos.s1 == 0){
                curpos.s1 = prepos.s1;
               }    
               else{
                curpos.s1 = prepos.s1 - 5;  
               }    
                          
            }
         }
         else if(packet.keycode[0] == 8){//e
            if (prev_keystate[0] == '0' && prev_keystate[1] == '0')
            {
               // Do something here
               printf("Key %02x pressed\n",packet.keycode[0]);        
               curpos.s1 = prepos.s1 + 5;           
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
                  curpos.ot = 70;
               }
               printf("Key %02x pressed\n",packet.keycode[0]);                   
            }
         }
         else if(packet.keycode[0] == 14){
          printf("Enter the position and seperate by space like x y z: ");
          memset(input, 0, sizeof(input));
          fget(input,200,stdin);
          char *tmp = strtok(input,"\n");
          char *first_pos = strtok(tmp," ");
          char *second_pos = strtok(NULL," ");
          char *third_pos = strtok(NULL," ");
          float first_float = atof(first_pos);
          float second_float = atof(second_pos);
          float third_float = atof(third_pos);
          curpos = cal_angleInput(first_float,second_float,third_float);
          printf("After algorithm\n");
          printf("%02x %02x %02x %02x\n",
	 curpos.s0, curpos.s1, curpos.s2, curpos.ot);
         }
         if (curpos.s0 > 150){
          curpos.s0 = 150;
         }
         
         if(curpos.s1 > 190){
          curpos.s1 = 190;
         }

         if(curpos.ot > 70){
          curpos.ot = 70;
         }
         else if(curpos.ot < 20){
          curpos.ot = 20;
         }
         
         set_servo_angle(&curpos);
      }
   }
  
  printf("Robot ARM Userspace program terminating\n");
  return 0;
}
