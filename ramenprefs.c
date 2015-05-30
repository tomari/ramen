#include <PalmOS.h>
#include "ramen.h"
#include "ramenprefs.h"
#include "ramenprefsrsc.h"

static void prefsSetField(UInt16 fieldID, UInt32 distnum);
static UInt16 prefsGetField(UInt16 fieldID);
static Boolean prefsHandleEvent(EventPtr eventP);

static void prefsSetField(UInt16 fieldID, UInt32 distnum) {
	FormType *frmP = FrmGetActiveForm();
	Char insertChars[3];
	
	StrIToA(insertChars, distnum);
	FldInsert(FrmGetObjectPtr(frmP,FrmGetObjectIndex(frmP, fieldID)), insertChars, StrLen(insertChars));
}

static UInt16 prefsGetField(UInt16 fieldID) {
	FormType *frmP = FrmGetActiveForm();
	MemHandle textH = FldGetTextHandle(FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, fieldID)));
	Char *text;
	UInt16 value;
	
	text = MemHandleLock(textH);
	value = StrAToI(text);
	MemHandleUnlock(textH);
	
	return value;
}

static Boolean prefsHandleEvent(EventPtr eventP) {
	Boolean handled = false;
	
	return handled;
}

extern void DoPrefs(UInt32 *values, UInt8 *AlarmVolume) {
	FormType *frmP;
	FormType *curfrm;
	UInt16 result;
	UInt8 i;
	Boolean ItsNotOver = true;
	UInt32 *curfld;
	UInt32 numbers[5];
	
	curfrm = FrmGetActiveForm();
	frmP=FrmInitForm(PREFSFORM);
	FrmSetActiveForm(frmP);
	FrmSetEventHandler(frmP, prefsHandleEvent);
	FrmDrawForm(frmP);
	
	/* Init form */
	/* Set preset button values to the fields */
	for(i=0;i<5;i++) {
		prefsSetField(PRESETNUM1+i, *(values + i));
	}
	
	/* Set the alarm volume to the push button */
	for(i=0;i<4;i++) {
		if(*AlarmVolume == i) {
			FrmSetControlValue(frmP, FrmGetObjectIndex(frmP, VOLUMELOW+i),1);
		}
	}
	
	/* Exit */
	while(ItsNotOver == true) {
		result = FrmDoDialog(frmP);
		ItsNotOver = false;
		if (result == OKAYBUTTON) {
			for(i=0; i<5; i++) {
				numbers[i] = prefsGetField(PRESETNUM1+i);
				if((numbers[i] < MINTIME) || (numbers[i] > MAXTIME)) {
					ItsNotOver = ItsNotOver|true;
				}
			}
			if(ItsNotOver == false) {
				for(i=0; i<5; i++) {
					curfld = (values + i);
					*curfld = numbers[i];
				}
				for(i=0; i<4; i++) {
					if(FrmGetControlValue(frmP, FrmGetObjectIndex(frmP, VOLUMELOW+i)) == 1) {
						*AlarmVolume = i;
					}
				}
			} else {
				FrmAlert(NONNUMBERALERT);
			}
		} else {
			ItsNotOver = false;
		}
	}
	
	FrmDeleteForm(frmP);
	FrmSetActiveForm(curfrm);
}
