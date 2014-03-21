/*
      
  *** FREQUENCIMETRO-VOLTIMETRO***
  
  Projeto feito por : 
                      Priscila Pereira Apocalypse


 Foi utilizado a placa programadora IKHC908GR4 operando a 10 MHZ

Observações:
            
 1) Para medir frequencia utilizamos os pinos: 
    PTD4 e PTD5 do microcontrolador
 2) Para medir tensão utilizamos os pinos : 
    PTB0 e PTB1 do microcontrolador
 3)Certifique-se com o osciloscopio que o pulso gerado esta dentro dos limites de 0 a 5V


 
*/

#include <mc68hc908gp32.h>

/* Biblioteca OpenSource */

#define NULL      0

#define sei()     _asm sei  _endasm
#define cli()     _asm cli  _endasm

unsigned char console=0;

int lcd_putchar(char);
int sci_putchar(char);

int putchar(char c)
{
  if(console)
    return lcd_putchar(c);
  else
    return sci_putchar(c);
}

void delay(unsigned value)
{
  unsigned count;
  
  while(value--)
    for(count=80;count;count--)
      ;
}

int sci_putchar(char c) {

  while(!SCTE);
  SCDR=c;
  
  if(c=='\n') {
  
    while(!SCTE);
    SCDR='\r';
  }
  
  return c;
}

int putstr(char *s) {

  int i,j;

  for(i=0;(j=s[i]);i++) putchar(j);
  
  return i;
}

int puts(char *s) {

  int i=putstr(s);
  
  putchar('\n');
  
  return i+1;
}

char *itox(unsigned int i) {

  static char tmp[6];
  const  char *zero="0";
  const  char value[16]="0123456789abcdef";
  
  int j;
  
  if(!i) return zero;
  
  for(tmp[j=sizeof(tmp)-1]=0;i;i=i>>4) tmp[--j]=value[i&0xf];
  
  return tmp+j;
}

char *itoa(int i) {

  static char tmp[7];
  const  char *zero="0";
  
  int j,f=0;
  
  if(!i) return zero;
  
  if(i<0) f=1;

  for(tmp[j=sizeof(tmp-1)]=0;i;i=i/10) tmp[--j]='0'+i%10;  
  
  if(f) tmp[--j]='-';
  
  return tmp+j;
}

int printf(const char *fmt, ... ) {

  int i,inside;

  char **p=(char **)&fmt;

  for(inside=i=0;fmt[i];i++) {
  
    if(inside) {
    
      if(fmt[i]=='s') putstr(*++p);
      if(fmt[i]=='d') putstr(itoa((int)*++p));
      if(fmt[i]=='x') putstr(itox((int)*++p));
      if(fmt[i]=='%') putchar('%');
      inside=0;
    
    } else { 
  
      if(fmt[i]=='%') inside=1;
      else putchar(fmt[i]);
    }
  }

  return i;
}

// configuracao dos pinos do LCD:

#define LCD_RS PTA1
#define LCD_RW PTA2
#define LCD_EN PTA3

#define LCD_D4 PTD0
#define LCD_D5 PTD1
#define LCD_D6 PTD2
#define LCD_D7 PTD3

#define clear_lcd         0x01 /* Clear Display                        */
#define return_home       0x02 /* Cursor to Home position              */
#define entry_mode        0x06 /* Normal entry mode                    */
#define entry_mode_shift  0x07 /* - with shift                         */
#define system_set_8_bit  0x38 /* 8 bit data mode 2 line ( 5x7 font )  */
#define system_set_4_bit  0x28 /* 4 bit data mode 2 line ( 5x7 font )  */
#define display_on        0x0c /* Switch ON Display                    */
#define display_off       0x08 /* Cursor plus blink                    */
#define set_dd_line1      0x80 /* Line 1 position 1                    */
#define set_dd_line2      0xC0 /* Line 2 position 1                    */
#define set_dd_ram        0x80 /* Line 1 position 1                    */
#define write_data        0x00 /* With RS = 1                          */
#define cursor_on         0x0E /* Switch Cursor ON                     */
#define cursor_off        0x0C /* Switch Cursor OFF                    */

void lcd_access_byte(unsigned char value,unsigned char mode)
{
  unsigned char value2;

  value2 = value>>4;

  PTA1   = mode;
  PTD    = value2 & 0xf;

  PTA3 = 1;
  
  delay(1);
  
  PTA3 = 0;
  
  delay(1);
}

