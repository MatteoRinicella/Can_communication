#include <Arduino.h>
#include "mcp_2515_ll.h"

uint8_t buff[8] = {};
uint8_t buff_FC[8] = {};
uint8_t counter_vin = 0;
uint8_t counter_CF = 1;
uint32_t StMin = 0;
uint8_t buff_int[3] = {};
uint8_t buff_wait[2] = {};
uint8_t counter_error = 0;
uint8_t counter_uds_error = 0;
uint8_t buff_err[8] = {};

const uint8_t vin[17] = {0x56, 0x46, 0x31, 0x41, 0x42, 0x32, 0x33, 0x43, 0x34, 0x44, 0x35, 0x45, 0x36, 0x37, 0x47, 0x38, 0x39};

enum class RxState: uint8_t{
        Idle,
        TxFF,
        SendTxFF,
        TxFC, 
};
static RxState state = RxState::Idle;

static void ecu_send_pr(uint8_t sub){
    uint8_t sub7 = (uint8_t)(sub & 0X7F); // tolgo il bit 7, nonchè il bit di sub-funzione
    uint8_t tx[3] = {0X02, 0X7E, sub7}; // mi serve per mandare una positive responce per 3E (3E+0X40)
    if(mcp_txb0_load_std(0X7E8, tx, 3)){
        mcp_rts_txb0();
        Serial.print(F("PR 7E 00 sent\r\n")); // mando un postive responce per 3E
        Serial.println();    
    } 
}

static void ecu_send_nr(uint8_t reqSid, uint8_t nrc){
    uint8_t tx[4] = {0X03, 0X7F, reqSid, nrc};
    if(mcp_txb0_load_std(0X7E8, tx, 4)) mcp_rts_txb0();
}

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
}

void loop(){

    //Serial.println(F("Start loop..."));
    //delay(3000);

    if((mcp_read(MCP_CANSTAT) & MCP_CANCTRL_REQOP_MASK) == MCP_MODE_NORMAL){
        uint16_t id{0};
        uint8_t dlc{0};
        uint8_t data[8]{};
        uint8_t L = 0, uds[8];
        
        if(mcp_poll_rx0(&id, data, &dlc)){

            if((id != 0X7E0) ||(counter_error > 0)){
                Serial.println(F("ID ERROR!\r\n"));
                counter_error++;
                return;
            }
            if(counter_uds_error > 0){
                return;
            }
            for(uint8_t i = 0; i < dlc-1; ++i){
               //Serial.println("Here after");
                switch(state){

                case RxState::Idle:{
                    //Serial.print(F("IDLE\r\n"));
                    uint8_t pci = data[0];
                    uint8_t type = pci >> 4;
                    memset(buff, 0, sizeof(buff));
                    memset(buff_int, 0, sizeof(buff_int));
                    memset(buff_wait, 0, sizeof(buff_wait));
                    counter_error = 0;
                    StMin = 0;     

                    if(type == 0X0){
                        if((data[2] == 0XF1) && data[3] == 0X90){
                        counter_vin = 0;
                        counter_CF = 0;
                        Serial.println();
                        Serial.print(F("MESSAGE RECEIVE: "));
                        stampa(id, dlc, data);

                            for(uint8_t i = 0; i < 3; i++){ // salvo 22 F1 90
                                buff_int[i] = data[i+1];
                            }
                        state = RxState::TxFF;
                        break;

                        }
                        else if(((data[2] != 0XF1) || (data[3] != 0X90)) ||(counter_uds_error > 0)){
                            buff_err[0] = 0X7F;
                            for(uint8_t i = 0; i < 3; i++) buff_err[i+1] = data[i];
                            for(uint8_t i = 3; i < sizeof(buff_err); i++) buff_err[i] = 0X55;
                            if(mcp_txb0_load_std(0X7E8, buff_err, 8)) mcp_rts_txb0();
                            //DEBUG
                            for(uint8_t i = 0; i < sizeof(buff_err); i++){
                                Serial.print(buff_err[i], HEX);
                                Serial.print(" ");
                            } 
                           Serial.println();
                            Serial.println(F("Error, unrecognized UDS service\r\n"));
                            counter_uds_error++;
                            return;
                        }

                        if(is_sf_and_extract(data, dlc, &L, uds)){
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
                                return;
                                }                   
                            }
                    }
                    else if(type == 0X3){
                        if(data[0] == 0X31){
                        delay(3000);
                        state= RxState::TxFC;
                        }
                        else if(data[0] == 0x32){
                            Serial.println(F("OVF ERROR!"));
                            return;
                        }
                        else if(data[0] == 0X30){
                            state = RxState::TxFC;
                            break;
                        } 
                    }
                    break;                    
                }    

                case RxState::TxFF:{
                    //Serial.print(F("TxFF\r\n"));

                    buff[0] = 0X10;
                    buff[1] = 0X14;
                    buff[2] = buff_int[0] + 0X40;
                    for(uint8_t i = 0; i < 2; i++){ // copio F1 90 dentro il buffer
                        buff[i+3] = buff_int[i+1];
                    }
                    for(uint8_t i = 0; i < 3; i++){ // metto i primi byte del VIN dentro il buffer 
                        buff[i+5] = vin[i];
                        counter_vin++;
                    }

                    //stampa
                    Serial.println();
                    for(uint8_t i = 0; i < sizeof(buff); i++){
                        Serial.print(buff[i], HEX);
                        Serial.print(" ");
                    }
                    state = RxState::SendTxFF;
                    break;
                }

                case RxState::SendTxFF:{
                    //Serial.print(F("SendTxFF\r\n"));

                    if(mcp_txb0_load_std(0X7E8, buff, 8)) mcp_rts_txb0();
                    Serial.println(F(" -> FIRST FRAME SENDED\r\n"));
                    //mcp_poll_rx0(&id, data, &dlc);
                    state = RxState:: Idle;
                    break;
                }

                case RxState::TxFC:{
                    //Serial.println(F("case -> TxFC"));
                    while(counter_vin < sizeof(vin)){
                        delay(100);
                        buff_FC[0] = 0X20+counter_CF;
                        counter_CF++;
                        for(uint8_t i =0; i < 7; i++){
                            buff_FC[i+1] = vin[counter_vin];
                            counter_vin++;
                        }
                        if(mcp_txb0_load_std(0x7E8, buff_FC, 8)) mcp_rts_txb0();
                        Serial.print(F("CF SENDED\r\n"));
                    }
                    state = RxState::Idle;                    
                    break;
                }
                return;
                }//end switch
            }
       //} // end if id error
        }          
    }
}