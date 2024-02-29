#include "AXFManager.h"

extern AXFManager AXF;


int main(void) {
	
	AXF.Init();
	
	while (1) {
	
		AXF.Refresh();

	}

}	//	Main Routine