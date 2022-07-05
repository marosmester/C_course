#include "mbed.h"
#include "messages.h"

DigitalOut myled(LED1);
Serial serial(SERIAL_TX, SERIAL_RX);
Ticker ticker;
InterruptIn mybutton(USER_BUTTON);
 
#define BUF_SIZE 255
 
bool abort_request = false;
char tx_buffer[BUF_SIZE];
char rx_buffer[BUF_SIZE];
 
// pointers to the circular buffers
volatile int tx_in = 0;
volatile int tx_out = 0;
volatile int rx_in = 0;
volatile int rx_out = 0;

#define MESSAGE_SIZE sizeof(message)
#define CONV_LIMIT 2    // convergence limit
#define INIT_CAP 100

typedef struct {
    double a;
    double b;
} complex;

// FUNCTION DECLARATIONS--------------------------------------------------------
void Tx_interrupt();
void Rx_interrupt();
bool send_buffer(const uint8_t* msg, unsigned int size);
bool receive_message(uint8_t *msg_buf, int size, int *len);
bool send_message(const message *msg, uint8_t *buf, int size);

void add_to_compl_arr(complex**compl_arr, int* cap, complex* new_cn);
complex* compl_init(complex** all_compl_nbrs, int* cap);
void free_complex_array(complex** all_compl_nbrs);
double abs_val(complex* cn);
complex* compl_square(complex* cn, complex** all_compl_nbrs, int* cap);
complex* compl_add(complex* cn1, complex* cn2, complex** all_compl_nbrs, int* cap);
int find_k(complex* cn_init, complex* c, int n);


void tick() {
    myled = !myled;
}

void btn_press() {
    abort_request = true;
}
// -----------------------------------------------------------------------------

