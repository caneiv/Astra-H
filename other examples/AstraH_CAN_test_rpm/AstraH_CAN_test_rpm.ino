#include <HardwareCAN.h>
/*
 * Uses STM32duino with Phono patch. Must add 33 and 95 CAN speeds
 */
#define BPIN 0 // there is a buttton
#define SPIN 1 // there is a switch
#define BUZZERPIN 17 // B1 for buzzer

// Instanciation of CAN interface
HardwareCAN canBus(CAN1_BASE);
CanMsg msg ;
CanMsg *r_msg;

void CANSetup(void)
{
  CAN_STATUS Stat ;

  // Initialize CAN module
  canBus.map(CAN_GPIO_PB8_PB9);       // This setting is already wired in the Olimexino-STM32 board
//  Stat = canBus.begin(CAN_SPEED_33, CAN_MODE_NORMAL);    
  Stat = canBus.begin(CAN_SPEED_95, CAN_MODE_NORMAL);    

  canBus.filter(0, 0, 0);
  canBus.set_irq_mode();              // Use irq mode (recommended), so the handling of incoming messages
                                      // will be performed at ease in a task or in the loop. The software fifo is 16 cells long, 
                                      // allowing at least 15 ms before processing the fifo is needed at 125 kbps
  Stat = canBus.status();
  if (Stat != CAN_OK)  
     {/* Your own error processing here */ ;   // Initialization failed
     Serial.print("Initialization failed");
     Serial1.print("Initialization failed");
     }
}


// Send one frame. Parameter is a pointer to a frame structure (above), that has previously been updated with data.
// If no mailbox is available, wait until one becomes empty. There are 3 mailboxes.
CAN_TX_MBX CANsend(CanMsg *pmsg) // Should be moved to the library?!
{
  CAN_TX_MBX mbx;
  do 
  {
    mbx = canBus.send(pmsg) ;
#ifdef USE_MULTITASK
    vTaskDelay( 1 ) ;                 // Infinite loops are not multitasking-friendly
#endif
  }
  while(mbx == CAN_TX_NO_MBX) ;       // Waiting outbound frames will eventually be sent, unless there is a CAN bus failure.
  return mbx ;
}

// Send message
// Prepare and send a frame containing some value 
void SendCANmessage(long id=0x001, byte dlength=8, byte d0=0x00, byte d1=0x00, byte d2=0x00, byte d3=0x00, byte d4=0x00, byte d5=0x00, byte d6=0x00, byte d7=0x00)
{
  // Initialize the message structure
  // A CAN structure includes the following fields:
  msg.IDE = CAN_ID_STD;          // Indicates a standard identifier ; CAN_ID_EXT would mean this frame uses an extended identifier
  msg.RTR = CAN_RTR_DATA;        // Indicated this is a data frame, as opposed to a remote frame (would then be CAN_RTR_REMOTE)
  msg.ID = id ;                  // Identifier of the frame : 0-2047 (0-0x3ff) for standard idenfiers; 0-0x1fffffff for extended identifiers
  msg.DLC = dlength;                   // Number of data bytes to follow

  // Prepare frame : send something
  msg.Data[0] = d0 ;
  msg.Data[1] = d1 ;
  msg.Data[2] = d2 ;
  msg.Data[3] = d3 ;
  msg.Data[4] = d4 ;
  msg.Data[5] = d5 ;
  msg.Data[6] = d6 ;
  msg.Data[7] = d7 ;

  digitalWrite(PC13, LOW);    // turn the onboard LED on
  CANsend(&msg) ;      // Send this frame
  delay(18);              
  digitalWrite(PC13, HIGH);   // turn the LED off 
  delay(10);  
}

// The application program starts here
bool bState = 0;         // variable for reading the pushbutton status
bool sState = 0;         // variable for reading the switch status
// byte st = 0x31; // buttot 1 on the CD30MP3

