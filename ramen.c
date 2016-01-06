#include <PalmOS.h>
#include <PalmChars.h>
#include <SonyChars.h>
#include "ramen.h"
#include "ramenrsc.h"

typedef struct {
	Int32 BombTime;
	Int32 RamenTime;
	Boolean TimerStarted;
	UInt32 Presets[5];
	UInt8 AlarmVolume;
} ramenPreferenceType;

static Boolean MainFormHandleEvent(EventPtr eventP);
static inline Boolean AppHandleEvent(EventPtr eventP);
static void setRamenTo(Int32 ramen);
static inline MemHandle memHandleWithMinSize(MemHandle mH, UInt32 minsize);
static void PaintNumbers(Int32 ramen);
static void SetAlarm(void);
static inline void DisplayAlarm(void);
static void refreshDisplay(void);
static inline Boolean isTheDeviceSupported(void);
static void refreshButtons(void);
static void RamenLoadPreferences(ramenPreferenceType *);
static void RamenSavePreferences(void);
static inline void RamenPlaySound(void);

/* Constants */
static const UInt32 MYCREATORID='Ramn';

/* var */
ramenPreferenceType rp={
	.BombTime = 0,
	.RamenTime = 3,
	.TimerStarted = false,
	.Presets = {1, 3, 5, 10, 60},
	.AlarmVolume = 3
};
UInt32 lastTimeRefreshed = 0;

/* implementation */
static Boolean MainFormHandleEvent(EventPtr eventP) {
	const eventsEnum eType=eventP->eType;
	if(eType == menuEvent) {
		const UInt16 itemID = eventP->data.menu.itemID;
		if (itemID == MenuAboutRamen) {
			MenuEraseStatus(0);
			FrmAlert(AboutAlert);
		} else if (itemID == MenuPrefs) {
			DoPrefs(rp.Presets, &rp.AlarmVolume);
			refreshButtons();
			RamenSavePreferences();
		} else {
			return false;
		}
	} else if(eType == ctlRepeatEvent) {
		const UInt16 controlID = eventP->data.ctlRepeat.controlID;
		if(controlID == REPEATUP) {
			setRamenTo(rp.RamenTime + 1);
		} else if(controlID == REPEATDOWN) {
			setRamenTo(rp.RamenTime - 1);
		}
		return false; /* leave unhandled so buttons can repeat */
	} else if(eType == ctlSelectEvent) {
		const UInt16 controlID = eventP->data.ctlSelect.controlID;
		if(controlID == STARTBUTTON) {
			SetAlarm();
		} else if(controlID == PRESET1 ||
			  controlID == PRESET2 ||
			  controlID == PRESET3 ||
			  controlID == PRESET4 ||
			  controlID == PRESET5) {
			setRamenTo(rp.Presets[eventP->data.ctlSelect.controlID - PRESET1]);
		} else {
			return false;
		}
	} else if(eType == keyDownEvent) {
		const WChar thekey =  eventP->data.keyDown.chr;
		if ((L'0' <= thekey) && (thekey <= L'9')) {
			const Int32 digit=thekey - L'0';
			if(rp.RamenTime > 9) {
				setRamenTo(digit);
			} else {
				setRamenTo(rp.RamenTime*10+digit);
			}
		} else if(thekey == vchrPageUp ||
			  thekey == vchrJogUp) {
			setRamenTo(rp.RamenTime+1);
		} else if(thekey == vchrPageDown ||
			  thekey == vchrJogDown) {
			setRamenTo(rp.RamenTime-1);
		} else if(NavSelectPressed(eventP) || 
			  thekey == vchrJogPush) {
			SetAlarm();
		} else if(thekey == vchrJogPushedUp) {
			setRamenTo(rp.RamenTime + 10);
		} else if(thekey == vchrJogPushedDown) {
			setRamenTo(rp.RamenTime - 10);
		} else {
			return false;
		}
	} else if(eType == nilEvent) {
		refreshDisplay();
	} else {
		return false;
	}
	return true;
}

static Boolean AppHandleEvent(EventPtr eventP) {
	const eventsEnum eType=eventP->eType;
	if (eType == frmLoadEvent) { /* Initialize and activate the form resource. */
		const FormPtr frmP = FrmInitForm(eventP->data.frmLoad.formID);
		FrmSetActiveForm(frmP);
		if (eventP->data.frmLoad.formID == MainForm) {
			refreshButtons();
			FrmSetEventHandler(frmP, MainFormHandleEvent);
		}
	} else if (eventP->eType == frmOpenEvent) { /* Load the form resource. */
		const FormPtr frmP = FrmGetActiveForm();
		FrmDrawForm(frmP);
		PaintNumbers(rp.RamenTime);
		refreshDisplay();
	} else if (eventP->eType == appStopEvent) { /* Unload the form resource. */
		const FormPtr frmP = FrmGetActiveForm();
		FrmEraseForm(frmP);
		FrmDeleteForm(frmP);
		RamenSavePreferences();	/* Store my preferences*/
	} else {
		return false;
	}
	return true;
}

