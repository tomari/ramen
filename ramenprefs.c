#include <PalmOS.h>
#include "ramen.h"
#include "ramenprefsrsc.h"

static void prefsSetField(UInt16 fieldID, UInt32 distnum);
static Int32 prefsGetField(UInt16 fieldID);
static Boolean prefsHandleEvent(EventPtr eventP);

static void prefsSetField(UInt16 fieldID, UInt32 distnum) {
	//const UInt32 DIGITS_LEN=3;
	const FormPtr frmP = FrmGetActiveForm();
	const FieldPtr fldP=FrmGetObjectPtr(frmP,FrmGetObjectIndex(frmP, fieldID));
	Char buf[24];
	StrIToA(buf,distnum);

	FldInsert(fldP,buf,StrLen(buf));
}

static Int32 prefsGetField(UInt16 fieldID) {
	const FormPtr frmP = FrmGetActiveForm();
	const FieldPtr fldP = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, fieldID));
	const Char *text = FldGetTextPtr(fldP);
	const Int32 value=StrAToI(text);
	return value;
}

static Boolean prefsHandleEvent(EventPtr eventP) {
	Boolean handled = false;
	
	return handled;
}

extern void DoPrefs(UInt32 *values, UInt8 *AlarmVolume) {
	const FormPtr curfrm=FrmGetActiveForm();
	const FormPtr frmP=FrmInitForm(PREFSFORM);
	Boolean ItsNotOver;
	unsigned int i;
	
	FrmSetActiveForm(frmP);
	FrmSetEventHandler(frmP, prefsHandleEvent);
	FrmDrawForm(frmP);
	
	/* Init form */
	/* Set preset button values to the fields */
	i=4; do {
		prefsSetField(PRESETNUM1+i, values[i]);
	} while(i--);
	
	/* Set the alarm volume to the push button */
	FrmSetControlValue(frmP, FrmGetObjectIndex(frmP, VOLUMELOW+ *AlarmVolume),1);
		
	/* Exit */
	do {
		const UInt16 result=FrmDoDialog(frmP);
		ItsNotOver = false;
		if (result == OKAYBUTTON) {
			UInt32 numbers[5];
			i=4; do {
				const Int32 nfld=prefsGetField(PRESETNUM1+i);
				if(MINTIME <= nfld && nfld <= MAXTIME) {
					numbers[i]=nfld;
				} else {
					ItsNotOver=true;
				}
			} while(i--);
			if(ItsNotOver) {
				FrmAlert(NONNUMBERALERT);
			} else {
				const UInt16 alarmVolIdx=FrmGetControlGroupSelection(frmP, ALARMGROUP);
				if(alarmVolIdx != frmNoSelectedControl) {
					const UInt16 objId=FrmGetObjectId(frmP,alarmVolIdx);
					*AlarmVolume = objId - VOLUMELOW;
				}
				MemMove(values,numbers,sizeof(numbers));
			}
		}
	} while(ItsNotOver);
	
	FrmDeleteForm(frmP);
	FrmSetActiveForm(curfrm);
}
