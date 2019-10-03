//////////
//Define//
//////////

//#define Com_Test



//COM defines
#define serial_com_baud 2e6
#define serial_wait_time_secure_faktor 3  //how many time/byte you want to wait for the next byte to arrive, before the algorithm goes on with the next step
#define serial_max_com_response_time 2     //[ms] how long the protocol should wait, until a data stream begins (max is 32ms anything else is infinite. If you need more change self_max_com_response_time to unsigned long)

#define error_byte 0x10
#define error_data_byte 0x08
#define error_ack_byte 0x02
#define acknowledge_byte 0x01

bool last_Serial_trans_good = 0;   //Tells you if the last transmition was successful or not
byte serial_rx_buffer [64]; // use this buffer if you want to transver data without a new global value




///////////////
//Com Structs//
///////////////

//Fill in all request bytes for the Slave
//Make sure the communication partner doesn't have a same request byte. That could make the conection unstable.
struct REQUEST_BYTES_SERIAL{ //requierd, so the Slave knows it's an requestbyte and to check the information byte
  byte update_infos = 0x22;
}request_bytes_serial;

byte Serial_read_box() {
  return (Serial.read());
}
byte Serial_peek_box() {
  return (Serial.peek());
}
int  Serial_available_box() {
  return (Serial.available());
}
size_t Serial_write_box(byte b) {
  return (Serial.write(b));
}

typedef struct {
  byte  (*read) ();
  byte  (*peek) ();
  int   (*available) ();
  size_t (*write) (byte);
} COM_LIB_FUN;

COM_LIB_FUN Serial_lib {Serial_read_box, Serial_peek_box, Serial_available_box, Serial_write_box};

bool serial_handle_functions_callback_list(byte r){ //callback functions for the slave Algorythm
//guide lines slave handle functions:
/* 
 * -You need one function for every request
 * -The functions have to return if the recived data is without errors. 1 Yes / 0 NO
 * -If you don't recive data, you don't have to return anything
 * -If you recive data and send data in one function. Save your rx data in the serial_rx_buffer array and use it from their,
 *  otherwise it won't be their if the function get's called agian after an Error. Also if you recognise an error in the incoming data,
 *  return 0 and don't send anything back.
 * -If you want to transfer data to your main code, use global values or the serial_rx_buffer array
 * -Make sure the handle function is as fast as posible
 * -Make sure the UART buffer is empty when you end your function. For example flush it. The Class has a function for that 
*/
 
 bool success = 1; //use it to return if your data is correctly
 
 switch(r){                                           //call by request byte
  case 0x22:    success = handle_info();        break; //call your function as a case
  }

  if(success != 1){
    return(success);
  }else{
    return(1);
  }

}



/*use the next two functions to turn on and off all interrupts that take longer than your wait times, including
 *the resoulting time of the serial_wait_time_secure_faktor. (Which can be really short)
 *Also if you call the call_slave_protocol() function with an timer Interrupt, add those turn on and off functions too.
 *Don't rename this functions.
*/
void serial_protocol_interrupt_turn_on(){
  //call function to turn interrupts on
  
}

void serial_protocol_interrupt_turn_off(){
  //call function to turn interrupts off
  
}

class Serial_protocol
{
  private:
    COM_LIB_FUN self_Serial;
    void* request_adresse;
    int request_size;
    float self_Serial_baud;
    int self_wait_time_secure_faktor;
    int self_max_com_response_time;

    int time_per_byte_micros = ((8.0*1000000)/self_Serial_baud);
    byte sendtype = 0;
    bool master_rx_callback = 0;
    
    int  send_data_buff_wp = 0;
    byte send_data_buff[65];

    //Slave handle function
    bool (*handle_slave) (byte);
    //callback send functionpointer (Master)
    void  (*send_function_serial_protocol) ();
    //rx callback functionpointer (Master)
    bool (*rx_function_serial_protocol) ();
    
    
    //This function waits until one byte is received, with a maximum wait time until the Data Stream begins, defined by the serial_max_com_response_time
    //Does the same as the wait_for_data() function, however it is more efficient if you only wait for a single Byte.
    //returns if data was received
    bool wait_for_i_byte();