static void setRamenTo(Int32 ramen) {
	static const Char* DOWNALLOW="\2";
	static const Char* UPALLOW="\1";
	static const Char* DOWNDISABLEDALLOW="\4";
	static const Char* UPDISABLEDALLOW="\3";
	Boolean scrollableUp=true;
	Boolean scrollableDown=true;
	const FormPtr frmP = FrmGetActiveForm();
	const ControlPtr UpButtonP=FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP,REPEATUP));
	const ControlPtr DownButtonP=FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, REPEATDOWN));
	
	if (ramen <= MINTIME) {
		ramen = MINTIME;
		SndPlaySystemSound(sndWarning);
		scrollableUp=true;
		scrollableDown=false;
	} else if (ramen >= MAXTIME) {
		ramen = MAXTIME;
		SndPlaySystemSound(sndWarning);
		scrollableUp=false;
		scrollableDown=true;
	}
	rp.RamenTime = ramen;
	rp.BombTime = 0; /*why did i add this code?*/
	rp.TimerStarted=true; /* Tell SetAlarm() to cancel any previously registered timer */
	SetAlarm();
	PaintNumbers(rp.RamenTime);
	if(CtlEnabled(UpButtonP) != scrollableUp) {
		CtlSetEnabled(UpButtonP, scrollableUp);
		if(scrollableUp == false) {
			CtlSetLabel(UpButtonP, UPDISABLEDALLOW);
		} else {
			CtlSetLabel(UpButtonP, UPALLOW);
		}
	}
	if(CtlEnabled(DownButtonP) != scrollableDown) {
		CtlSetEnabled(DownButtonP, scrollableDown);
		if(scrollableDown == false) {
			CtlSetLabel(DownButtonP, DOWNDISABLEDALLOW);
		} else {
			CtlSetLabel(DownButtonP, DOWNALLOW);
		}
	}
}

static MemHandle memHandleWithMinSize(MemHandle mH, UInt32 minsize) {
	if(mH) {
		if(MemHandleSize(mH)<minsize) {
			if(MemHandleResize(mH,minsize)) {
				return NULL;
			}
		}
	} else {
		mH = MemHandleNew(minsize);
	}
	return mH;
}

/* PaintNumbers(Int32 ramen)
	->ramen : The number to be drawn; if 0, it only erases the display area.
	UPDATE: new version uses a field to avoid flickers
*/
static void PaintNumbers(Int32 ramen) {
	const FormPtr frmP = FrmGetActiveForm();
	const UInt16 fldIndex = FrmGetObjectIndex(frmP, DISPLAY0);
	const FieldPtr fieldP = FrmGetObjectPtr(frmP, fldIndex);
	MemHandle textHandle=FldGetTextHandle(fieldP);
	FldSetTextHandle(fieldP,NULL);
	if((textHandle=memHandleWithMinSize(textHandle,RAMEN_DIGITS))) {
		Char *textp=MemHandleLock(textHandle);
		if(!ramen) {
			textp[0]=0;
		} else {
			StrIToA(textp,ramen);
		}
		MemHandleUnlock(textHandle);
	}
	FldSetTextHandle(fieldP,textHandle);
	FldDrawField(fieldP);
}

static void SetAlarm() {
	UInt16 cardNo;
	LocalID dbID;
	SysCurAppDatabase(&cardNo, &dbID);
	if (!rp.TimerStarted) {
		if (!rp.BombTime) {
			rp.BombTime = TimGetSeconds() + rp.RamenTime * 60;
		}
		{
			const Err errID=AlmSetAlarm(cardNo, dbID, 0, rp.BombTime, true);
			if(errID == almErrMemory) {
				FrmAlert(OUTOFMEMORYALERT);
			} else if(errID == almErrFull) {
				FrmAlert(TABLEFULLALERT);
			} else {
				rp.TimerStarted = true;
			}
		}
	} else { /* cancel timer */
		AlmSetAlarm(cardNo, dbID, 0, 0, true);
		rp.TimerStarted = false;
	}
}

static void DisplayAlarm() {
	MenuEraseStatus(0);
	{
		const FormPtr frmP = FrmInitForm(ALARMFORM);
		FrmDoDialog(frmP);
		FrmEraseForm(frmP);
		FrmDeleteForm(frmP);
	}
}

