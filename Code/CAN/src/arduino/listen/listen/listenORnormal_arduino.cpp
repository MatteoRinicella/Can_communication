/*#include <Arduino.h>
#include "mcp_2515_ll.h"

bool ok = false;

void setup(){
    Serial.begin(115200);
    //while(!Serial){} //attendo che la seriale sia aperta 
    delay(1000);
    Serial.println(F("\n"));
    Serial.println(F("----------------"));
    Serial.println(F("Connection start"));
    Serial.println(F("----------------"));
    Serial.println();
     
    mcp_reset();
    uint8_t cs = mcp_read(MCP_CANSTAT);
    Serial.print(F("REGISTER CONFIGURATION -> CANSTAT=0x")); Serial.println(cs,HEX);
    
    mcp_set_mode(MCP_MODE_CONFIG);
    
    mcp_set_bit_timing_500k_8MHz();

    if(!mcp_set_bit_timing_500k_8MHz()) Serial.println(F("CNF set FAIL"));
    else
        Serial.println() ;
        Serial.println(F("SET BIT TIMING OK."));

    mcp_rx0_accept_all();

    mcp_set_mode(MCP_MODE_NORMAL);
    //mcp_set_mode(MCP_MODE_LISTEN_ONLY);
    //if(!(mcp_set_mode(MCP_MODE_NORMAL) || mcp_set_mode(MCP_MODE_LISTEN_ONLY))) Serial.println(F("set operating mode FAIL"));
    if(mcp_read(MCP_CANSTAT) == 0X00){
        Serial.println(F("MODE: NORMAL (ready)"));
        ok = false;
    }
     if(mcp_read(MCP_CANSTAT) == 0X60){
        Serial.println(F("MODE: LISTEN ONLY (ready)"));
        ok = true;
    }       
}

void loop() {
    if(mcp_read(MCP_CANSTAT) == 0X00){
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
    }
    delay(50);
    }
    else if(ok == true) {
        Serial.println(F("FAIL, ACK ERROR DETECTED"));
        ok = false;
    }
        
    
}

*/