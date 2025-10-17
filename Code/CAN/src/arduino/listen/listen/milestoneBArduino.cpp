/*#include <Arduino.h>
#include "mcp_2515_ll.h"

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
    //uint8_t cs = mcp_read(MCP_CANSTAT);
    //Serial.print(F("REGISTER CONFIGURATION -> CANSTAT=0x")); Serial.println(cs,HEX);
    
    if(!mcp_set_mode(MCP_MODE_CONFIG)) Serial.print(F("MODE CONFIG SET FAIL"));
    
    if(!mcp_set_bit_timing_500k_8MHz()) Serial.print(F("BIT TIMING SET FAIL"));

    if(!mcp_rx0_set_filter_std(0X7E0)) Serial.print(F("ID NOT ACCEPTED"));
        
    if(!mcp_set_mode(MCP_MODE_NORMAL)) Serial.println(F("MODE NORMAL FAIL"));        
}

void loop() {
    if((mcp_read(MCP_CANSTAT) & MCP_CANCTRL_REQOP_MASK) ==MCP_MODE_NORMAL){
    uint16_t id{0};
    uint8_t dlc{0};
    uint8_t data[8]{};
    if(mcp_poll_rx0(&id, data, &dlc)){

        Serial.print(F("id=0X")); Serial.print(id,HEX); Serial.print(" ");
        Serial.print(F("dlc=")); Serial.print(dlc); Serial.print(" ");
        Serial.print(F("data="));
        for(int i = 0; i < dlc; i++){
            if(data[i] < 0X10) Serial.print('0');  // aggiungo lo zero soltanto per valori che vanno da 0X00 a 0X0F, dopodichè stampo il valore in esadecimale
            Serial.print(data[i],HEX);
            Serial.print(' ');   
        }
        Serial.println();

        if(id != 0X7E0) return;

        uint8_t L = 0, uds[8];
        if(!is_sf_and_extract(data, dlc, &L, uds)){
            Serial.print(F("RX NOT SF OR INCOHERENT DLC\r\n"));
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
            } 
            
            ecu_send_pr(sub7);
            Serial.print(F("PR 7E 00 sent\r\n")); // mando un postive responce per 3E
            Serial.println();
            return;
        }        
    }
    delay(50);
    }           
}*/