void lcd_access_nibble(unsigned char value,unsigned mode)
{
  unsigned char value2;

  value2 = value>>4;

  PTA1   = mode;
  PTD    = value2 & 0xf;

  PTA3 = 1;
  
  delay(1);
  
  PTA3 = 0;
  
  delay(1);
  
  PTD    = value & 0xf;

  PTA3 = 1;
  
  delay(1);
  
  PTA3 = 0;
  
  delay(1);  
}

void lcd_init()
{
  DDRD = DDRA  = 0xf;
  PTD  = PTA   = 0;  

  lcd_access_byte( system_set_4_bit ,0); /* This sequence resets the LCD */
  delay(10);
  lcd_access_byte( system_set_4_bit ,0);
  delay(10);
  lcd_access_byte( system_set_4_bit ,0);
  delay(10); 
  lcd_access_nibble( system_set_4_bit ,0);
  delay(10);
  lcd_access_nibble( display_off ,0);
  delay(10);
  lcd_access_nibble( entry_mode ,0);
  delay(10);
  lcd_access_nibble( display_on ,0);
  delay(10);
  lcd_access_nibble( set_dd_ram ,0);
  delay(10);
  
  printf("lcd: standard 16x2 on SV1/SV7 pta/ptd ports.\n");
}

int lcd_putchar(char c)
{
  switch(c)
  {
    case '\f':
      lcd_access_nibble(clear_lcd,0);
      break;
      
    case '\n':
      lcd_access_nibble(set_dd_line2,0);
      break;
    
    default:
      lcd_access_nibble(c,1);
      break;
  }
  
  return c;
}

/* end of library */

#define MCU_CLOCK 2500000
#define REFERENCE 40

int ch0=0,ch1=0,f1=0,f2=0,ch0_cp=0,ch1_cp=0;
char second=0;

void timer_ch0(void) interrupt 4
{
  ch0++;
  if(CH0F) CH0F=0;
  f1 = T1CH0;   // registro do trigger do canal0
}

void timer_ch1(void) interrupt 5
{
  ch1++;
  if(CH1F) CH1F=0;
  f2 = T1CH1;   // registro do trigger do canal1
}

void timer_overflow(void) interrupt 6
{
  static int soft_timer=0; // a ideia eh ser muito rapido aqui!

  TOF = 0;                      // limpa flag de overflow do tim1  

  if(!soft_timer--)             // checa o soft_timer de 1Hz
  {  
    soft_timer = REFERENCE-1;   // rstaura contador
    
    ch0_cp = ch0; ch0 = 0;      // copia contador!
    ch1_cp = ch1; ch1 = 0;      // copia contador!
    
   second++;                   // ativa impressao
  }  
}

