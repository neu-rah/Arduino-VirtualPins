//#include "../VPinsSPI/VPinsSPI.h"

class VPortServer {
protected:
	TwoWire &Wire;
public:
	VPortServer(TwoWire & wire);
	void begin(uint8_t serverID);
};

