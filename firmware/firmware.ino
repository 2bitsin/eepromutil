/*
 * PORTD: 21, 20, 19, 18, --, --, --, 38 
 * PORTE: 00, 01, --, 05, 02, 03, --, -- 
 * PORTH: 17, 16, --, 06, 07, 08, 09, -- 
 * PORTI: --, --, --, --, --, --, --, -- 
 * PORTC: 37, 36, 35, 34, 33, 32, 31, 30
 * PORTA: 22, 23, 24, 25, 26, 27, 28, 29
 * 
 * #ADDRESS (50 - #CE)
 * PORTF: A0, A1, A2, A3, A4, A5, A6, A7 
 * PORTK: A8, A9, AA, AB, AC, AD, AE, AF
 * PORTB: 53, 52, 51, 50, / 10, 11, 12, 13  /
 * 
 * #DATA
 * PORTL: 49, 48, 47, 46, 45, 44, 43, 42
 * 
 * #OE, #WE
 * PORTG: 41, 40, 39, --, --, --, --, -- 
 * 
 * 
 */

#include "protocol.hpp"
#include "crc32.hpp"

#define DDR_ADDR_L   DDRA
#define DDR_ADDR_M   DDRC
#define DDR_ADDR_H   DDRG

#define DDR_DATA_L   DDRL

#define PORT_ADDR_L  PORTA
#define PORT_ADDR_M  PORTC
#define PORT_ADDR_H  PORTG

#define PORT_DATA_L  PORTL
#define PINP_DATA_L  PINL

///////////////////////////////////

#define DDR_WE       DDRD
#define PORT_WE      PORTD
#define MASK_WE      0x80u

#define DDR_OE       DDRB
#define PORT_OE      PORTB
#define MASK_OE      0x01u

//////////////////////////////

#define DATA_DIR_OUT    1
#define DATA_DIR_INP    0

#define WAIT_SINGLE() asm volatile ("nop")

#define WAIT_TCE() delay(100)
#define WAIT_TSE() delay(25)
#define WAIT_BYP() delayMicroseconds(20)

//////////////////////////////////


void programmer_init()
{
  /* ADDRESS# + CE# */
  DDR_ADDR_L = 0xffu;
  DDR_ADDR_M = 0xffu;
  DDR_ADDR_H = 0x07u;

  /* DATA# */
  DDR_DATA_L = 0xffu;

  /* OE# , WE# */
  DDR_WE = MASK_WE;
  DDR_OE = MASK_OE;
   
  PORT_WE |= MASK_WE;
  PORT_OE |= MASK_OE;
}

void set_addr(dword addr)
{
  addr &= 0x7FFFFu;
  PORT_ADDR_L = ((addr >>  0u) & 0xffu);
  PORT_ADDR_M = ((addr >>  8u) & 0xffu);
  PORT_ADDR_H = ((addr >> 16u) & 0x07u);
}

void out_data(byte data)
{
  DDR_DATA_L = 0xFFu;
  WAIT_SINGLE();
  WAIT_SINGLE();
  PORT_DATA_L = data;
}

byte inp_data()
{
  DDR_DATA_L = 0x00u;
  PORT_DATA_L = 0x00u;
  WAIT_SINGLE();
  WAIT_SINGLE();
  return PINP_DATA_L;
}

void set_we(byte b)
{
  if (!b) PORT_WE |= MASK_WE;
  else PORT_WE &= ~MASK_WE;
}

void set_oe(byte b)
{
  if (!b) PORT_OE |= MASK_OE;
  else PORT_OE &= ~MASK_OE;
}

void write_cycle(dword addr, byte data)
{
  set_oe(0);
  set_we(0);
  WAIT_SINGLE(); 
  set_addr(addr);
  set_we(1);
  WAIT_SINGLE();  
  out_data(data);
  set_we(0);  
  WAIT_SINGLE();
}

byte read_cycle(dword addr)
{  
  set_we(0);
  set_oe(0);
  WAIT_SINGLE();
  set_addr(addr);
  set_oe(1);
  WAIT_SINGLE();  
  byte r = inp_data();
  set_oe(0);
  WAIT_SINGLE();  
  return r;  
}

void enter_softid_mode()
{
  write_cycle(0x5555u, 0xAAu);
  write_cycle(0x2AAAu, 0x55u);
  write_cycle(0x5555u, 0x90u);
  delay(1);
}

void exit_softid_mode()
{
  write_cycle(0x5555u, 0xAAu);
  write_cycle(0x2AAAu, 0x55u);
  write_cycle(0x5555u, 0xf0u);
  delay(1);
}

word read_soft_id()
{
  enter_softid_mode();
  word a = read_cycle(0x0000u);  
  word b = read_cycle(0x0001u);  
  exit_softid_mode();
  return a*0x100u + b;
}

