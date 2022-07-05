/*

NEDOKONCENE.
Do cvicenia v utorok to dokoncim. 

*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <termios.h> 
#include <unistd.h>  // for STDIN_FILENO
#include <pthread.h>
#include <math.h>

#include "prg_serial_nonblock.h"
#include "messages.h"
#include "event_queue.h"
#include "xwin_sdl.h"
#include "complex_math.h"

#define SERIAL_READ_TIMEOUT_MS 500 //timeout for reading from serial port
#define W 640 //640 or 320
#define H 480 //480 or 240
#define C_RE -0.4
#define C_IM 0.6
#define N 60
#define BOUND1A -1.6
#define BOUND1B -1.1
#define BOUND2A 1.6
#define BOUND2B 1.1
#define DEFAULT_TOTAL_CHUNKS 24 //240
#define DEFAULT_C_WIDTH 160//80
#define DEFAULT_C_HEIGHT 80//4
#define SMALLEST_HEIGHT 10
#define READ_SUCCESS 2
#define ANIMATION_LEN 100

// shared data structure
typedef struct {
   int win_w;
   int win_h;
   bool quit;
   int fd; // serial port file descriptor
} data_t;

pthread_mutex_t mtx;
pthread_cond_t cond;

// THREADS:
void* input_thread(void*);
void* serial_rx_thread(void*); // serial receive buffer

//FUNCTIONS:
int compute_display_params(int w, int h, uint8_t* arr_ptr);
int min(int a, int b); 
void get_display_dimensions(int* w_ptr, int* h_ptr);
bool send_message(data_t *data, message *msg);
void call_termios(int reset);

// - main ---------------------------------------------------------------------
int main(int argc, char *argv[])
{
   data_t data = { .quit = false, .fd = -1, .win_w = W, .win_h = H};
   const char *serial = argc > 1 ? argv[1] : "/dev/ttyACM0";
   data.fd = serial_open(serial);

   if (data.fd == -1) {
      fprintf(stderr, "ERROR: Cannot open serial port %s\n", serial);
      exit(100);
   }

   enum { INPUT, SERIAL_RX, NUM_THREADS };
   const char *threads_names[] = { "Input", "Serial In" };

   void* (*thr_functions[])(void*) = { input_thread, serial_rx_thread};

   pthread_t threads[NUM_THREADS];
   pthread_mutex_init(&mtx, NULL); // initialize mutex with default attributes
   pthread_cond_init(&cond, NULL); // initialize mutex with default attributes

   call_termios(0);

   for (int i = 0; i < NUM_THREADS; ++i) {
      int r = pthread_create(&threads[i], NULL, thr_functions[i], &data);
      fprintf(stderr, "INFO: Create thread '%s' %s\n", threads_names[i], ( r == 0 ? "OK" : "FAIL") );
   }

   // example of local variables for computation and messaging
   struct main_data{
      uint16_t chunk_id;
      double dx;
      double dy;
      unsigned char* img_ptr;
      bool computing;
      uint8_t total_chunks;
      uint8_t c_width;
      uint8_t c_height;
   } computation = { .chunk_id= 0,
                     .dx= (BOUND2A - BOUND1A)/((double)data.win_w),            //  0.01 (with default settings)
                     .dy= (-1)*( (BOUND2B - BOUND1B)/((double)data.win_h) ),   // -0.00916 (with default settings),
                     .computing= false };
   
   // initialize computation.image with zeros:
   computation.img_ptr = (unsigned char*) calloc(RGB*W*H, sizeof(unsigned char));

   // initialize default image resolution
   computation.total_chunks = DEFAULT_TOTAL_CHUNKS;
   computation.c_width = DEFAULT_C_WIDTH;
   computation.c_height = DEFAULT_C_HEIGHT;

   // declare message:
   message msg;
  
   // initialize drawing board:
   xwin_init(data.win_w, data.win_h);  

   bool quit = false;
   while (!quit) {
      //example of the event queue
      event ev = queue_pop();
      if (ev.source == EV_KEYBOARD) {  // handle keyboard events
         msg.type = MSG_NBR;
         switch(ev.type) {
            case EV_HELP:
               printf("ALL HOTKEYS:\n");
               printf("g - gets version\n");
               printf("s - sets computation parameteres\n");
               printf("1 - starts the computation of one chunk\n");
               printf("a - aborts the current computation\n");
               printf("r - sets chunk id to 0\n");
               printf("l - erases the current buffer (image)\n");
               printf("p - refreshes the window with the current buffer content\n");
               printf("c - computes the fractal using CPU\n");
               printf("u - resizes the window with user-submitted dimensions\n");
               printf("o - animation (slow, unfinished)\n");
               printf("q - quits everything\n");
               break;
            case EV_GET_VERSION:
               { // prepare packet for get version
                  msg.type = MSG_GET_VERSION;
                  fprintf(stderr, "INFO: Get version requested\n");
               }
               break;
            case EV_ABORT:
               if(!computation.computing) { // it is not computing
                  fprintf(stderr, "WARN: Abort requested but it is not computing\n\r");
               } else {
                  msg.type = MSG_ABORT;
                  computation.computing = false;
               }
               break;
            case EV_RESET_CHUNK:
               fprintf(stderr, "INFO: Chunk reset request\n\r");
               if (!computation.computing) { // it is not computing
                  computation.chunk_id = 0;
               } else {
                  fprintf(stderr, "WARN: Chunk reset request discarded, it is currently computing\n\r");
               }
               break;
            case EV_SET_COMPUTE:
               msg.type = MSG_SET_COMPUTE;
               if (computation.computing) {
                  fprintf(stderr, "WARN: New computation parameters requested but it is discarded due to on ongoing computation\n");
               } else {
                  
                  msg.data.set_compute.c_re = C_RE;
                  msg.data.set_compute.c_im = C_IM;
                  msg.data.set_compute.d_re= computation.dx;
                  msg.data.set_compute.d_im= computation.dy;
                  msg.data.set_compute.n= N;
                  fprintf(stderr, "INFO: Set new computation resolution %dx%d no. of chunks: %d\n", data.win_w, data.win_h, computation.total_chunks);
               }
               break;
            case EV_COMPUTE:
               if (!computation.computing) { // it has not been computing
                  if (computation.chunk_id < computation.total_chunks) {
                     msg.type = MSG_COMPUTE;
                  
                     int c_per_width = data.win_w/computation.c_width;

                     msg.data.compute.cid = computation.chunk_id;
                     msg.data.compute.re = BOUND1A + (computation.chunk_id%c_per_width) * computation.c_width*computation.dx;       // starting x-coord
                     msg.data.compute.im = BOUND2B + (computation.chunk_id/c_per_width) * computation.c_height*computation.dy;       // starting y-coord
                     //printf("sending x= %f sending y=%f \n", msg.data.compute.re, msg.data.compute.im );
                     msg.data.compute.n_re= computation.c_width;
                     msg.data.compute.n_im= computation.c_height;

                     computation.computing = true;
                     computation.chunk_id ++;
                     fprintf(stderr, "INFO: New computation chunk id: %d for part %d x %d\n", msg.data.compute.cid,
                                                                                             msg.data.compute.n_re,
                                                                                             msg.data.compute.n_im);
                  } else {
                     fprintf(stderr,"INFO: New computation requested but it is discarded - there is nothing left to compute\n");
                  }
                  
               } else {
                  fprintf(stderr, "WARN: New computation requested but it is discarded due on ongoing computation\n\r");
               }
               break;
            case EV_COMPUTE_CPU:
               ;     // without this it would not compile
               int capacity = INIT_CAP;
               complex** main_nbrs = (complex**)calloc(capacity, sizeof(complex));
               if (main_nbrs == NULL) {
                  exit(EXIT_FAILURE);
               }

               complex* z1 = compl_init(main_nbrs, &capacity);
               complex* z2 = compl_init(main_nbrs, &capacity);
               complex* constant = compl_init(main_nbrs, &capacity);
               z1->a = BOUND1A;
               z1->b = BOUND1B;
               z2->a = BOUND2A;
               z2->b = BOUND2B;
               constant->a = C_RE;
               constant->b = C_IM;

               double* t_array = create_t_array(N, constant, data.win_w, data.win_h, z1, z2 );
               unsigned char* image = create_image(data.win_w, data.win_h, t_array);
               xwin_redraw(data.win_w, data.win_h, image);

               free(image);
               free(t_array);
               free_complex_array(main_nbrs);

               break;
            case EV_ANIMATE:
               /*
               Not working properly + slow.
               */
               fprintf(stderr, "INFO: Animation started.\n");
               for (int i=0; i < ANIMATION_LEN; ++i) {
                  int capacity = INIT_CAP;
                  complex** main_nbrs = (complex**)calloc(capacity, sizeof(complex));
                  if (main_nbrs == NULL) {
                     exit(EXIT_FAILURE);
                  }

                  complex* z1 = compl_init(main_nbrs, &capacity);
                  complex* z2 = compl_init(main_nbrs, &capacity);
                  complex* constant = compl_init(main_nbrs, &capacity);
                  z1->a = BOUND1A;
                  z1->b = BOUND1B;
                  z2->a = BOUND2A;
                  z2->b = BOUND2B;
                  constant->a = C_RE + i * 0.1* computation.dx;
                  constant->b = C_IM + i* 0.1* computation.dy ;

                  double* t_array = create_t_array(40, constant, data.win_w, data.win_h, z1, z2 );
                  unsigned char* image = create_image(data.win_w, data.win_h, t_array);
                  xwin_redraw(data.win_w, data.win_h, image);

                  free(image);
                  free(t_array);
                  free_complex_array(main_nbrs);
               }
               fprintf(stderr, "INFO: Animation ended.\n");
               break;
            case EV_CLEAR_BUFFER:
               ;
               free(computation.img_ptr);
               computation.img_ptr = (unsigned char*) calloc(RGB*W*H, sizeof(unsigned char));
               xwin_redraw(data.win_w, data.win_h, computation.img_ptr);
               computation.chunk_id = 0;
               fprintf(stderr, "INFO: Buffer cleared.\n");
               break;
            case EV_REFRESH:
               xwin_redraw(data.win_w, data.win_h, computation.img_ptr);
               break;
            case EV_RESIZE:
               xwin_resize(data.win_w, data.win_h);
               uint8_t disp_params[3] = {0,0,0};
               int r = compute_display_params(data.win_w, data.win_h, disp_params);
               if (r == EXIT_SUCCESS) {
                  computation.chunk_id = 0;
                  computation.dx = (BOUND2A - BOUND1A)/((double)data.win_w);
                  computation.dy = (-1)*( (BOUND2B - BOUND1B)/((double)data.win_h) );
                  computation.total_chunks = disp_params[0];
                  computation.c_width = disp_params[1];
                  computation.c_height = disp_params[2];
                  computation.img_ptr = (unsigned char*)realloc(computation.img_ptr, RGB*data.win_w*data.win_h*sizeof(unsigned char));
               } else {
                  fprintf(stderr, "INFO: Could not find suitable window parameters, reverting chagnes.\n");
                  data.win_w = W;
                  data.win_h = H;
                  xwin_resize(data.win_w, data.win_h);
               }
               
               break;
            case EV_QUIT:
               pthread_mutex_lock(&mtx);
               data.quit = true;
               pthread_mutex_unlock(&mtx);
               quit = true;
               break;
            default:
               break;
         }
         if (msg.type != MSG_NBR) { // message has been set
            if (!send_message(&data, &msg)) {
               fprintf(stderr, "ERROR: send_message() does not send all bytes of the message!\n");
            }
         }
      } else if (ev.source == EV_NUCLEO) { // handle nucleo events
         if (ev.type == EV_SERIAL) {
            message *msg = ev.data.msg;
            switch (msg->type) {
               case MSG_STARTUP:
                  {
                     char str[STARTUP_MSG_LEN+1];
                     for (int i = 0; i < STARTUP_MSG_LEN; ++i) {
                        str[i] = msg->data.startup.message[i];
                     }
                     str[STARTUP_MSG_LEN] = '\0';
                     fprintf(stderr, "INFO: Nucleo restarted - '%s'\n", str);
                  }
                  break;
               case MSG_VERSION:
                  if (msg->data.version.patch > 0) {
                     fprintf(stderr, "INFO: Nucleo firmware ver. %d.%d-p%d\n", msg->data.version.major,
                                                                               msg->data.version.minor,
                                                                               msg->data.version.patch);
                  } else {
                     fprintf(stderr, "INFO: Nucleo firmware ver. %d.%d\n", msg->data.version.major, 
                                                                           msg->data.version.minor);
                  }
                  break;
               case MSG_ERROR:
                  fprintf(stderr, "WARN: Received error from Nucleo\r\n");
                  break;
               case MSG_OK:
                  fprintf(stderr, "INFO: Received ok from Nucleo\r\n");
                  break;
               case MSG_COMPUTE_DATA:
                  if(computation.computing) {
                     uint8_t c_id = msg->data.compute_data.cid;
                     int c_per_width = data.win_w/computation.c_width;
                     int pivot_x = ( c_id%c_per_width ) * computation.c_width;
                     int pivot_y = ( c_id/c_per_width ) * computation.c_height;

                     int pixel_x = pivot_x + msg->data.compute_data.i_re;
                     int pixel_y = pivot_y + msg->data.compute_data.i_im;
                     //printf("i_re= %d, i_im =%d", msg->data.compute_data.i_re, msg->data.compute_data.i_im); 

                     double t = ((double)msg->data.compute_data.iter)/((double)N);    // k/N
                     unsigned char r = (unsigned char) round( R(t) );
                     unsigned char g = (unsigned char) round( G(t) );
                     unsigned char b = (unsigned char) round( B(t) );
                     *(computation.img_ptr + RGB*(pixel_y*data.win_w + pixel_x) ) = r;
                     *(computation.img_ptr + RGB*(pixel_y*data.win_w + pixel_x) + 1) = g;
                     *(computation.img_ptr + RGB*(pixel_y*data.win_w + pixel_x) + 2) = b;

                     /* printf("k = %d, r_idx=%d, g_idx=%d, b_idx=%d\n",msg->data.compute_data.iter,
                     RGB*(pixel_y*W + pixel_x),
                     RGB*(pixel_y*W + pixel_x) + 1,
                     RGB*(pixel_y*W + pixel_x) + 2); */

                  } else {
                     fprintf(stderr, "WARN: Nucleo sends new data without computing \r\n");
                  }
                  break;
               case MSG_DONE:
                  fprintf(stderr, "INFO: Nucleo reports the computation is done computing: %d\r\n",
                                                                              computation.computing);
                  xwin_redraw(data.win_w, data.win_h, computation.img_ptr);
                  computation.computing = false;
                  break;
               case MSG_ABORT:
                  fprintf(stderr, "INFO: Abort from Nucleo\r\n");
                  computation.computing = false;
                  break;
               default:
                  break;
            }
            if (msg) {
               free(msg);
            }
         } else if (ev.type == EV_QUIT) {
            quit = true;
         } else {
            // ignore all other events
         }
      }
     
   } // end main quit
   // queue_cleanup(); // cleanup all events and free allocated memory for messages.
   for (int i = 0; i < NUM_THREADS; ++i) {
      fprintf(stderr, "INFO: Call join to the thread %s\n", threads_names[i]);
      int r = pthread_join(threads[i], NULL);
      fprintf(stderr, "INFO: Joining the thread %s has been %s\n", threads_names[i], (r == 0 ? "OK" : "FAIL"));
   }
   free(computation.img_ptr);
   serial_close(data.fd);
   call_termios(1); // restore terminal settings
   return EXIT_SUCCESS;
}

