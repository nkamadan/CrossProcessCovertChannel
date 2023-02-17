#include "fr_util.h"
#include "tx_array.h"

//********************
// SET FOR YOUR SYSTEM
//********************
int CACHE_MISS_LATENCY = 154;  //218 - 64 = 154
int CPU_FREQ_MHZ       = 4000;
//--------------------


//Bit string received/transmitted.
char rx_bit_string[1500];
char* tx_bit_string;
int rx_bits_received;

//---------------------------------------------------------------------------
// Debug variables below can be useful to
// copy values of the program (e.g. cycle-count, bit values)
// as the program executes. Print these variables at the end to
// understand what these values were, without affecting program's execition.

/* #define DEBUG_LEN (1000) */
/* uint64_t debug_array[DEBUG_LEN] ={0}; */
/* int debug_entry = 0; */
//---------------------------------------------------------------------------


//***************************************************
// Detect_bit(): Function executed to receive each bit.
//***************************************************

/*
 * Sync at the beginning of a window of length config->sync_time_mask.
 * Then, Detect a bit by repeatedly measuring the access time of the load from config->addr,
 * and counting the number of misses for the clock length of config->interval,
 * - Detect a bit 1 if misses <= hits
 * - Detect a bit 0 otherwise.
 * Then, flush the address to get ready for the next bit.
 */
bool detect_bit(struct config *config)
{
  ADDR_PTR addr = config->addr;
  uint64_t interval = config->tx_interval;

  int misses = 0;
  int hits = 0;
  bool detected_bit;
  
  // Synchronize with sender
  CYCLES start_t = cc_sync(config->sync_time_mask, config->sync_jitter);

  //DEBUG /* debug_array[(debug_entry++)%DEBUG_LEN] = start_t; */

  //TODO:
  // For the length of config->tx_interval, perform :
  while(rdtscp() - start_t < config->tx_interval){
    //  -- Load shared addr and measure latency (access_time)
    CYCLES access_time = maccess_t(config->addr);
    //  -- Flush the shared addr.
    clflush(config->addr);
    //  -- Wait about 1000 cycles (to allow sender a chance to install the line),
    //    or till end of config->tx_interval, whichever is earlier.
    for(int x=0; x<900; x++){
      asm volatile ("nop"); //Also accounting the for loop management
    }
    //  -- If access_time is more than 1000 cycles, ignore (noise).
    //  -- If access_time is greater than CACHE_MISS_LATENCY,
    //    increment misses, otherwise increment hits.
    if(access_time >= CACHE_MISS_LATENCY){ //miss
      if(access_time < 1000){ //Not noise
          misses++;
      }
    }
    else { //hit
      hits++;
    }
  }
  // Decide if sender sent 1 or 0 (based on whether more hits or misses in interval)
  detected_bit = (hits > misses) ? 1 : 0;
  // DEBUG: /* debug_array[(debug_entry++)%DEBUG_LEN] = detected_bit;  */

  return detected_bit;
}
//---------------------------------------------------------------------------


