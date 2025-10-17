/*#include <Arduino.h>
#include "mcp_2515_ll.h"

uint8_t buff[8] = {};
uint8_t counter_vin = 0;
uint8_t counter_CF = 1;
uint32_t StMin = 0;
uint8_t buff_int[3] = {};
bool ok = 0;
bool has_rx = false;
bool session_active = false;
bool ok2 = true; 
const uint8_t vin[17] = {0x56, 0x46, 0x31, 0x41, 0x42, 0x32, 0x33, 0x43, 0x34, 0x44, 0x35, 0x45, 0x36, 0x37, 0x47, 0x38, 0x39};

enum class RxState: uint8_t{
        Idle,
        TxFF,
        TxFC, 
        //Abort //errore/timeout -> reset
};
static RxState state = RxState::Idle;

void stampa(uint16_t id, uint8_t dlc, const uint8_t* data){
    Serial.print(F("id=0X")); Serial.print(id,HEX); Serial.print(" ");
    Serial.print(F("dlc=")); Serial.print(dlc); Serial.print(" ");
    Serial.print(F("data="));
    for(int i = 0; i < dlc; i++){
        if(data[i] < 0X10) Serial.print('0');  // aggiungo lo zero soltanto per valori che vanno da 0X00 a 0X0F, dopodichè stampo il valore in esadecimale
            Serial.print(data[i],HEX);
            Serial.print(' ');   
            }
            Serial.println();
}

//DEBUG REGISTRI
static void dump_mcp2515(const char* tag) {
  Serial.print(F("\r\n--- MCP2515 DUMP: ")); Serial.print(tag); Serial.println(F(" ---"));

  uint8_t canctrl = mcp_read(MCP_CANCTRL);
  uint8_t canstat = mcp_read(MCP_CANSTAT);
  uint8_t cnf1    = mcp_read(MCP_CNF1);
  uint8_t cnf2    = mcp_read(MCP_CNF2);
  uint8_t cnf3    = mcp_read(MCP_CNF3);
  uint8_t intf    = mcp_read(MCP_CANINTF);
  uint8_t eflg    = mcp_read(MCP_EFLG);
  uint8_t tec     = mcp_read(MCP_TEC);
  uint8_t rec     = mcp_read(MCP_REC);
  uint8_t rxb0    = mcp_read(MCP_RXB0CTRL);

  Serial.print(F("CANCTRL=0x")); Serial.println(canctrl, HEX);
  Serial.print(F("CANSTAT=0x")); Serial.println(canstat, HEX);
  Serial.print(F("CNF1   =0x")); Serial.println(cnf1,   HEX);
  Serial.print(F("CNF2   =0x")); Serial.println(cnf2,   HEX);
  Serial.print(F("CNF3   =0x")); Serial.println(cnf3,   HEX);
  Serial.print(F("CANINTF=0x")); Serial.println(intf,   HEX);
  Serial.print(F("EFLG   =0x")); Serial.println(eflg,   HEX);
  Serial.print(F("TEC    =0x")); Serial.println(tec,    HEX);
  Serial.print(F("REC    =0x")); Serial.println(rec,    HEX);
  Serial.print(F("RXB0CTL=0x")); Serial.println(rxb0,   HEX);

  // opzionale: mask/filters
  uint8_t m0h=mcp_read(MCP_RXM0SIDH), m0l=mcp_read(MCP_RXM0SIDL);
  uint8_t m1h=mcp_read(MCP_RXM1SIDH), m1l=mcp_read(MCP_RXM1SIDL);
  Serial.print(F("RXM0  =0x")); Serial.print(m0h,HEX); Serial.print(' ');
  Serial.println(m0l,HEX);
  Serial.print(F("RXM1  =0x")); Serial.print(m1h,HEX); Serial.print(' ');
  Serial.println(m1l,HEX);
}


static void ecu_send_pr(uint8_t sub){
    uint8_t sub7 = (uint8_t)(sub & 0X7F); // tolgo il bit 7, nonchè il bit di sub-funzione
    uint8_t tx[3] = {0X02, 0X7E, sub7}; // mi serve per mandare una postive responce per 3E (3E+0X40)
    if(mcp_txb0_load_std(0X7E8, tx, 3)) mcp_rts_txb0();
}

static void ecu_send_nr(uint8_t reqSid, uint8_t nrc){
    uint8_t tx[4] = {0X03, 0X7F, reqSid, nrc};
    if(mcp_txb0_load_std(0X7E8, tx, 4)) mcp_rts_txb0();
}
void setup(){
    Serial.begin(115200);
    delay(1500);
    Serial.println(F("\n"));
    Serial.println(F("----------------"));
    Serial.println(F("ECU connection start"));
    Serial.println(F("----------------"));
    Serial.println();
     
    mcp_reset();
    
    if(!mcp_set_mode(MCP_MODE_CONFIG)) Serial.print(F("MODE CONFIG SET FAIL\r\n"));
    
    if(!mcp_set_bit_timing_500k_8MHz()) Serial.print(F("BIT TIMING SET FAIL\r\n"));

    //if(!mcp_rx0_set_filter_std(0X7E0)) Serial.print(F("ID NOT ACCEPTED\r\n")); filtro hardware (non funzionante, da aggiustare)
     mcp_rx0_accept_all();
        
    if(!mcp_set_mode(MCP_MODE_NORMAL)) Serial.println(F("MODE NORMAL FAIL\r\n"));
    //dump_mcp2515("after init");
        
}
void loop(){

 if((mcp_read(MCP_CANSTAT) & MCP_CANCTRL_REQOP_MASK) == MCP_MODE_NORMAL){
     uint16_t id{0};
     uint8_t dlc{0};
     uint8_t data[8]{};
         
     if(mcp_poll_rx0(&id, data, &dlc)){
         uint8_t pci = data[0];
         uint8_t type = pci >> 4;
         uint8_t L = 0, uds[8];
 
         if(id != 0X7E0){ //filtro software
         Serial.print("ID ERROR\r\n");
         return; 
         }
            
         if((type == 0X0) && ((data[2] == 0XF1) && (data[3] == 0X90))){
         has_rx = true;
         //RxState::Idle;
         Serial.println();
         Serial.print(F("MESSAGE RECEIVE: "));
         stampa(id, dlc, data);
         }
         
         else if((type == 0X0) && ((data[2] != 0XF1) && (data[3] != 0X90))){
             Serial.print(F("DEBUG OK1\r\n")); // cioè il nibble alto
             is_sf_and_extract(data, dlc, &L,uds);
             if(L != 2){
                 ecu_send_nr(0X3E, 0X13);
                 return;
             }
 
             if(L>= 1 && uds[0] == 0X3E){ // verifico se il serivzio richisto sia il tester present 
                 if(L != 2){ // perché la richiesta di tester present ha esttamente due 2 byte di payload, SID = 0X3E + subfunction
                     ecu_send_nr(0X3E, 0X13);
                     return;
                 } 
 
                 uint8_t sub = (L >= 2) ? uds[1] : 0X00; // cioè se c'è almeno un byte oltre al SID, allora proprio quell'ulteriore byte è la subfunction e la salvo dentro sub.
                 uint8_t sub7 = (uint8_t)(sub & 0X7F); //calcolo la sub-funzione reale (solo i 7 bit bassi), azzerando il bit 7. Mi serve per validare la sub: per il tester presetn aceeto soltanto sub7 == 0X00, e inoltre perché per la PR devo "echoare" solo i 7 bit bassi (il bit 7 non si deve riflettere nella risposta)
                 
                 if((sub & 0X80) != 0){ //quindi se è 1 è come se il tester dicesse: esegui ma non mandarmi la risposta positiva, mandami una NR solo se c'é un errore. (0X80 = 1000 0000)
                     Serial.print(F("3E 80: NO PR\r\n"));  
                     //ecu_send_nr();
                     return;
                 
                     ecu_send_pr(sub7);
                     Serial.print(F("PR 7E 00 sent\r\n")); // mando un postive responce per 3E
                     Serial.println();
                     return;
                 } 
             }   
         }             
     }
 
     if(has_rx){
         switch(state){
                        
         case RxState::Idle: {

            Serial.print(F("IDLE"));

            if(session_active == true){
                Serial.println();
                Serial.print(F("-----------\r\n"));
                Serial.print(F("END SESSION\r\n"));
                Serial.print(F("-----------\r\n"));
            }
            session_active = false;
            memset(buff, 0, sizeof(buff));
             ok = 0;
             counter_vin = 0;
             counter_CF = 1;
             memset(buff, 0, sizeof(buff));
             memset(buff_int, 0, sizeof(buff_int));
             StMin = 0;
 
             for(uint8_t i = 0; i<3; i++){
                 buff_int[i] = data[i+1];
             }
             
             //has_rx = false;
             Serial.println();
             Serial.print(F("COMMAND RECEIVE: "));
             for(uint8_t i = 0; i<3; i++){
                 Serial.print(buff_int[i],HEX);
                 Serial.print(F(" "));
             } 
             Serial.println();
             state = RxState::TxFF;
             break;
         }
 
          case RxState::TxFF:{
          Serial.print(F("TxFF")); 
          buff[0] = 0X10;
          buff[1] = 0X14;
          buff[2] = buff_int[0] + 0X40;
          for(uint8_t i = 0; i < 2; i++) buff[i+3] = buff_int[i+1];
         
          if(counter_vin < 17){
              for(uint8_t i = 0; i < 3; i++){
                  buff[i+5] = vin[i];
                  counter_vin++;
          }
          }
          for(uint8_t i = 0; i < sizeof(buff); i++){
              Serial.print(buff[i], HEX);
              Serial.print(" ");
          }
          Serial.println(); 
          if(mcp_txb0_load_std(0X7E8, buff, 8)) mcp_rts_txb0();
          Serial.print(F("FIRST FRAME SENDED\r\n"));
 
          state = RxState:: TxFC;
          break;
          }
 
          case  RxState::TxFC:{
            Serial.print(F("TxFC"));
              if(!mcp_poll_rx0(&id, data, &dlc)) break;
            
              uint8_t type_data = data[0] >> 4;
              if(type_data != 0X3){
                Serial.print("FC ERROR\r\n");
                break;
              } 
                                      
              else if(data[0] == 0X31){
                  delay(3000);
                  ok = 1;
              }
              else if(data[0] == 0X32){
                  Serial.print(F("OVF ERROR."));
                  has_rx = false;
                  break;
              }
          
              else if((data[0] == 0X30) || (ok == 1)){
                  Serial.println();
                  uint8_t buff_FC[8] = {};
                  while(counter_vin < sizeof(vin)){
                      Serial.println();
                      delay(100); // STmin
                      for(uint8_t i = 0; i <7; i++){
                      buff_FC[i+1] = vin[counter_vin];
                      counter_vin++;
                      }
                      buff_FC[0] = 0X20+counter_CF;
                      counter_CF++;
 
                      if(mcp_txb0_load_std(0x7E8, buff_FC, 8)) mcp_rts_txb0();
 
                      Serial.print(F("CF SENDED\r\n"));
                      
                      for(uint8_t i = 0; i < sizeof(buff_FC); i++){
                          Serial.print(buff_FC[i], HEX);
                          Serial.print(F(" "));
                      }
                          Serial.println();
                  }
              }
              has_rx = false;
              session_active = true;
              //state = RxState::Idle;
              break;
          }
         }
        }
    }
}*/