// - function -----------------------------------------------------------------
void call_termios(int reset)
{
   static struct termios tio, tioOld;
   tcgetattr(STDIN_FILENO, &tio);
   if (reset) {
      tcsetattr(STDIN_FILENO, TCSANOW, &tioOld);
   } else {
      tioOld = tio; //backup 
      cfmakeraw(&tio);
      tio.c_oflag |= OPOST;
      tcsetattr(STDIN_FILENO, TCSANOW, &tio);
   }
}

// - thread //-----------------------------------------------------------------
// HANDLES KEYBOARD INPUTS
void* input_thread(void* d)
{
   data_t *data = (data_t*)d;
   bool end = false;
   int c;
   event ev = {.source = EV_KEYBOARD };
   while ( !end && (c = getchar())) {
      ev.type = EV_TYPE_NUM;
      switch(c) {
         case 'h':
            ev.type = EV_HELP;
            break;
         case 'g': 
            ev.type = EV_GET_VERSION;
            break;
         case 's':
            ev.type = EV_SET_COMPUTE;
            break;
         case '1':
            ev.type = EV_COMPUTE;
            break;
         case 'a':
            ev.type = EV_ABORT;
            break;
         case 'r':
            ev.type = EV_RESET_CHUNK;
            break;
         case'l':
            ev.type = EV_CLEAR_BUFFER;
            break;
         case 'p':
            ev.type = EV_REFRESH;
            break;
         case 'c':
            ev.type = EV_COMPUTE_CPU;
            break;
         case 'u':
            ev.type = EV_RESIZE;
            int new_w;
            int new_h;
            xwin_close();
            get_display_dimensions(&new_w, &new_h);
            pthread_mutex_lock(&mtx);
            data->win_w = new_w;
            data->win_h = new_h;
            pthread_mutex_unlock(&mtx);
            fprintf(stderr, "INFO: New image window dimensions have been seledcted.\n");
            break;
         case 'o':
            ev.type = EV_ANIMATE;
            break;
         case 'q':
            ev.type = EV_ABORT;
            end = true;
            xwin_close();              
            break;
         default:
            break;
      }
      if (ev.type != EV_TYPE_NUM) { // new event 
         queue_push(ev);
      }
      pthread_mutex_lock(&mtx);
      end = end || data->quit; // check for quit
      pthread_mutex_unlock(&mtx);
   }
   ev.type = EV_QUIT;
   queue_push(ev);
   fprintf(stderr, "INFO: Exit input thread %p\n", (void*)pthread_self());
   return NULL;
}