int main() {
    mybutton.fall(&btn_press);
    serial.baud(115200);
    serial.attach(&Rx_interrupt, Serial::RxIrq); // attach interrupt handler to receive data
    serial.attach(&Tx_interrupt, Serial::TxIrq); // attach interrupt handler to transmit data

    // blink LED 5 times
    for (int i = 0; i < 10; i++) {
        myled = !myled;
        wait(0.15);
    }   
    while (serial.readable())
        serial.getc();
//    serial.putc('i');
    struct {
       uint16_t chunk_id;
       //SET_COMPUTE:
       double c_re;  // re (x) part of the c constant in recursive equation
       double c_im;  // im (y) part of the c constant in recursive equation
       double d_re;  // increment in the x-coords
       double d_im;  // increment in the y-coords
       uint8_t n;    // number of iterations per each pixel
       //COMPUTE:
       uint16_t nbr_tasks;
       uint16_t task_id;
       double re; 
       double im;
       uint8_t n_re;
       uint8_t n_im;
    } nucleo_data = {0, 0, 0, 0, 0, 0, 0, 0, 0};    

    message msg = { .type = MSG_STARTUP, .data.startup.message = {'M','E','S','T','E','M','A','R'} };
    uint8_t msg_buf[MESSAGE_SIZE];
    int msg_len;

    send_message(&msg, msg_buf, MESSAGE_SIZE);
    bool computing = false;
    float period = 0.2;
    while (1) {
        if (abort_request) {
            if (computing) {  //abort computing
                msg.type = MSG_ABORT;
                send_message(&msg, msg_buf, MESSAGE_SIZE);
                computing = false;
                abort_request = false;
                ticker.detach();
                myled = 0;
            }
        }
        if (rx_in != rx_out) { // something is in the receive buffer
            if (receive_message(msg_buf, MESSAGE_SIZE, &msg_len)) {
                if (parse_message_buf(msg_buf, msg_len, &msg)) {
                    switch(msg.type) {
                        case MSG_GET_VERSION:
                            msg.type = MSG_VERSION;
                            msg.data.version.major  = 0;
                            msg.data.version.minor  = 1;
                            msg.data.version.patch  = 2;
                            send_message(&msg, msg_buf, MESSAGE_SIZE);
                            break;
                        case MSG_ABORT:
                            msg.type = MSG_OK;
                            send_message(&msg, msg_buf, MESSAGE_SIZE);
                            computing = false;
                            abort_request = false;
                            ticker.detach();
                            myled = 0;
                            break;
                        case MSG_SET_COMPUTE:
                            nucleo_data.c_re = msg.data.set_compute.c_re;
                            nucleo_data.c_im = msg.data.set_compute.c_im;
                            nucleo_data.d_re = msg.data.set_compute.d_re;
                            nucleo_data.d_im = msg.data.set_compute.d_im;
                            nucleo_data.n = msg.data.set_compute.n;
                            msg.type = MSG_OK;
                            send_message(&msg, msg_buf, MESSAGE_SIZE); 
                            break;
                        case MSG_COMPUTE:
                            //if (msg.data.compute.nbr_tasks > 0) {
                             //   ticker.attach(tick, period);
                             //   nucleo_data.chunk_id = msg.data.compute.chunk_id;
                            //    nucleo_data.nbr_tasks = msg.data.compute.nbr_tasks;
                            //    nucleo_data.task_id = 0; // reset the task counter
                            //    computing = true;
                            //}
                            nucleo_data.chunk_id = msg.data.compute.cid;
                            nucleo_data.re = msg.data.compute.re;
                            nucleo_data.im = msg.data.compute.im;
                            nucleo_data.n_re = msg.data.compute.n_re;
                            nucleo_data.n_im = msg.data.compute.n_im;
                            nucleo_data.nbr_tasks = msg.data.compute.n_re * msg.data.compute.n_im;
                            nucleo_data.task_id = 0;
                                        
                            computing = true;
                            ticker.attach(tick, period);
                            msg.type = MSG_OK;
                            send_message(&msg, msg_buf, MESSAGE_SIZE);
                            break;
                    } // end switch
                } else { // message has not been parsed send error
                    msg.type = MSG_ERROR;
                    send_message(&msg, msg_buf, MESSAGE_SIZE);
                }
            } // end message received
        } else if (computing) {
            if (nucleo_data.task_id < nucleo_data.nbr_tasks) {
                // do some computation
                uint8_t x_pixel_coord = nucleo_data.task_id % nucleo_data.n_re;
                uint8_t y_pixel_coord = nucleo_data.task_id / nucleo_data.n_re;
                
                double actual_re = nucleo_data.re + x_pixel_coord * nucleo_data.d_re;
                double actual_im = nucleo_data.im + y_pixel_coord* nucleo_data.d_im;
                
                complex cn;
                cn.a = actual_re;
                cn.b = actual_im;
                
                complex constant;
                constant.a = nucleo_data.c_re;
                constant.b = nucleo_data.c_im;
                
                int k = find_k(&cn, &constant, nucleo_data.n);
                
                msg.type = MSG_COMPUTE_DATA;
                msg.data.compute_data.cid = nucleo_data.chunk_id;
                msg.data.compute_data.i_re = x_pixel_coord;
                msg.data.compute_data.i_im = y_pixel_coord;
                msg.data.compute_data.iter = k;      // k
            
                nucleo_data.task_id += 1;
                send_message(&msg, msg_buf, MESSAGE_SIZE);
            } else { //computation done
                ticker.detach();
                myled = 0;
                msg.type = MSG_DONE;
                send_message(&msg, msg_buf, MESSAGE_SIZE);
                computing = false;
            }
        } else {
            sleep(); // put the cpu to sleep mode, it will be wakeup on interrupt
        }
    } // end while (1)
}



bool send_message(const message *msg, uint8_t *buf, int size) {
    return fill_message_buf(msg, buf, MESSAGE_SIZE, &size)
                        && send_buffer(buf, size);
}

void Tx_interrupt()
{
    // send a single byte as the interrupt is triggered on empty out buffer 
    if (tx_in != tx_out) {
        serial.putc(tx_buffer[tx_out]);
        tx_out = (tx_out + 1) % BUF_SIZE;
    } else { // buffer sent out, disable Tx interrupt
        USART2->CR1 &= ~USART_CR1_TXEIE; // disable Tx interrupt
    }
    return;
}

void Rx_interrupt()
{
    // receive bytes and stop if rx_buffer is full
    while ((serial.readable()) && (((rx_in + 1) % BUF_SIZE) != rx_out)) {
        rx_buffer[rx_in] = serial.getc();
        rx_in = (rx_in + 1) % BUF_SIZE;
    }
    return;
}

