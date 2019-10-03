//privat

    bool Serial_protocol::wait_for_i_byte(){
      
      int available = 0;
      unsigned long start_t = micros();
      unsigned long timer;
    
      do {
        timer = micros();
        available = self_Serial.available();  
        if (timer < start_t) start_t = timer; //catch timer overflow
      } while (   available == 0 && (timer - start_t) <= self_max_com_response_time * 1000 );
    
      if (available == 0){
        return(0);
      }else{
        return(1);
      }
            
      }


    bool Serial_protocol::check_known_slave_byte(byte b)
    {
      for(int i = 0; i < request_size; i++)
      {
        if(b == *((byte*)request_adresse + i)) return(1);
      }
      return(0);
    }



    //////////////////
    //Slave Protocol//
    //////////////////


    void Serial_protocol::slave_protocol(){
      
      int status = 0;
      byte r_byte;
      byte ack_byte;
      bool trans = 1;
      bool recive_status = 0;

      int error_num = 0;

      
      if(self_Serial.available() != 0){
      serial_protocol_interrupt_turn_off(); //call function to turn interrupts off

      do{
        switch(status){

          case 0: //wait for data get request

          #ifdef Com_Test
          Serial.print("start_slave ");
          #endif

          recive_status = wait_for_data();

     
          
          if(recive_status == 0){ 
          //No conection End
          status = 99;
          
          #ifdef Com_Test
          Serial.print(" S_C0_FFF_r ");
          #endif
          
          }else{
            r_byte = self_Serial.read();

            if(r_byte == error_byte){//double Slave Problem end
               status = 99;
            }else{
               status = 1; //always go to 1
            }

            #ifdef Com_Test
          
           if(r_byte == error_byte){
              Serial.print("S_rx_error ");
           }else{
              Serial.print(" S_r_byte ");
             Serial.print(r_byte, HEX);
             Serial.print(" ");
           }
          #endif 
          
          }
          break;
          
          
          case 1: //do call callback

          if(check_known_slave_byte(r_byte) == 1){ //check if it's an request
            self_Serial.write(r_byte); //send info byte
            #ifdef Com_Test
            Serial.print("S_call_handle ");
            #endif
            recive_status = handle_slave(r_byte); //call handle function
            if(recive_status == 0){ //data not ok
              flush_serial_buffer(); //flush, for a clean restart
              self_Serial.write(error_data_byte); //send error data
              error_num++;
              status = 0;
            }else{
              status = 2; //every thing fine
            }
          }else{  //if not send error
            #ifdef Com_Test
            Serial.print("S_request_er ");
            #endif
            flush_serial_buffer(); //flush, for a clean restart  
            self_Serial.write(error_byte);
            error_num++;
            status = 0;
          }
          break;

          case 2:
            self_Serial.write(acknowledge_byte); ///always send an acknowledge, if the function works fine. If the Slave sends data, it can be dumped by the masters recive function.
            status = 3;
          break;

          case 3: //check acknowledge_byte
          recive_status = wait_for_i_byte(); //wait for acknowledge_byte
          if(recive_status == 0){
          //No conection End
          status = 99;
          }else{
            ack_byte = self_Serial.read();

            #ifdef Com_Test
              Serial.print("S_r_ack: ");
              Serial.print(ack_byte, HEX);
            #endif

            if(check_known_slave_byte(ack_byte) == 1){ //check if it`s an request: sync Problem
              flush_serial_buffer(); //flush, for a clean restart  
              self_Serial.write(error_byte);
              error_num++;
              status = 0; 
            }else{

            ack_byte = ack_byte & (0b00000000 | error_byte | error_data_byte | error_ack_byte | acknowledge_byte); //MASKE get's calculatet by the compiler
            
            if(ack_byte == error_byte){
              error_num++;
              #ifdef Com_Test
              Serial.print("S_c3_er_1 ");
              #endif
              status = 0;
            }else if(ack_byte == error_data_byte){
              error_num++;
              status = 1;
            }else if(ack_byte == error_ack_byte){
              error_num++;
              status = 2;
            }else{ //has to be acknowledge_byte
              #ifdef Com_Test
              Serial.println(F(" S_End "));
              #endif

              trans = 0; //End
              last_Serial_trans_good = 1; //everything fine
              //End
            }
           }
          }
          break;

        case 99:
        trans = 0;
        flush_serial_buffer(); //flush, for a clean restart
        last_Serial_trans_good = 0; //bad ending
        break;
       }

     {//Watchdog
      if(error_num >= 5) status = 99;
     }
     
     }while(trans);

      serial_protocol_interrupt_turn_on(); //call function to turn interrupts on
     }
      #ifdef Com_Test
      else{
      //Serial.print("no_s_request ");
      }
      #endif
    }


    ///////////////////
    //Master Protocol//
    ///////////////////

    
    void Serial_protocol::master_protocol(byte r){

      serial_protocol_interrupt_turn_off(); //call function to turn interrupts off
      
      int status = 0;
      byte i_byte;
      byte ack_byte;
      bool trans = 1;
      bool recive_status = 0;

      int error_num = 0;

      
      
      do{
      switch(status){

        case 0: //check if we can be master
        
        if(self_Serial.available() >= 1){
          slave_protocol(); //first answer if the other one has an request
          }else{
          delayMicroseconds(time_per_byte_micros * 2.0); //check if the other on is sending one right now
          #ifdef Com_Test
          //Serial.print("Call SLAVE ");
          //Serial.println("");
          //Serial.println(Serial1.peek(), HEX);
          #endif
          slave_protocol();
        }
        
        serial_protocol_interrupt_turn_off(); //call function to turn interrupts off 
        status = 1; //always go to 1
        
        break;


        case 1: //send request and data

        //flush_serial_buffer(); //first flush, in case of an unclean communication befor        
        #ifdef Com_Test
        Serial.print("Request_s ");
        #endif
        
        self_Serial.write(r); //Send request byte
        switch(sendtype){
          case 1://///////////        
            for(int i = 0; i < send_data_buff_wp; i++){
              self_Serial.write(send_data_buff[i]);
            }
          break;
          case 2://///////////           
            send_function_serial_protocol();         
          break;         
          default:
          break;
        }
        
        status = 2; //always go to 2

        break;



        case 2: //wait for the slave and check answer

        #ifdef Com_Test
        Serial.print("start_2 ");
        #endif

        recive_status = wait_for_i_byte(); //wait for informaition byte
        if(recive_status == 0){
          //No conection End
          status = 99;
          #ifdef Com_Test
          Serial.print("No_i_byte ");
          #endif
        }else{
          
          i_byte = self_Serial.read(); //get information Byte
          if(i_byte == r){ //informaition byte matches     
            
            if(master_rx_callback){
              recive_status = wait_for_data(); //if we have a callback, wait for the Data
              if(recive_status == 0){
                //No conection End
                status = 99;
                #ifdef Com_Test
                Serial.print("No_data ");
                #endif
              }else{
                if(sendtype == 1 || sendtype == 2){
                #ifdef Com_Test
                Serial.print("Er_peek: ");
                #endif                                       
                ack_byte = self_Serial.peek(); //peek for error
                #ifdef Com_Test
                Serial.print(ack_byte, HEX);
                Serial.print(" ");
                #endif
                  if(ack_byte == error_data_byte){ //sent data is wrong
                    flush_serial_buffer();
                    error_num++;
                    status = 1;
                  }
                }
                if(status == 2){ //only call callback, if no error was detectet befor           
                 recive_status = rx_function_serial_protocol(); //CALLBACK function
                 if(recive_status == 0){ //if the function returns error
                   flush_serial_buffer(); //flush, for a clean restart  
                   self_Serial.write(error_data_byte); //send error
                   error_num++;
                   status = 2; //stay   
                    #ifdef Com_Test
                    Serial.print("Callback_bad ");
                    #endif          
                 }else{
                   status = 3; //next step
                 }
               }
              }
            }else{ //no callback, wait for acknolege
              recive_status = wait_for_i_byte(); //wait for acknolege byte
              if(recive_status == 0){
                //No conection End
                status = 99;
              }else{
                ack_byte = self_Serial.read(); //read it
                if(ack_byte == acknowledge_byte){  
                  status = 3; //all good to go
                }else if(ack_byte == error_data_byte){ //error go back send request and data
                  flush_serial_buffer();
                  status = 1;
                  error_num++; 
                } 
                else{ //unknown
                  flush_serial_buffer(); //flush, for a clean restart
                  error_num++; 
                  self_Serial.write(error_ack_byte); //send error
                  status = 2; //stay
                }
              }
            }
            
          }else if(i_byte == error_byte){
            flush_serial_buffer(); //flush, for a clean restart
            error_num++;
            status = 1; //error_byte go back
            
          }else if(check_known_slave_byte(i_byte) == 1){ //check for double master problem
            #ifdef Com_Test
            Serial.print("d_master ");
            #endif
            if(r > i_byte){
              flush_serial_buffer(); //flush, for a clean restart 
              status = 1; //if the request is higer, we win and send again
              #ifdef Com_Test
              Serial.print("win ");
              #endif
            }else{
              #ifdef Com_Test
              Serial.print("lose ");
              #endif
              recive_status = wait_for_i_byte(); //else, wait and call slave protocol
              if(recive_status == 0){
                //No conection End
                status = 99;
              }else{
                #ifdef Com_Test
                Serial.print(Serial1.peek(), HEX);
                Serial.print(" ");
                #endif
                status = 0;
              }
            }
            
          }else { //unknown send error byte
            flush_serial_buffer(); //flush, for a clean restart  
            self_Serial.write(error_byte);
            error_num++;
            status = 2; //stay
          }

        }

        break;

        case 3: //send acknowledge and end
        #ifdef Com_Test
        Serial.println("ack_send ");
        #endif
        flush_serial_buffer(); //flush, for a clean restart
        self_Serial.write(acknowledge_byte);
        //Set last trans to good
        trans = 0;
        last_Serial_trans_good = 1; //everything fine 
        //End
        break;

        case 99:
        trans = 0;
        flush_serial_buffer(); //flush, for a clean restart
        last_Serial_trans_good = 0; //bad ending
        break;

        
      }
      
    {//Watchdog
      if(error_num >= 5)  status = 99;
    }
      
    }while(trans);

  serial_protocol_interrupt_turn_on(); //call function to turn interrupts on
}