void setup() {
  // put your setup code here, to run once:
  pinMode(PC13, OUTPUT); // LED
  digitalWrite(PC13, HIGH);
  pinMode(BPIN, INPUT); // input for hardware button
  pinMode(SPIN, INPUT); // input for hardware switch

  Serial.begin(115200);
  Serial.println("Hello World!");
  Serial.println("Output to \"Serial\"");
  Serial.println("Starting CAN-test-rpm program");
  Serial.println("downloaded via serial");
  Serial.println("Initialize the CAN module ...");

  pinMode(BUZZERPIN, OUTPUT);
  digitalWrite(BUZZERPIN, LOW);
  
  Serial1.begin(115200);
  Serial1.println("Hello World!");
  Serial1.println("Output to \"Serial1\"");
  Serial1.println("Starting CAN-test-rpm program");
  Serial1.println("downloaded via serial");
  Serial1.println("Initialize the CAN module ...");

  CANSetup() ;        // Initialize the CAN module and prepare the message structures.
     Serial.println("Initialization OK");
     Serial1.println("Initialization OK");

}

void beep(int times)
{
  for (int i=0;i<times;++i)
  {
      digitalWrite(BUZZERPIN, HIGH);
      digitalWrite(PC13, LOW);    // turn the onboard LED on
      delay(50);
      digitalWrite(BUZZERPIN, LOW);
      digitalWrite(PC13, HIGH);   // turn the LED off 
      delay(100);
  } 
  delay(200); // already has a delay, so mind it
}

void loop() {
  bState = digitalRead(BPIN);
  sState = digitalRead(SPIN);
  
  // check if the pushbutton is pressed.
  // if it is, the buttonState is HIGH:
  if (bState == HIGH) {
  long msgID = 0x201 ;
  SendCANmessage(msgID, 3, 0x01, 0x6f, 0x00) ;       
  SendCANmessage(msgID, 3, 0x00, 0x6f, 0x00) ;       
  Serial.println("OK pressed");
  Serial1.println("OK pressed");
//  beep(3);
  }
  
  // check if the switch is high.
  // if it is:
  if (sState == HIGH) {
    /* --- Start check RPMs --- */
    bool gotit = 0; 
    while ( ( ( r_msg = canBus.recv() ) != NULL ) and (!gotit)  )
    {
      //switch (r_msg->ID) {
      // case 0x108:
      // do smth;
      // break;
      if ((r_msg->ID) == 0x108 ) {
      Serial1.println("Data read at 0x108");
      byte tt = r_msg->Data[0];
      byte xx = r_msg->Data[1];
      byte yy = r_msg->Data[2];
      int rpm = (xx<<6) + (yy>>2);
      Serial1.print("rpm = ");
      Serial.print("rpm = ");
      Serial1.println(rpm);
      Serial.println(rpm);
        if (tt&0x20){ /* Throttle on? */
          Serial1.println("Throttle ON");
          if (rpm > 3100) {beep(2); Serial1.println("Shift up!");}
          if (rpm < 1700) {beep(1); Serial1.println("Shift down!");}
          //if (xx > 0x2e) {beep(2); Serial1.println("Shift up!");}
          //if (xx < 0x1c) {beep(1); Serial1.println("Shift down!");}
        }
      gotit = 1;
      }
    canBus.free();
	}     
    /* --- End check RPMs --- */
  } 
  else 
  {
  // try to read message and output to serial
  CanMsg *r_msg;
  if ((r_msg = canBus.recv()) != NULL)
	  {
  Serial1.print(r_msg->ID, HEX);
  Serial1.print(" # ");
  Serial1.print(r_msg->Data[0], HEX);
  Serial1.print(" ");
  Serial1.print(r_msg->Data[1], HEX);
  Serial1.print(" ");
  Serial1.print(r_msg->Data[2], HEX);
  Serial1.print(" ");
  Serial1.print(r_msg->Data[3], HEX);
  Serial1.print(" ");
  Serial1.print(r_msg->Data[4], HEX);
  Serial1.print(" ");
  Serial1.print(r_msg->Data[5], HEX);
  Serial1.print(" ");
  Serial1.print(r_msg->Data[6], HEX);
  Serial1.print(" ");
  Serial1.println(r_msg->Data[7], HEX);
 
    }

  canBus.free();
//  delay(300);
  }
  

  
}
