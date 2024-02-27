#ifndef MAX11270_HELPER
#define MAX11270_HELPER

#include <Arduino.h>
#include "max11270.h"


void tbinprint(byte a)
{
  for (byte i = 0; i<8; i++)
  {
    Serial.print(0x01 & (a>>i));
    Serial.print("\t");
  }
}

void print_status_register(Max11270 &M)
{
  uint16_t status = M.getStatusRegister();
  Serial.println("Status register  ---------- ");
  Serial.println("\
RDY\t\
mstat\t\
dor\t\
sysgor\t\
rate0\t\
rate1\t\
rate2\t\
rate3\t\
\t\t\
aor\t\
rderr\t\
pdstat0\t\
pdstat1\t\
-----\t\
----\t\
error\t\
inreset\t");
  tbinprint(status & 0xff);
  Serial.print("\t\t");
  tbinprint(status>>8);
  Serial.println();
  Serial.println();
}    

void print_ctrl_registers(Max11270 &M)
{
	            

  Serial.println("CTRL 1  ---------");
  Serial.println("\
contsc\t\
scycle\t\
format\t\
u/b\t\
pd0\t\
pd1\t\
sync\t\
extck");
  tbinprint(M.getCtrlRegister(1));
  Serial.println();
  Serial.println();

  Serial.println("CTRL2 ----------");
  Serial.println("\
pga0\t\
pga1\t\
pga2\t\
pgaen\t\
lpmode\t\
bufen\t\
dgain0\t\
dgain1");
  tbinprint(M.getCtrlRegister(2));
  Serial.println();
  Serial.println();

  Serial.println("CTRL3 ----------");
  Serial.println("\
----\t\
----\t\
----\t\
data32\t\
modbits\t\
enmsync\t\
----\t\
----");
  tbinprint(M.getCtrlRegister(3));
  Serial.println();
  Serial.println();

  Serial.println("CTRL 4 ----------");
  Serial.println("\
dio1\t\
dio2\t\
dio3\t\
----\t\
dir1\t\
dir2\t\
dir3\t\
----");
  tbinprint(M.getCtrlRegister(4));
  Serial.println();
  Serial.println();

  Serial.println("CTRL5 ----------");
  Serial.println("\
nosco\t\
noscg\t\
nosyso\t\
nosysg\t\
----\t\
----\t\
cal0\t\
cal1");
  tbinprint(M.getCtrlRegister(5));
  Serial.println();
  Serial.println();

}

void print_data_and_adc_registers(Max11270 &M)
{

  Serial.print("DATA (0x6):      0x"); Serial.println(M.read24bitRegister(0x6), HEX);
  Serial.print("SOC_ADC (0x15):  0x"); Serial.println(M.read24bitRegister(0x15), HEX);
  Serial.print("SGC_ADC (0x16):  0x"); Serial.println(M.read24bitRegister(0x16), HEX);
  Serial.print("SCOC_ADC (0x17): 0x"); Serial.println(M.read24bitRegister(0x17), HEX);
  Serial.print("SCGC_ADC (0x18): 0x"); Serial.println(M.read24bitRegister(0x18), HEX);

}

void print_all_registers	(Max11270 &M)
{

     Serial.println("\n\n #######-- Print Registers --#######");
     print_status_register(M);
     print_ctrl_registers(M);
     print_data_and_adc_registers(M);
}

#endif