int main(int argc, char **argv)
{
  // Initialize config and local variables
  struct config config;
  init_config(&config, argc, argv);
  char msg_ch[MAX_BUFFER_LEN + 1];
  int flip_sequence = 4;
  bool current;
  bool previous = true;

  uint64_t start_time = 0, end_time = 0;

  puts("-------------------");
  puts("Flush Reload Covert Channel Receiver CS7292 Asst-2 v1.0");
  puts("-------------------\n");
  printf("sync_time_mask : %#lx cycles // Sync time window for each bit\n",config.sync_time_mask);
  printf("tx_interval    : %#lx cycles // Interval for tranmission within each sync time window\n",config.tx_interval);

  printf("Listening...\n");
  fflush(stdout);

  bool is_comm = false;
  int binary_msg_len = 0;

  //---------------------------------------------------------------------------
  // Start Receiving Bits
  //---------------------------------------------------------------------------

  while (1) {
    current = detect_bit(&config);

    //DEBUG:  /* printf("Bit %d \n",current); */
    
    // Detect the sequence '101011' that indicates sender is sending a message    
    if (flip_sequence == 0 && current == 1 && previous == 1) {
      // Got 1010 (flip_sequence==0) and then 11
      int binary_msg_len = 0;
      int strike_zeros = 0;

      //-----------------------------------
      // Start Detecting Payload from Sender
      //-----------------------------------

      start_time = rdtscp();      
	    
      for (int i = 0; i < MAX_BUFFER_LEN; i++) {
        binary_msg_len++;

        if (current = detect_bit(&config)) {
          msg_ch[i] = '1';
          strike_zeros = 0;
        } else {
          msg_ch[i] = '0';
	  
          if (++strike_zeros >= 8 && i % 8 == 0) {
	    //Received \0 character. End of Payload.
            break;
          }	  
        }
	//DEBUG: /* printf("Bit %d \n",current); */
      }

      //-----------------------------------
      // End Communication
      //-----------------------------------      
      msg_ch[binary_msg_len - 8] = '\0';
      end_time = rdtscp();
      
      // Print out message
      int ascii_msg_len = binary_msg_len / 8;
      char msg[ascii_msg_len];
      puts("\n-------------------");
      printf("Rx> %s\n", conv_char(msg_ch, ascii_msg_len, msg));

      //Terminate channel
      strncpy(rx_bit_string,msg_ch,ascii_msg_len*8 - 8 );
      rx_bits_received = ascii_msg_len*8 -8;
      break;    

    } else if (flip_sequence > 0 && current != previous) {
      // alternating bit pattern, but not yet four flips (flip_sequence > 0)
      flip_sequence--;

    } else if (current == previous) {
      // Not a alternating sequence: reset flip_sequence to 4 (need 4 flips from sender) 
      flip_sequence = 4;
    }
    
    previous = current;
  }

  //-----------------------------------
  // Final Analysis
  //-----------------------------------      
  
  //Convert Tx_String to Bit-String  
  tx_bit_string = string_to_binary(tx_string);

  //Print Received Bits
  printf("rx bit-string : ");
  for(int i=0; i< rx_bits_received; i++){
    if(rx_bit_string[i] != tx_bit_string[i])      
      printf(RED "%c" RESET,rx_bit_string[i]);
    else
      printf(GREEN "%c" RESET,rx_bit_string[i]);
  }
  printf("\n");

  //Print Sent String and bits.
  printf("Tx> %s\n", tx_string);
  printf("tx bit-string : %s\n", tx_bit_string);
 
  //Analyze Errors
  int num_correct=0;
  int tot=0;
  for(int i=0; i< rx_bits_received; i++){
    if(rx_bit_string[i] == tx_bit_string[i]) 
      num_correct++;
    tot++;        
  }

  //Analyze Bit-Rate
  double tx_time_usec = (end_time - start_time) / CPU_FREQ_MHZ ;
  double bits_per_sec = tot * 1000000.0 / tx_time_usec;
  
  puts("\n-------------------");
  printf("Receiver finished\n");
  printf("Correct Bits-Percentage= %.4f%% (%d/%d)\n",100.0*num_correct/tot,num_correct,tot);
  printf("Transmission Bit-rate= %.2f Bytes/Sec\n",bits_per_sec/8);
  puts("-------------------\n");


  //PRINT DEBUG VARIABLES
  /* for(int i=0;i<DEBUG_LEN;i++){ */
  /*   //cycles */
  /*   /\* if(debug_array[i] && i) *\/ */
  /*   /\*   printf("DEBUG_ARRAY[%d]: %ld\n",i,debug_array[i]-debug_array[i-1]); *\/ */

  /*   //bit values */
  /*   /\* printf("DEBUG_ARRAY[%d]: %ld\n",i,debug_array[i]); *\/ */
  /* } */

  
  return 0;
}