bool send_buffer(const uint8_t* msg, unsigned int size)
{
    if (!msg && size == 0) {
        return false;    // size must be > 0
    }
    int i = 0;
    NVIC_DisableIRQ(USART2_IRQn); // start critical section for accessing global data
    USART2->CR1 |= USART_CR1_TXEIE; // enable Tx interrupt on empty out buffer
    bool empty = (tx_in == tx_out);
    while ( (i == 0) || i < size ) { //end reading when message has been read
        if ( ((tx_in + 1) % BUF_SIZE) == tx_out) { // needs buffer space
            NVIC_EnableIRQ(USART2_IRQn); // enable interrupts for sending buffer
            while (((tx_in + 1) % BUF_SIZE) == tx_out) {
                /// let interrupt routine empty the buffer
            }
            NVIC_DisableIRQ(USART2_IRQn); // disable interrupts for accessing global buffer
        }
        tx_buffer[tx_in] = msg[i];
        i += 1;
        tx_in = (tx_in + 1) % BUF_SIZE;
    } // send buffer has been put to tx buffer, enable Tx interrupt for sending it out
    USART2->CR1 |= USART_CR1_TXEIE; // enable Tx interrupt
    NVIC_EnableIRQ(USART2_IRQn); // end critical section
    return true;
}

bool receive_message(uint8_t *msg_buf, int size, int *len)
{
    bool ret = false;
    int i = 0;
    *len = 0; // message size
    NVIC_DisableIRQ(USART2_IRQn); // start critical section for accessing global data
    while ( ((i == 0) || (i != *len)) && i < size ) {
        if (rx_in == rx_out) { // wait if buffer is empty
            NVIC_EnableIRQ(USART2_IRQn); // enable interrupts for receing buffer
            while (rx_in == rx_out) { // wait of next character
            }
            NVIC_DisableIRQ(USART2_IRQn); // disable interrupts for accessing global buffer
        }
        uint8_t c = rx_buffer[rx_out];
        if (i == 0) { // message type
            if (get_message_size(c, len)) { // message type recognized
                msg_buf[i++] = c;
                ret = *len <= size; // msg_buffer must be large enough
            } else {
                ret = false;
                break; // unknown message
            }
        } else {
            msg_buf[i++] = c;
        }
        rx_out = (rx_out + 1) % BUF_SIZE;
    }
    NVIC_EnableIRQ(USART2_IRQn); // end critical section
    return ret;
}

//list of complex numbers
void add_to_compl_arr(complex**compl_arr, int* cap, complex* new_cn) {
    int occupied = 0;
    complex* element = compl_arr[0];
    while (element != NULL) {
        occupied++;
        element = *(compl_arr + occupied);
    }

    //add number to array:
    *(compl_arr + occupied) = new_cn;
   
}

//create a complex number
complex* compl_init(complex** all_compl_nbrs, int* cap) {
    complex* cn = (complex*)malloc(sizeof(complex));
    if(cn == NULL) {
        exit(EXIT_FAILURE);
    }
    add_to_compl_arr(all_compl_nbrs, cap, cn);
    return cn;
}

//free all complex numbers
void free_complex_array(complex** all_compl_nbrs) {
    int cnt = 0;
    while (*(all_compl_nbrs + cnt)!=NULL) {
        free( *(all_compl_nbrs+cnt) );
        cnt++;
    }
    free(all_compl_nbrs);
}

//MATH
//abs value
double abs_val(complex* cn) {
    return sqrt(cn->a*cn->a + cn->b*cn->b);
}

//complex square
complex* compl_square(complex* cn, complex** all_compl_nbrs, int* cap) {    
    complex* ret = compl_init(all_compl_nbrs, cap);
    ret->a = cn->a*cn->a - cn->b*cn->b;      //a^2 - b^2
    ret->b = 2*cn->a*cn->b;                  //2abi
    return ret;
}
//complex addition
complex* compl_add(complex* cn1, complex* cn2, complex** all_compl_nbrs, int* cap) {
    complex* ret = compl_init(all_compl_nbrs, cap);
    ret->a = cn1->a + cn2->a;
    ret->b = cn1->b + cn2->b;
    return ret;
}

//
int find_k(complex* cn_init, complex* c, int n) {
    int k = 0;
    int cap = 2*n + 10;
    complex** numbers = (complex**) calloc(cap, sizeof(complex));
    if (numbers == NULL) {
        exit(EXIT_FAILURE);
    }

    complex* z = compl_init(numbers, &cap);
    z->a = cn_init->a;
    z->b = cn_init->b;

    while ( (abs_val(z) < CONV_LIMIT) && (k < n) ){
        complex* new_z = compl_square(z, numbers, &cap);
        z = compl_add(new_z , c, numbers, &cap);
        k++;
    }

    free_complex_array(numbers);

    return k;
}

