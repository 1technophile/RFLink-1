//#######################################################################################################
//##                    This Plugin is only for use with the RFLink software package                   ##
//##                                        Plugin-13: Powerfix RCB-i 3600                             ##
//#######################################################################################################
/*********************************************************************************************\
 * This plugin takes care of sending and receiving the Powerfix RCB-i 3600 protocol
 * Works with: Powerfix RCB-i 3600 - 4 power outlets and a remote, Quigg GT7000
 * 
 * Author             : StuntTeam
 * Support            : http://sourceforge.net/projects/rflink/
 * License            : This code is free for use in any open source project when this header is included.
 *                      Usage of any parts of this code in a commercial application is prohibited!
 ***********************************************************************************************
 * Technical information:
 * Partially based on http://wiki.pilight.org/doku.php/quigg_switch_gt_7000_v7_0
 *
 * The Powerfix RF packets are 42 pulses long resulting in 20 bits data packets
 *
 * 0000 1000 0000 00 0 1 0 0 0 1 
 * AAAA AAAA AAAA BB C D E F G H
 *
 * A = 12 bits address
 * B = 2 bits Unit code     
 * C = Group Command (to all devices)
 * D = State (ON/OFF/DIM UP/DIM DOWN)
 * E = Dim command (1=dim/bright command)
 * F = always 0
 * G = unknown?
 * H = parity calculated over all bits
 *
 * 000010000000 11 111010  dim
 * 000010000000 11 101011  bright
 * 000010000000 11 100001  all off
 * 000010000000 11 110000  all on
 * 000010000000 00 010001  1 on 
 * 000010000000 00 000000  1 off 
 * 000010000000 10 010011  2 on
 * 000010000000 10 000010  2 off
 * 000010000000 01 010000  3 on
 * 000010000000 01 000001  3 off
 * 000010000000 11 010010  4 on
 * 000010000000 11 000011  4 off
 *
 * Sample:
 *  
 * BUTTON: DIM
 * 20;05;DEBUG;Pulses=42;Pulses(uSec)=600,600,1250,625,1250,625,1225,625,1250,1275,575,600,1250,625,1225,625,1250,625,1225,625,1225,625,1225,625,1225,1300,575,1300,575,1300,575,1300,600,1300,550,625,1225,1300,550,625,1175;
 * 00101010110010101010101011010101010011001
 * BUTTON: BRIGHT
 * 20;12;DEBUG;Pulses=42;Pulses(uSec)=600,600,1250,600,1250,600,1250,600,1250,1275,575,625,1225,625,1225,625,1250,625,1225,625,1225,625,1225,625,1250,1300,550,1300,575,1300,550,625,1250,1300,550,625,1225,1300,575,1300,525;
 * 00101010110010101010101011010100110011010 
 * BUTTON: ALL OFF
 * 20;18;DEBUG;Pulses=42;Pulses(uSec)=600,600,1250,625,1225,625,1225,625,1250,1275,575,625,1225,625,1225,625,1250,625,1225,625,1225,625,1225,625,1225,1300,575,1300,550,1300,550,625,1250,650,1200,625,1225,650,1225,1300,500;
 * 00101010110010101010101011010100101010110
 * BUTTON: ALL ON
 * 20;15;DEBUG;Pulses=42;Pulses(uSec)=600,600,1250,600,1250,625,1225,625,1225,1275,575,625,1225,625,1225,625,1250,625,1225,650,1200,625,1225,625,1225,1300,550,1300,575,1300,550,1300,575,650,1225,625,1225,625,1225,625,1175;
 * 00101010110010101010101011010101001010101
 * BUTTON: 1 ON
 * 20;04;DEBUG;Pulses=42;Pulses(uSec)=600,600,1250,625,1225,625,1225,625,1225,1300,575,625,1225,625,1225,625,1250,625,1225,625,1225,625,1225,625,1225,625,1225,625,1225,625,1225,1300,575,625,1225,625,1225,625,1225,1300,500;
 * 0101010110010101010101010101011001010110
 * BUTTON: 1 OFF
 * 20;07;DEBUG;Pulses=42;Pulses(uSec)=600, 600,1250,600,1250,625,1250,625,1225,1275,575,625,1225,600,1250,625,1275,625,1225,625,1225,625,1225,625,1225,625,1225,625,1225,625,1225,625,1250,625,1225,625,1225,625,1225,625,1175;
 * 0101010110010101010101010101010101010101
 * BUTTON: 2 ON
 * 20;0A;DEBUG;Pulses=42;Pulses(uSec)=575,600,1250,625,1225,625,1225,625,1225,1300,550,625,1225,625,1225,625,1275,625,1225,625,1225,625,1225,625,1225,1300,550,625,1225,625,1225,1300,575,625,1225,625,1225,1300,575,1300,500;
 * 0101010110010101010101011001011001011010
 * BUTTON: 2 OFF
 * 20;0D;DEBUG;Pulses=42;Pulses(uSec)=600,600,1250,625,1225,625,1225,625,1250,1300,550,625,1225,625,1225,625,1250,625,1225,625,1225,625,1225,625,1250,1300,575,625,1225,625,1225,625,1250,625,1225,650,1225,1300,550,625,1175;
 * 0101010110010101010101011001010101011001
 \*********************************************************************************************/