    //Checks if the the given byte is known as a slave request byte
    bool check_known_slave_byte(byte b);

    //Main algorithem for com as slave
    void slave_protocol();

    //Main algorithem for com as master
    void master_protocol(byte r);

    
  public:

    //Class constructor
    //give: COM_LIB_FUN struct, address of slave request bytes, the size of the struct, the baud rate, callback function to call all slave callback functions, defined secure_faktor, the max response time
    Serial_protocol(COM_LIB_FUN lib_struct, void* request_struct, int sizeof_request_struct, float com_serial_baud, bool (*handle_function_list) (byte), int wait_time_secure_faktor, int max_com_response_time);

    //This function waits until all bytes are received, with a maximum wait time until the Data Stream begins, defined by the serial_max_com_response_time
    //returns if data was received
    bool wait_for_data();

    //flushes the buffer of the UART
    void flush_serial_buffer();

    //Send a Value over the UART the way it's saved on the RAM not as a string
    //Doesn't call the protocoll, so you should only use it in a callback function
    template <typename INFO_TYPE>
    void send_value(INFO_TYPE data);

    //overwrite the value on the address you give to the function, to whatever is on the UART
    //for example to receive something the send_value function send
    //use it in a callback function
    template <typename INFO_TYPE>
    void get_value(INFO_TYPE* save_adress);    

    //The next 6 functions call the master protocol and give you the option to send data.
    //give: a request byte that the slave understands, 
    //      a value to send additional information’s or a callback function to send them,
    //      a callback function to get data from the slave
    //However, only a request byte is always necessary, if you are happy with only sending a request as a command, you can do that.
    //The value gets send over the UART the way it's saved on the RAM not as a string. Check out the example, so you know how to
    //turn it back to the datatype you want it to be.

    
    void send_request(byte r);

    template <typename INFO_TYPE>
    void send_request(byte r, INFO_TYPE data);

    void send_request_with_callback_f(byte r, void (*callback) ());

    void send_request(byte r, bool (*rx_callback)());

    template <typename INFO_TYPE>
    void send_request(byte r, INFO_TYPE data, bool (*rx_callback)());

    void send_request_with_callback_f(byte r, void (*callback) (), bool (*rx_callback)());


    //Use this function to call the slave protocol. Only if you call it, requests will be answered
    //You can call it repeatedly in the loop function, or if it is not possible to call it often enough,
    //you can call it in a timer interrupt function, but don't forget to call the function to deactivate
    //the interrupt in the serial_protocol_interrupt_turn_off() function.
    void call_slave_protocol();
};

Serial_protocol UNO_SP(Serial_lib, &request_bytes_serial, sizeof(request_bytes_serial), serial_com_baud, serial_handle_functions_callback_list, serial_wait_time_secure_faktor, serial_max_com_response_time);




float infos = -222;





//slave rx function

bool handle_info(){
   
  void* pt = &serial_rx_buffer[0];
  if(Serial.available() == sizeof(float)){
  for(int i = 0; i < sizeof(float); i++){
    serial_rx_buffer[i] = Serial.read();
  }
  infos = *((float*)pt);

  infos++;

  UNO_SP.flush_serial_buffer(); //making sure the buffer is empty

  //send own fixed value to make sure data is no known byte
  Serial.write(0x22); // but it can be the request byte
  //only needed if you recive and send data.

  UNO_SP.send_value(infos);
  
  return(1);
  }else{
    return(0);
  }
}










void setup() {
  // put your setup code here, to run once:
  Serial.begin(serial_com_baud);
}

void loop() {
  // put your main code here, to run repeatedly:
  static unsigned long l_timer = 0, c_timer = 0;
  
  while(infos < 1000000){
    c_timer = millis();
    if(c_timer - l_timer >= 3){
      UNO_SP.send_request(0x23);
      l_timer = c_timer;
    }   
    UNO_SP.call_slave_protocol();
  }
  


  UNO_SP.call_slave_protocol();
}
