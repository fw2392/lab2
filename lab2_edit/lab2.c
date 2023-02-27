/*
 *
 * CSEE 4840 Lab 2 for 2019
 *
 * Name/UNI: Please Changeto Yourname (pcy2301)
 */
#include "fbputchar.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "usbkeyboard.h"
#include <pthread.h>

/* Update SERVER_HOST to be the IP address of
 * the chat server you are connecting to
 */
/* arthur.cs.columbia.edu */
#define SERVER_HOST "128.59.19.114"
#define SERVER_PORT 42000

#define BUFFER_SIZE 128

/*
 * References:
 *
 * http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html
 * http://www.thegeekstuff.com/2011/12/c-socket-programming/
 * 
 */

int sockfd; /* Socket file descriptor */

struct libusb_device_handle *keyboard;
uint8_t endpoint_address;

pthread_t network_thread;
void *network_thread_f(void *);
int convert_to_ascii(uint8_t, uint8_t, uint8_t, int, int);
int main()
{
  int err, col;

  struct sockaddr_in serv_addr;

  struct usb_keyboard_packet packet;
  int transferred;
  char keystate[12];

  if ((err = fbopen()) != 0) {
    fprintf(stderr, "Error: Could not open framebuffer: %d\n", err);
    exit(1);
  }

  /* Draw rows of asterisks across the top and bottom of the screen */
  for (col = 0 ; col < 64 ; col++) {
    fbputchar('*', 0, col);
    fbputchar('*', 23, col);
  }
  for (col = 0; col <64; col++)
  {
    fbputchar('-', 12, col);
  }
  

  fbputs("Hello CSEE 4840 World!", 4, 10);

  /* Open the keyboard */
  if ( (keyboard = openkeyboard(&endpoint_address)) == NULL ) {
    fprintf(stderr, "Did not find a keyboard\n");
    exit(1);
  }
    
  /* Create a TCP communications socket */
  if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
    fprintf(stderr, "Error: Could not create socket\n");
    exit(1);
  }

  /* Get the server address */
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(SERVER_PORT);
  if ( inet_pton(AF_INET, SERVER_HOST, &serv_addr.sin_addr) <= 0) {
    fprintf(stderr, "Error: Could not convert host IP \"%s\"\n", SERVER_HOST);
    exit(1);
  }

  /* Connect the socket to the server */
  if ( connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
    fprintf(stderr, "Error: connect() failed.  Is the server running?\n");
    exit(1);
  }

  /* Start the network thread */
  pthread_create(&network_thread, NULL, network_thread_f, NULL);

  /* Look for and handle keypresses */
  int rownum = 21;
  int colnum = 0;
  char message_to_send[128];
  int charIdex = 0;
  char acsii;
  for (;;) {
    fbputchar('_',rownum, colnum);
    libusb_interrupt_transfer(keyboard, endpoint_address,
			      (unsigned char *) &packet, sizeof(packet),
			      &transferred, 0);
    if (transferred == sizeof(packet)) {
      sprintf(keystate, "%02x %02x %02x", packet.modifiers, packet.keycode[0],
	      packet.keycode[1]);
      printf("%s\n", keystate);
      if(packet.keycode[0] == 0x28){
        printf("%s\n", message_to_send);
        write(sockfd,message_to_send,strlen(message_to_send));
        fbclean(strlen(message_to_send),21,0);   
        message_to_send[0] = '\0';
        rownum = 21;
        colnum = 0;
        charIdex = 0;
        
      }
      else if(packet.keycode[0] == 0x2A){
        fbputchar(' ',rownum,colnum);
        if((colnum-1 < 0)){
          colnum = 63;
          rownum = rownum - 1;
        }
        else{
          colnum = colnum - 1;
        }
        charIdex = charIdex - 1;
        message_to_send[charIdex] = '\0';   
      }
      else if ((packet.keycode[0] != 0x0) || (packet.keycode[1]!= 0x0) || (packet.modifiers != 0x0)){
        acsii = convert_to_ascii(packet.keycode[0], packet.keycode[1], packet.modifiers, rownum, colnum);
        colnum+=1;
        message_to_send[charIdex] = acsii;
        charIdex+=1;
        if(colnum == 64){
          colnum = 0;
          rownum = 22;
        }
      }
      

      //fbputs(keystate, 6, 0);
      
      if (packet.keycode[0] == 0x29) { /* ESC pressed? */
	      break;
      }


    }
  }

  /* Terminate the network thread */
  pthread_cancel(network_thread);

  /* Wait for the network thread to finish */
  pthread_join(network_thread, NULL);

  return 0;
}

void *network_thread_f(void *ignored)
{
  char recvBuf[BUFFER_SIZE];
  int n;
  int rownum = 1;
  /* Receive data */
  while ( (n = read(sockfd, &recvBuf, BUFFER_SIZE - 1)) > 0 ) {
    recvBuf[n] = '\0';
    printf("%s", recvBuf);
    fbputs(recvBuf, rownum, 0);
    rownum+=1;
  }

  return NULL;
}

int convert_to_ascii(uint8_t keycode0,uint8_t keycode1,uint8_t modifier,int row, int col){
    char c;
    
    if(keycode0 >= 4 && keycode0 <= 29){
      c = keycode0 + 93;
      if(modifier == 2){
        c = c - 32;
    }
    }
    else if(keycode0 >=  30 && keycode0 <= 27){
      c = keycode0 + 19;
      if(modifier == 2){
        switch (keycode0)
        {
          case 30:
            c = 33;
            break;
          case 31:
            c = 64;
            break;
          case 32:
            c = 35;
            break;
          case 33:
            c = 36;
            break;
          case 34:
            c = 37;
            break;
          case 35:
            c = 94;
            break;
          case 36:
            c = 38;
            break;
          case 37:
            c = 42;
            break;
          case 38:
            c = 40;
            break;
          case 39:
            c = 41;
            break;    
          default:
            break;
        }
      }
    }
    fbputchar(c,row,col);

    return c;

}

