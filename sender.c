#include "fr_util.h"
#include "tx_array.h"


//***********************************************
// Send_bit: Function executed to send each bit.
//***********************************************

/*
 * Sync at the beginning of a window of length config->sync_time_mask.
 * Then, for a clock length of config->tx_interval,
 * - Send a bit 1 to the receiver by repeatedly accessing an address.
 * - Send a bit 0 by doing nothing
 */

void send_bit(bool one, struct config *config){
  ADDR_PTR addr = config->addr;
  uint64_t interval = config->tx_interval;

  // Synchronize with receiver
  CYCLES start_t = cc_sync(config->sync_time_mask, config->sync_jitter);

  
  //TODO:
  // If 1: Load repeatedly (using maccess()) for the length of the config->tx_interval.
  // If 0: Do nothing for the length of the config->tx_interval.
  while(rdtscp() - start_t < config->tx_interval){
    if(one){
      maccess_t(config->addr);
    }
    else{
      
    }
  }
}

//---------------------------------------------------------------------------


int main(int argc, char **argv)
{
    // Initialize config and local variables
    struct config config;
    init_config(&config, argc, argv);
    int sending = 1;

    puts("-------------------");
    puts("Flush Reload Covert Channel Sender CS7292 Asst-2 v1.0");
    puts("-------------------\n");   
    printf("sync_time_mask : %#lx cycles // Sync time window for each bit\n",config.sync_time_mask);
    printf("tx_interval    : %#lx cycles // Interval for tranmission within each sync time window\n",config.tx_interval);
   
    //Sleep a bit, to ensure receiver is ready.
    usleep(1000*100);

    //-------------------------
    // Start Sending Bits
    //-------------------------

    while (sending) {
      // DEBUG: Send infinite constant or alternating values.
      /* for (int i = 0; i < 100000000; i++) { */
      /* send_bit(true, &config); */
      // OR
      /* send_bit(i % 2 == 0, &config); */
      /* } */
      
      // Get a message to send from the user
      char text_buf[128] ;
      strncpy(text_buf, tx_string, strlen(tx_string)+1);
      // Convert that message to binary
      char *msg = string_to_binary(text_buf);
        
      // Send '10101011' bit sequence tell the receiver
      // a message is going to be sent
      for (int i = 0; i < 6; i++) {
        send_bit(i % 2 == 0, &config);
      }
      send_bit(true, &config);
      send_bit(true, &config);

      //-------------------------
      // Start Sending Payload
      //-------------------------

      // Send the message bit by bit
      size_t msg_len = strlen(msg);
      for (int ind = 0; ind < msg_len; ind++) {
	send_bit(msg[ind] - '0', &config); 
      }

      printf("%s\n",text_buf);
      sending = 0;
    }
 
    printf("Sender finished\n");
    return 0;
}











