

outcompare.h
	#define OC_FAULT_PIN_TRISTATE		0xffff
	#define OC_FAULT_PIN_DEFAULT		0xdfff
	#define OC_DIRN_TRIS              0xffff  /*The signal ocmp_oe_out is forced to zero*/
	#define OC_DIRN_OUTPUT                0xffdf  /*The signal ocmp_oe_out can be active*/
	#define OC_SYNC_ENABLE			    0xff7f /* Sync_trig[x] is used to sync to another module timer */
	#define OC_SYNC_TRIG_IN_DISABLE 	0xffff	/* Disables trigger operation */
#define OC_FAULT_ALL_DISABLE		0xfc0f