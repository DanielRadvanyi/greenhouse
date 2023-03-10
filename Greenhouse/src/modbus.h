#ifndef SRC_MODBUSCONFIG_H_
#define SRC_MODBUSCONFIG_H_

#include "./ModbusMaster.h"
#include "./ModbusRegister.h"

#define HMP60_ID            241
#define GMP252_ID           240

#define TRANSMISSION_RATE	9600

#define read_temp_r_ADD     0x0101
#define read_rh_r_ADD       0x0100

#define read_co2_r_ADD      0x0101

#define Error_rh_r_ADD      0x0200

#define TIMEOUT_LIMIT       20
#define SUCCESS_MASK        0x0100
#define BREAK_TIME          1000

class modbusConfig {
public:
	modbusConfig();
	int get_temp();
	int get_rh();
	int get_co2();
	virtual ~modbusConfig();
private:

	ModbusMaster node_hmp; //rh&temp
	ModbusMaster node_gmp; //co2

	ModbusRegister temp_;
	ModbusRegister rh_;
	ModbusRegister co2_;
	Fmutex mutex;
};

#endif /* SRC_MODBUSCONFIG_H_ */