// - thread -------------------------------------------------------------------
// HANDLES MESSAGES FROM NUCLEO ON SERIAL PORT
void* serial_rx_thread(void* d) 
{ // read bytes from the serial and puts the parsed message to the queue
   data_t *data = (data_t*)d;
   uint8_t msg_buf[sizeof(message)]; // maximal buffer for all possible messages defined in messages.h
   event ev = { .source = EV_NUCLEO, .type = EV_SERIAL, .data.msg = NULL };
   bool end = false;
   int len;
   int count = 0; 
   unsigned char c;
   while (serial_getc_timeout(data->fd, SERIAL_READ_TIMEOUT_MS, &c) > 0) {}; // discard garbage

   while (!end) {
      int r = serial_getc_timeout(data->fd, SERIAL_READ_TIMEOUT_MS, &c);
      if (r > 0) { // character has been read

         // new message, try to find out its size:
         if (count == 0){
            if (!get_message_size(c, &len)){
               fprintf(stderr, "ERROR: Unknown message type has been received 0x%x\n - '%c'\r", c, c);
               end = true;
               continue;
            }
         }

         msg_buf[count] = c;
         count++;

         // message is complete:
         if (count == len){
            count = 0;
            message* msg_ptr = malloc(sizeof(message));
            if(msg_ptr == NULL) {
               exit(EXIT_FAILURE);
            }
            ev.data.msg = msg_ptr;

            if (!parse_message_buf(msg_buf, len, ev.data.msg)){
               fprintf(stderr, "ERROR: Cannot parse message type %d\n\r", msg_buf[0]);
               free(msg_ptr);
               end = true;
               continue;
            } else {
               queue_push(ev);
            }

         }
      } else if (r == 0) { //read but nothing has been received
         // TODO you may put your code here
      } else {
         fprintf(stderr, "ERROR: Cannot receive data from the serial port\n");
         end = true;
      }
      pthread_mutex_lock(&mtx);
      end = end || data->quit;
      pthread_mutex_unlock(&mtx);
   }
   ev.type = EV_QUIT;
   queue_push(ev);
   fprintf(stderr, "INFO: Exit serial_rx_thread %p\n", (void*)pthread_self());
   return NULL;
}

