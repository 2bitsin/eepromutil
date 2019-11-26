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
typedef uint8_t byte;
typedef uint16_t word;
typedef uint32_t dword;

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

//////////////////////////////////

static const constexpr dword BUFFER_SIZE    = 4096u;
static const constexpr dword SYNC_MAGIC     = 0x434E5953ull;
static const constexpr dword SACK_MAGIC     = 0x4B434153ull;

static const constexpr dword CMD_SOFT_INFO  = 0x44494653ull;
static const constexpr dword CMD_ERASE_CHIP = 0x48435245ull;

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
  PORT_DATA_L = data;
}

byte inp_data()
{
  DDR_DATA_L = 0x00u;
  PORT_DATA_L = 0x00u;
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
  out_data(data);
  set_we(1);
  WAIT_SINGLE();
}

uint8_t read_cycle(dword addr)
{
  set_oe(0);
  set_we(0);
  set_addr(addr);
  set_oe(1);
  WAIT_SINGLE();
  return inp_data();
}

void poll_dq7(byte last_write)
{
  while ((inp_data()^last_write)&0x80u);  
}

word read_soft_id()
{
  write_cycle(0x5555u, 0xAAu);
  write_cycle(0x2AAAu, 0x55u);
  write_cycle(0x5555u, 0x90u);
  poll_dq7(0x90u);
  
  word a = read_cycle(0x0000u);  
  word b = read_cycle(0x0001u);  

  write_cycle(0x5555u, 0xAAu);
  write_cycle(0x2AAAu, 0x55u);
  write_cycle(0x5555u, 0x10u);
  poll_dq7(0x10u);  

  return a*0x100u + b;
}

enum cmd_state_type
{
  CMD_STATE_IDLE,
  CMD_STATE_SYNCED
};


static byte           sec_buff[BUFFER_SIZE] = {0};
static dword          cmd_buff[4u] = {0};
static cmd_state_type cmd_state = CMD_STATE_IDLE;

void setup()
{   
  programmer_init(); 
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
      cmd_buff[0] = (cmd_buff[0] >> 8u) | (dword(b) << 24);
      if (cmd_buff[0] == SYNC_MAGIC)
        cmd_state = CMD_STATE_SYNCED;
      return;
    }    
  case CMD_STATE_SYNCED:
    {
      if (!Serial.available())
        return;        
      cmd_buff[0] = 0u;
      int len = Serial.readBytes((byte*)&cmd_buff[0], 
        sizeof(cmd_buff[0]));
      if (len != sizeof(cmd_buff[0])) {
        Serial.write("LOS!");
        cmd_state = CMD_STATE_IDLE;
        return;
      }  
      
      // Decode command
      
      switch(cmd_buff[0])
      {
      case CMD_SOFT_INFO:
        {
          word sfid = read_soft_id();
          //Serial.write((char*)&sfid, 2);
          Serial.write("SFID");
          Serial.print(sfid, HEX);
          break;
        }
      case CMD_ERASE_CHIP:
        {
          break;
        }
      default:
        Serial.print("IVC!");
        break;
      }
      cmd_state = CMD_STATE_IDLE;
      break;
    }    
  }
}