void chip_erase()
{
  write_cycle(0x5555u, 0xAAu);
  write_cycle(0x2AAAu, 0x55u);
  write_cycle(0x5555u, 0x80u);
  write_cycle(0x5555u, 0xAAu);
  write_cycle(0x2AAAu, 0x55u);
  write_cycle(0x5555u, 0x10u);
  WAIT_TCE();
}

void sector_erase(dword seca)
{
  seca <<= 12;
  write_cycle(0x5555u, 0xAAu);
  write_cycle(0x2AAAu, 0x55u);
  write_cycle(0x5555u, 0x80u);
  write_cycle(0x5555u, 0xAAu);
  write_cycle(0x2AAAu, 0x55u);
  write_cycle(seca,    0x30u);
  WAIT_TSE();  
}

void sector_read(byte (&buff)[BUFFER_SIZE], dword addr)
{
  memset(buff, 0, BUFFER_SIZE);
  for(dword a = 0; a < BUFFER_SIZE; ++a)
    buff[a] = read_cycle(addr+a);
}

void program_byte(dword addr, byte data)
{
  write_cycle(0x5555u, 0xAAu);
  write_cycle(0x2AAAu, 0x55u);
  write_cycle(0x5555u, 0xA0u);  
  write_cycle(addr, data);  
  WAIT_BYP();
}

void sector_program(const byte (&buff)[BUFFER_SIZE], dword addr)
{
  for(dword a = 0; a < BUFFER_SIZE; ++a)
    program_byte(addr+a, buff[a]);
}

enum cmd_state_type
{
  CMD_STATE_IDLE,
  CMD_STATE_SYNCED
};


static byte           sec_buff[BUFFER_SIZE] = {0};
static dword          cmd = {0};
static cmd_state_type cmd_state = CMD_STATE_IDLE;

void setup()
{   
  programmer_init(); 
  exit_softid_mode();
  inp_data();
  memset(sec_buff, 0, sizeof(sec_buff));
  Serial.begin(115200);
}

void loop()
{
  switch(cmd_state)
  {
  case CMD_STATE_IDLE:
    {
      if (!Serial.available())
        return;
      int b = Serial.read();
      if (b < 0)
        return;
      cmd = (cmd >> 8u) | (dword(b) << 24);
      if (cmd == SYNC_MAGIC)
        cmd_state = CMD_STATE_SYNCED;
      return;
    }    
  case CMD_STATE_SYNCED:
    {
      if (!Serial.available())
        return;        
      cmd = 0u;
      int len = Serial.readBytes((byte*)&cmd, 
        sizeof(cmd));
      if (len != sizeof(cmd)) {
        Serial.write("LOS!");
        cmd_state = CMD_STATE_IDLE;
        return;
      }  
      
      // Decode command
      
      switch(cmd)
      {
      case CMD_SOFT_INFO:
        {
          word sfid = read_soft_id();
          Serial.write((char*)&CMD_SOFT_INFO, sizeof(CMD_SOFT_INFO));
          Serial.write((char*)&sfid, sizeof(sfid));
          Serial.write((char*)&sfid, sizeof(sfid));
          break;
        }
      case CMD_ERASE_CHIP:
        {
          chip_erase();
          Serial.write((char*)&CMD_ERASE_CHIP, sizeof(CMD_ERASE_CHIP));
          break;
        }
      case CMD_ERASE_SECTOR:
        {
          Serial.readBytes((byte*)&cmd, sizeof(cmd));
          sector_erase(cmd);
          Serial.write((char*)&CMD_ERASE_SECTOR, sizeof(CMD_ERASE_SECTOR));
          break;
        }
      case CMD_READ_BYTES:
        {          
          Serial.readBytes((byte*)&cmd, sizeof(cmd));
          sector_read(sec_buff, cmd);
          dword crcc = xcrc32(sec_buff, BUFFER_SIZE, 0u);
          Serial.write((char*)&CMD_READ_BYTES, sizeof(CMD_READ_BYTES));
          Serial.write((char*)&crcc, sizeof(crcc));
          Serial.write((char*)sec_buff, BUFFER_SIZE);
          break;
        }
      case CMD_WRITE_BYTES:
        {
          dword addr, crcc;
          Serial.readBytes((byte*)&addr, sizeof(addr));
          Serial.readBytes((byte*)&crcc, sizeof(crcc));
          Serial.readBytes(sec_buff, BUFFER_SIZE);
          if (crcc == xcrc32(sec_buff, BUFFER_SIZE, 0u))
          {          
            sector_program(sec_buff, addr);            
            Serial.write((char*)&CMD_WRITE_BYTES, sizeof(CMD_WRITE_BYTES));
          }
          else
          {
            Serial.write("CRC!");
          }
          break;
        }
      
      default:
        Serial.print("IVC!");
        break;
      }
      cmd_state = CMD_STATE_IDLE;
      return;
    }    
  }
}