int main() 
{
  static unsigned int  max_idle=0,idle=0,sub_idle=0;
  static unsigned int  v1,v2;
  static unsigned char i;

  CONFIG1  = 1; // desabilita modulo COP   
                // configuracao da UART                  
                
  sei();        //desabilita todas as interrupções              
                
  SCP1  = 1;    // seta prescaler como sendo 4
  SCP0  = 0;    // 
                // valores para prescaler SCP1-0:
                // 
                // 00    1
                // 01    3
                // 10    4
                // 11    13
                //
  SCR2  = 0;    // para clock de 10MHz
  SCR1  = 1;    // e prescaler 4, temos o divisor 4
  SCR0  = 0;    // fornecendo baudrate de 9600 bps
                // 
                // valores para divisor SCR2-0:
                // 
                // 000    1
                // 001    2
                // 010    4
                // 011    8
                // 100    16
                // 101    32
                // 110    64
                // 111    128
                //
  ENSCI  = 1;   // habilita SCI
  TE  = 1;      // habilita transmissor

  printf("\33[2J\33[H");
  printf("starting HC908GR4 system...\n\n");
  printf("sci: 9600bps console on SV5\n");

  lcd_init();
/*
  DDRA=0xf;     // porta pta como saida
  PTA =0;
*/
  printf("out: pta0, pta1, pta2, pta3 on SV1.\n");

                // configuracao do ADC!
                //
  ADICLK  = 0;  // seleciona clock de 2.5MHz no ADC

  printf("adc: input clock: 2.5MHz.\n");

  ADCO  = 1;    // habilita ADC com conversao continua

  printf("adc: continuous convertion mode selected.\n");

  ADCH4  = 0;   // seleciona canal AD0, no pino PTB0
  ADCH3  = 0;   //
  ADCH2  = 0;   //
  ADCH1  = 0;   //
  ADCH0  = 0;   //
                // valores para selecao de canal ADCH04-0:
                //    
                // 00000  PTB0
                // 00001  PTB1
                // 00010  PTB2
                // 00011  PTB3
                // ...
                // 11101  5V
                // 11110  0V
                // 11111  off
                //
                // o HC908GR4CP possui apenas 4 canais. 

  printf("adc: ptb0, ptb1, ptb2, ptb3 on SV6.\n");
/*
  DDRD   = 0xf; // porta D como saida
  PTD    = 0x0; // porta D como 0

  printf("out: ptd0, ptd1, ptd2, ptd3 on SV2.\n");
*/
  PS2   = 0;    // PS0-2 setam o prescaler, que
  PS1   = 0;    // no caso divide o clock de 10MHz por
  PS0   = 0;    // 4 antes de ir para o timer

  printf("tim: input clock: 2.5MHz.\n");
                
  T1MOD = (MCU_CLOCK/REFERENCE)-1; // divisor calculado p/ 10MHz e 10KHz
              
  printf("tim: output clock: %dHz.\n",REFERENCE);
                
  TOIE  = 1;    // habilita interrupcao de TIM1
  TSTOP = 0;    // habilita contagem de TIM1

  TSTOP = 1;    // para timer

  CH0IE = 1;    // habilita interrupcoes do canal0
  CH0F  = 0;    // limpa status
  
  MS0B = 0;     // cconfigura canal0 para input capture em borda de subida
  MS0A = 0;
  ELS0B = 0;
  ELS0A = 1;

  CH0F  = 0;    // limpa status

  printf("ch0: ptd4 on SV7 (rising edge triggered).\n");

  CH1IE = 1;    // habilita interrupcoes do canal0
  CH1F  = 0;    // limpa status
  
  MS1A = 0;     // cconfigura canal0 para input capture em borda de subida
  ELS1B = 0;
  ELS1A = 1;

  CH1F  = 0;    // limpa status

  PTDPUE |= 0x30; // enable PTD pullups para timers!
          
  printf("ch1: ptd5 on SV7 (rising edge triggered).\n");

  TRST = 1;
  TSTOP = 0;    // reabilita timer



                // setup final: habilitar interrupcoes e ir para idle!

  printf("irq: enabled interrupts.\n\n");
  printf("system ready.\n\n");

  cli();        // habilita as interrupcoes

  for(idle=sub_idle=0;;sub_idle++,idle++) // contador de idle
  {
    while(second)      // flag
    {
      second--;
      
      PTA0 = !PTA0;       // led :D

      ADCH4 = 0;    // seleciona canal AD0, no pino PTB0
      ADCH3 = 0;    //
      ADCH2 = 0;    //
      ADCH1 = 0;    //
      ADCH0 = 0;    //
                    // valores para selecao de canal ADCH04-0:
                    //
                    // 00000  PTB0
                    // 00001  PTB1                                           

      for(v1=i=0;i!=196;i++) v1+=ADR;

      ADCH4 = 0;    // seleciona canal AD0, no pino PTB0
      ADCH3 = 0;    //
      ADCH2 = 0;    //
      ADCH1 = 0;    //
      ADCH0 = 1;    //
                    // valores para selecao de canal ADCH04-0:
                    //
                    // 00000  PTB0
                    // 00001  PTB1                                            

      for(v2=i=0;i!=196;i++) v2+=ADR;

      if(ch0_cp==0 && ch1_cp==0 && idle > max_idle) // calculo automatico de idle!
      {
        max_idle = idle;
      } 

      printf("ch0: %dmV %dHz ch1: %dmV %dHz cpu: %d%% %s\n",
        v1/10, ch0_cp,
        v2/10, ch1_cp,
        max_idle?100-idle/(max_idle/100):0,
        (idle==0&&max_idle!=0)?"overload":"");

      console = 1; // switch to lcd
    
      printf("\f%dmV %dHz\n%dmV %dHz",
        v1/10, ch0_cp,
        v2/10, ch1_cp);
      
      console = 0; // switch to sci! :D

      idle=0; // limpa totalizadores!
    }
  }
}
