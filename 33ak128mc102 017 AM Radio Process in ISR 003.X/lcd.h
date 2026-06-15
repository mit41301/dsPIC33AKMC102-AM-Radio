//#define LCD_I2C_ADDR 0x3E // Farnell part number 2063208, MIDAS Part No MCCOG21605B6W-SPTLYI
//#define LCD_I2C_ADDR 0x3E // Farnell part number 2063205, MIDAS Part No MCCOG21605D6W-SPTLYI 
#define LCD_I2C_ADDR 0x3E // AE-AQM0802 
#define LCD_TRIS_RESET TRISCbits.TRISC4 // Reset
#define LCD_LAT_RESET LATCbits.LATC4 // Reset

void LCDInit(void);
void LCDPutC(char c);
void LCDPutU32(unsigned long u32,int8_t n8NumDigits);
void LCDClear(void);
void LCDCursor(uint8_t u8x,uint8_t u8y);
void LCDPutS(const char *psz);