static void refreshDisplay() {
	if (rp.TimerStarted) {
		const Int32 now = TimGetSeconds();
		const Int32 interval = now - lastTimeRefreshed;
		if (interval) {
			const Int32 timeremaining = rp.BombTime - now;
			Int32 dispTime;
			if (timeremaining > 60) {
				if(now&0x1) {
					dispTime=0;
				} else {
					dispTime = timeremaining / 60;
				}
			} else if (timeremaining <= 0) {
				dispTime=rp.RamenTime;
				rp.TimerStarted = false;
				lastTimeRefreshed = 0;
				rp.BombTime = 0;
			} else {
				dispTime = timeremaining;
			}
			PaintNumbers(dispTime);
			lastTimeRefreshed = now;
		}
	}
}

static Boolean isTheDeviceSupported() {
	UInt32 romversion;
	FtrGet(sysFtrCreator,sysFtrNumROMVersion, &romversion);
	return (romversion>=sysMakeROMVersion(2,0,0,sysROMStageRelease,0));
}

static void refreshButtons() {
	const FormPtr frmP = FrmGetActiveForm();
	unsigned int i=4; do {
		const ControlPtr theButton=FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP,PRESET1+i));
		Char *label=(void *)CtlGetLabel(theButton);
		StrIToA(label, rp.Presets[i]);
		CtlDrawControl(theButton);
	} while(i--);
}

/*
	Global-Free Preference Loader for AlarmVolume implementation
*/
static void RamenLoadPreferences(ramenPreferenceType *pref_addr) {
	UInt16 prefsSize=0U;
	if(PrefGetAppPreferences(MYCREATORID, 0, NULL, &prefsSize, false) != noPreferenceFound) {
		if(prefsSize==sizeof(ramenPreferenceType)) {
			PrefGetAppPreferences(MYCREATORID,0,pref_addr,&prefsSize,false);
			if(TimGetSeconds()>pref_addr->BombTime) {
				pref_addr->TimerStarted=false;
			}
			if(!pref_addr->TimerStarted) {
				pref_addr->BombTime = 0;
			}
		}
	}
}

static void RamenSavePreferences() {
	PrefSetAppPreferences(MYCREATORID, 0, 1, &rp, sizeof(rp), false);
}

/* Function to play alarm sound; NO GLOBAL VARIABLE CAN BE USED!!! */
static void RamenPlaySound() {
	ramenPreferenceType lp;
	SndCommandType snd;
	snd.param1=1760;
	snd.param2=230;
	snd.cmd = sndCmdFreqDurationAmp;
	RamenLoadPreferences(&lp);
	if(lp.AlarmVolume<3) {
		snd.param3 = 16 + lp.AlarmVolume * 24;
	} else {
		const UInt16 prefver=PrefGetPreference(prefVersion);
		if(prefver<3) {
			const SoundLevelTypeV20 sl=PrefGetPreference(prefAlarmSoundLevelV20);
			snd.param3=(sl==slOn)?sndMaxAmp:0;
		} else {
			snd.param3=PrefGetPreference(prefAlarmSoundVolume);
		}
	}
	{
		const UInt16 ticks_per_sec = SysTicksPerSecond();
		unsigned int counter1=2;
		do {
			unsigned int counter0=2;
			do {
				SndDoCmd(NULL, &snd, true);
				SysTaskDelay(ticks_per_sec /45);
			} while(counter0--);
		} while(counter1--);
		SysTaskDelay(ticks_per_sec >> 5);
	}
}

UInt32 PilotMain(UInt16 cmd, MemPtr cmdPBP, UInt16 launchFlags) {
	if(cmd == sysAppLaunchCmdNormalLaunch) {
		EventType event;
		if(isTheDeviceSupported() == false) {
			FrmAlert(NOINTLMGRALERT);
			return 1;
		}
		/* Load my preferences */
		RamenLoadPreferences(&rp);
		
		FrmGotoForm(MainForm);
		do {
			UInt16 MenuError;
			EvtGetEvent(&event, SysTicksPerSecond()>>2);
			if (! SysHandleEvent(&event))
				if (! MenuHandleEvent(NULL, &event, &MenuError))
					if (! AppHandleEvent(&event))
						FrmDispatchEvent(&event);
		} while (event.eType != appStopEvent);
	} else if(cmd == sysAppLaunchCmdAlarmTriggered) {
		RamenPlaySound();
	} else if(cmd == sysAppLaunchCmdDisplayAlarm) {
		DisplayAlarm();
	}
	return 0;
}