// - function -----------------------------------------------------------------
bool send_message(data_t *data, message *msg) 
{
   bool ret = true;
   pthread_mutex_lock(&mtx);
   int fd = data->fd;
   pthread_mutex_unlock(&mtx);
   
   uint8_t msg_buf[sizeof(message)];
   
   int msg_size;
   int msg_buf_len=0;
   ret = get_message_size(msg->type, &msg_size);

   if (ret) {
      ret = fill_message_buf(msg, msg_buf, msg_size, &msg_buf_len);
   }

   if (ret) {
      for (int i=0; i < msg_buf_len; ++i) {
         if (serial_putc(fd, msg_buf[i])== -1) {
            ret = false;
            break;
         }
      }
   }

   return ret;
}

 // - functions -----------------------------------------------------------------
int min(int a, int b) {
   return (a>b) ? a : b;
}

int compute_display_params(int w, int h, uint8_t* arr_ptr ) {

   int cw_start = (int) ( (1/((double)SMALLEST_HEIGHT))*w);
   int ch_start = (int) SMALLEST_HEIGHT;
   int cw;
   int ch;
   //printf("cw=%d, ch=%d", cw, ch);
   double w_divide_cw = ((double)w)/cw_start;
   double h_divide_ch = ((double)h)/ch_start;
   int c;

   int minimum = min(h, 255);
   int total_possible =  minimum * cw_start;
   int cnt = 0;

   bool no_possible_params = false;
   bool found = false;

   while (!no_possible_params && !found) {
      ch = ch_start + cnt % minimum;
      cw = cw_start - cnt / minimum;
      c = (w*h)/(cw*ch);
   
      cnt ++;

      w_divide_cw = ((double)w)/cw;
      h_divide_ch = ((double)h)/ch;

      if (cw < 0 || ch > minimum) no_possible_params = true;
      if ( cnt > total_possible ) no_possible_params = true;
      if ( (c < 256) && (ceilf(w_divide_cw) == w_divide_cw) && (ceilf(h_divide_ch) == h_divide_ch) ) found = true;
   }

   if (found) {
      //printf("FOUND: cw=%d, ch= %d, c=%d\n", cw, ch, c);
      arr_ptr[0] = c;
      arr_ptr[1] = cw;
      arr_ptr[2] = ch;
      return EXIT_SUCCESS;
   } else {
      //printf("DIDNT FIND, selecting default params\n");
      arr_ptr[0] = DEFAULT_TOTAL_CHUNKS;
      arr_ptr[1] = DEFAULT_C_WIDTH;
      arr_ptr[2] = DEFAULT_C_HEIGHT;
      return EXIT_FAILURE;
   }
   
} 

// get window dimensions from the user
void get_display_dimensions(int* w_ptr, int* h_ptr) {
   call_termios(1);
   printf("Enter new window dimensions in format widthxheight (integer values):\n");
   int r = scanf("%dx%d", w_ptr, h_ptr);
   while (r != READ_SUCCESS) {
      printf("ERROR: Inappropriate value(s).\nEnter new window dimensions in format widthxheight (integer values):\n");
      r = scanf("%dx%d", w_ptr, h_ptr);;
   }
   call_termios(0);
}