//public

  Serial_protocol::Serial_protocol(COM_LIB_FUN lib_struct, void* request_struct, int sizeof_request_struct, float com_serial_baud, bool (*handle_function_list) (byte), int wait_time_secure_faktor, int max_com_response_time)
  :self_Serial(lib_struct), request_adresse(request_struct), request_size(sizeof_request_struct), self_Serial_baud(com_serial_baud), handle_slave(handle_function_list), self_wait_time_secure_faktor(wait_time_secure_faktor), self_max_com_response_time(max_com_response_time)
  {}

   bool Serial_protocol::wait_for_data()
    {
    
      int available = 0;
      int l_available = 0;
      unsigned long start_t = micros();
      unsigned long timer;
    
      do {
        timer = micros();
    
        l_available = available;
        available = self_Serial.available();
        if (l_available != available) start_t = timer;
    
        if (timer < start_t) start_t = timer; //catch timer overflow
    
      } while (   (available == 0 && (timer - start_t) <= self_max_com_response_time * 1000)  ||  (available > 0 && ((timer - start_t) <= ( time_per_byte_micros * self_wait_time_secure_faktor) ) )     );
    
      if (available == 0){
        return(0);
      }else{
        return(1);
      }
      
    }
  
  void Serial_protocol::flush_serial_buffer()
  {
    while(self_Serial.available() != 0){self_Serial.read();}
  }



  template <typename INFO_TYPE>
  void Serial_protocol::send_value(INFO_TYPE data)
  {
    void* pd = &data;
    for(int i = 0; i < sizeof(data); i++){
      self_Serial.write( *((byte*)(pd + i)) );
    }
    return;
  }

  template <typename INFO_TYPE>
  void Serial_protocol::get_value(INFO_TYPE* save_adress){
    
    for(int i = 0; i < sizeof(*(save_adress)); i++){
      *(((byte*)save_adress) + i) = self_Serial.read();
    }
  }

  
  void Serial_protocol::send_request(byte r){
    sendtype = 0;
    master_rx_callback = 0;
    
    master_protocol(r);
  }

  template <typename INFO_TYPE>
  void Serial_protocol::send_request(byte r, INFO_TYPE data)
  {
    void* pd = &data;
    int i = 0;

    sendtype = 1;
    master_rx_callback = 0;

    for(int i = 0; i < sizeof(data); i++){
      send_data_buff[i] = *((byte*)(pd + i));
    }
    send_data_buff_wp = sizeof(data);


    master_protocol(r);
  }

  void Serial_protocol::send_request_with_callback_f(byte r, void (*callback) ()){
    
    sendtype = 2;
    master_rx_callback = 0;
    
    send_function_serial_protocol = callback;

    master_protocol(r);
    
  }



  void Serial_protocol::send_request(byte r, bool (*rx_callback)()){
    sendtype = 0;
    master_rx_callback = 1;
    
    rx_function_serial_protocol = rx_callback;
    
    master_protocol(r);
  }

  template <typename INFO_TYPE>
  void Serial_protocol::send_request(byte r, INFO_TYPE data, bool (*rx_callback)())
  {
    void* pd = &data;
    int i = 0;

    sendtype = 1;
    master_rx_callback = 1;

    for(int i = 0; i < sizeof(data); i++){
      send_data_buff[i] = *((byte*)(pd + i));
    }
    send_data_buff_wp = sizeof(data);

    rx_function_serial_protocol = rx_callback;

    master_protocol(r);
  }

  void Serial_protocol::send_request_with_callback_f(byte r, void (*callback) (), bool (*rx_callback)()){
    
    sendtype = 2;
    master_rx_callback = 1;
    
    send_function_serial_protocol = callback;
    rx_function_serial_protocol = rx_callback;

    master_protocol(r);
    
  }


  void Serial_protocol::call_slave_protocol(){
    slave_protocol();
  }