#define POWERFIX_PulseLength    42
#define POWEFIX_PULSEMID  900/RAWSIGNAL_SAMPLE_RATE

#ifdef PLUGIN_013
boolean Plugin_013(byte function, char *string) {
   if (RawSignal.Number!= POWERFIX_PulseLength) return false; 
      unsigned long bitstream=0L;
      unsigned int address=0;
      byte unitcode=0;
      byte button=0;
      byte command=0;
      byte parity=1;
      // ==========================================================================
      if (RawSignal.Pulses[1] > POWEFIX_PULSEMID) return false; // start pulse is short
      for (byte x=2;x <=POWERFIX_PulseLength-2;x+=2) {  
          if (RawSignal.Pulses[x] > POWEFIX_PULSEMID) {
             if (RawSignal.Pulses[x+1] > POWEFIX_PULSEMID) return false; // pulse sequence check 01/10
             bitstream = (bitstream << 1) | 0x1;
             parity=parity^1;
          } else {
             if (RawSignal.Pulses[x+1] < POWEFIX_PULSEMID) return false; // pulse sequence check 01/10
             bitstream = (bitstream << 1); 
          }
      }
      //==================================================================================
      // Perform Sanity Checks
      if (bitstream==0) return false;               // No bits detected? 
      if (parity != 0) return false;                // Parity check
      if (((bitstream)&0x4) == 4) return false;     // Tested bit should always be zero
      //==================================================================================
      // Sort data
      address=((bitstream)>>8);                     // 12 bits address
      unitcode=((bitstream >> 6)& 0x03);
      if (unitcode==2) button=1;
      if (unitcode==1) button=2;
      if (unitcode==3) button=3;
 
      parity=((bitstream)&0x3f);                    // re-use parity variable
      command=((parity)>>4)&0x01;                   // On/Off command
      if (((parity)&0x08) == 0x08) {                // dim command
         command=command+2;
      } else {
        if (((parity)&0x20) == 0x20) {              // group command
           command=command+4;
        }
      }
      //==================================================================================
      // ----------------------------------
      // Output
      // ----------------------------------
      sprintf(pbuffer, "20;%02X;", PKSequenceNumber++); // Node and packet number 
      Serial.print( pbuffer );
      // ----------------------------------
      Serial.print(F("Powerfix;"));                           // Label
      sprintf(pbuffer, "ID=%04x;", address); // ID    
      Serial.print( pbuffer );
      sprintf(pbuffer, "SWITCH=%02x;", button); // ID    
      Serial.print( pbuffer );
      Serial.print(F("CMD="));                    

      if ( command == 0) {
         Serial.print(F("OFF;"));
      } else 
      if ( command == 1) {
         Serial.print(F("ON;"));
      } else 
      if ( command == 2) {
         Serial.print(F("BRIGHT;"));
      } else 
      if ( command == 3) {
         Serial.print(F("DIM;"));
      } else 
      if ( command == 4) {
         Serial.print(F("ALLOFF;"));
      } else 
      if ( command == 5) {
         Serial.print(F("ALLON;"));
      }
      Serial.println();
      // ----------------------------------
      RawSignal.Repeats=true;                    // suppress repeats of the same RF packet         
      RawSignal.Number=0;
      return true;
}
#endif //PLUGIN_013
   
