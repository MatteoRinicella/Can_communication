#pragma once
#include <stdint.h>
#include <stdbool.h>
#ifdef __cplusplus
extern "C"{
#endif 

//op code
#define MCP_RESET 0XC0
#define MCP_READ 0X03
#define MCP_WRITE 0X02
#define MCP_READ_STATUS 0XA0
#define MCP_BIT_MOD 0X05
#define MCP_RTS_TXB0  0X81

//registri principali
#define MCP_CANSTAT 0X0E // mi dice in che stato mi trovo
#define MCP_CANCTRL 0X0F // decido la mdoalità
/*#define MCP_CNF1 0X00 //contiente baudrate e prescaler (BRP e SJW)
#define MCP_CNF2 0X91 // con questi 3 imposto il baudrate e il bit timing
#define MCP_CNF3 0X01*/

#define MCP_CNF1 0X2A
#define MCP_CNF2 0X29
#define MCP_CNF3 0X28
#define MCP_CANINTF 0X2C // per capire se la trasmissione/recezione è finita
#define MCP_CANINTF_RX0IF 0X01
#define MCP_CANINTF_RX1IF 0X02
#define MCP_CANCTRL_OSM 0X08 
#define MCP_TEC 0X1C
#define MCP_REC 0X1D //rileva gli errori di protocollo, me lo aspetto sempre a zero
#define MCP_EFLG 0X2D

//TXB0
#define MCP_TXB0CTRL 0X30 // Controllo/avvio stato del buffer
#define MCP_TXB0SIDH 0X31 // identificazione standard bit
#define MCP_TXB0SIDL 0X32 // stessa cosa 
#define MCP_TXB0DLC 0X35 // lunghezza e tipo di frame 
#define MCP_TXB0D0 0X36 // dati
#define MCP_TXB0CTRL_TXREQ 0X08

//RXB0
#define MCP_RXB0CTRL 0X60
#define MCP_RXB0SIDH 0X61
#define MCP_RXB0SIDL 0X62
#define MCP_RXB0DLC 0X65
#define MCP_RXB0D0 0X66
#define MCP_RXF0SIDH 0X00
#define MCP_RXF0SIDL 0X01
#define MCP_RXF1SIDH 0X04
#define MCP_RXF1SIDL 0X05
#define MCP_RXF2SIDH 0X08
#define MCP_RXF2SIDL 0X09
#define MCP_RXF3SIDH 0X10
#define MCP_RXF3SIDL 0X11
#define MCP_RXF4SIDH 0X14
#define MCP_RXF4SIDL 0X15
#define MCP_RXF5SIDH 0X18
#define MCP_RXF5SIDL 0X19

//BITMASK
#define MCP_CANCTRL_REQOP_MASK 0XE0 
#define MCP_MODE_NORMAL 0X00
#define MCP_MODE_SLEEP 0X20
#define MCP_MODE_LOOPBACK 0X40
#define MCP_MODE_LISTEN_ONLY 0X60
#define MCP_MODE_CONFIG 0X80
#define MCP_RXM0SIDH 0X20
#define MCP_RXM0SIDL 0X21
#define MCP_RXM1SIDH 0X24
#define MCP_RXM1SIDL 0X25



// API LL
uint8_t mcp_spi_txrx(uint8_t b);

void mcp_reset(void); // manda l'opcode RESET (0XC0), dopo entra in configurazione mode.
uint8_t mcp_read(uint8_t addr);
void mcp_write(uint8_t addr, uint8_t val);
void mcp_bit_modify(uint8_t addr, uint8_t mask, uint8_t data); // aggiorna solo i bit selezionati da mask nel registro addr, mask dice quali bit toccare (1=modifica, 0=lascia com'è), data dice quali sono i nuovi valori per quei bit (solo dove mask ha 1)

bool mcp_set_mode(uint8_t mode); // cambia la modalità operativa dell'MCP e verifica che il chip sia davvero entrato. Restituisce true se il cambio è andato a buon fine, false se non ci entra entro un piccolo timeout.
bool mcp_set_bit_timing_500k_8MHz(void); // 500 -> valore comune in automotive, 1 bit ogni due microsecondi.
//bool mcp_set_bit_timing_500k_16MHz(void);
void mcp_rx0_accept_all(void); // isolo i problemi fisici/ bitrate senza l'incognita dei filtri. Quindi in loopback entra qualsiasi cosa io trasmetto. Una volta certo che RX funziona, passo ai filtri (mask)

void mcp_id_to_regs_std(uint16_t id, uint8_t *sidh, uint8_t *sidl); //mi serve per impacchettare l'ID std a 11 bit nei registri SIDH/SIDL (entrambi a 8 bit)

bool mcp_txb0_load_std(uint16_t id,const uint8_t *data, uint8_t len); // len è la lunghezza del payload, diventa DLC nel registro TXB0DLC. 
bool mcp_rts_txb0(void); // serve a far partire la trasmissione del frame che ho caricato in TXB0

bool mcp_poll_rx0(uint16_t *id, uint8_t *data, uint8_t *len); // sonda non bloccante del receive buffer (RXB0), guarda se è arrivaot un frame 

bool mcp_rx0_set_filter_std(uint16_t flt);
bool is_sf_and_extract(const uint8_t *can_data, uint8_t dlc, uint8_t *outlen, uint8_t *out_payload);

#ifdef __cplusplus
} // chiudo extern C
#endif