#ifdef PLUGIN_TX_013
// NOTE: Send does not work yet
void Powerfix_Send(unsigned long bitstream);

boolean PluginTX_013(byte function, char *string) {
        boolean success=false;
        //10;POWERF;01b523;3;ON;
        //10;POWERF;000080;0;ON;
        //012345678901234567890123
        if (strncasecmp(InputBuffer_Serial+3,"POWERF;",7) == 0) {
           if (InputBuffer_Serial[16] != ';') return success;
          
           InputBuffer_Serial[8]=0x30;
           InputBuffer_Serial[9]=0x78;              
           InputBuffer_Serial[16]=0x00;             // Get address from hexadecimal value 

           unsigned long bitstream=0L;              // Main placeholder
           byte command=0;
           byte c;
           // -------------------------------
           bitstream=str2int(InputBuffer_Serial+8); // Address, first 12 bits of the 20 bits in total
           
           bitstream=(bitstream)<<8;                // shift left so that we can add the 8 command bits
           // -------------------------------
           byte temp=str2int(InputBuffer_Serial+17);// button/unit number (0..3)
           if (temp==1) command=0x82;
           if (temp==2) command=0x40;
           if (temp==3) command=0xc2;
           // -------------------------------
           c=0;
           c = str2cmd(InputBuffer_Serial+19);      // ON/OFF command
           if (c == VALUE_ON) { 
              command=command | 0x10;               // turn "on" bit for on command 
           } else {
              if (c == VALUE_ALLOFF) {              // set "all off" bits
                 command=0xe0;
              } else
              if (c == VALUE_ALLON) { 
                 command=0xf0;                      // set "all on" bits
              } 
           } 
           // not supported yet..
           // dim: command=0xfa
           // bright: command=0xea;
           // -------------------------------
           bitstream=bitstream+command;
           // -------------------------------
           Powerfix_Send(bitstream);                // bitstream to send
           success=true;
        }
        return success;
}

#define PLUGIN_013_RFLOW        650
#define PLUGIN_013_RFHIGH       1300

void Powerfix_Send(unsigned long bitstream) { 
     RawSignal.Repeats=7;                           // Number of RF packet retransmits
     RawSignal.Delay=20;                            // Delay between RF packets
     RawSignal.Number=42;                           // Length

     uint32_t fdatabit;
     uint32_t fdatamask = 0x80000;
     byte parity=1;                                 // to calculate the parity bit
     // -------------------------------
     RawSignal.Pulses[1]=PLUGIN_013_RFLOW/RawSignal.Multiply; // start pulse
     for (byte i=2; i<40; i=i+2) {                  // address and command bits
         fdatabit = bitstream & fdatamask;          // Get most left bit
         bitstream = (bitstream << 1);              // Shift left

         if (fdatabit != fdatamask) {               // Write 0
            RawSignal.Pulses[i]  = PLUGIN_013_RFLOW/RawSignal.Multiply;
            RawSignal.Pulses[i+1]= PLUGIN_013_RFHIGH/RawSignal.Multiply;
         } else {                                   // Write 1
            parity=parity^1;
            RawSignal.Pulses[i]  = PLUGIN_013_RFHIGH/RawSignal.Multiply;
            RawSignal.Pulses[i+1]= PLUGIN_013_RFLOW/RawSignal.Multiply;
         }  
     }
     // parity
     if (parity == 0) {                             // Write 0
        RawSignal.Pulses[40] = PLUGIN_013_RFLOW/RawSignal.Multiply;
        RawSignal.Pulses[41] = PLUGIN_013_RFHIGH/RawSignal.Multiply;
     } else {                                       // Write 1
        RawSignal.Pulses[40] = PLUGIN_013_RFHIGH/RawSignal.Multiply;
        RawSignal.Pulses[41] = PLUGIN_013_RFLOW/RawSignal.Multiply;
     }  
     RawSendRF();
}
#endif // PLUGIN_TX_013
