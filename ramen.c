#include <PalmOS.h>
#include <PalmChars.h>
#include <SonyChars.h>
#include "ramen.h"
#include "ramenrsc.h"
#include "ramenprefs.h"

static void PaintNumbers(Int32 ramen);
static void setRamenTo(Int32 ramen);
static void SetAlarm(void);
static void DisplayAlarm(void);
static void refreshDisplay(void);
static Boolean isTheDeviceSupported(void);
static void refreshButtons(void);
static void RamenLoadPreferences(Int32 *LocalBombTime, Int32 *LocalRamenTime, Boolean *LocalTimerStarted, UInt32 *LocalButtonNumbers, UInt8 *LocalAlarmVolume);
static void RamenSavePreferences(void);
static void RamenPlaySound(void);

/* var */
Int32 RamenTime=DEFTIME;
Boolean TimerStarted = false;
UInt32 BombTime = 0;
UInt32 lastTimeRefreshed = 0;
UInt32 ButtonNumbers[5] = {1, 3, 5, 10, 60};
Char ButtonLabels[5][3];
Char NumLabel[3];
UInt8 AlarmVolume = 3;

/* implementation */
static Boolean MainFormHandleEvent(EventPtr eventP) {
	const eventsEnum eType=eventP->eType;
	if(eType == menuEvent) {
		const UInt16 itemID = eventP->data.menu.itemID;
		if (itemID == MenuAboutRamen) {
			MenuEraseStatus(0);
			FrmAlert(AboutAlert);
		} else if (itemID == MenuPrefs) {
			DoPrefs(ButtonNumbers, &AlarmVolume);
			refreshButtons();
			RamenSavePreferences();
		}
	} else if(eType == ctlRepeatEvent) {
		const UInt16 controlID = eventP->data.ctlRepeat.controlID;
		if(controlID == REPEATUP) {
			setRamenTo(RamenTime + 1);
		} else if(controlID == REPEATDOWN) {
			setRamenTo(RamenTime - 1);
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
			setRamenTo(ButtonNumbers[eventP->data.ctlSelect.controlID - PRESET1]);
		}
	} else if(eType == keyDownEvent) {
		const WChar thekey =  eventP->data.keyDown.chr;
		if ((L'0' <= thekey) && (thekey <= L'9')) {
			const Int32 digit=thekey - L'0';
			if(RamenTime > 9) {
				setRamenTo(digit);
			} else {
				setRamenTo(RamenTime*10+digit);
			}
		} else if(thekey == vchrPageUp ||
			  thekey == vchrJogUp) {
			setRamenTo(RamenTime+1);
		} else if(thekey == vchrPageDown ||
			  thekey == vchrJogDown) {
			setRamenTo(RamenTime-1);
		} else if(NavSelectPressed(eventP) || 
			  thekey == vchrJogPush) {
			SetAlarm();
		} else if(thekey == vchrJogPushedUp) {
			setRamenTo(RamenTime + 10);
		} else if(thekey == vchrJogPushedDown) {
			setRamenTo(RamenTime - 10);
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
		refreshButtons();
		if (eventP->data.frmLoad.formID == MainForm) {
			FrmSetEventHandler(frmP, MainFormHandleEvent);
		}
	} else if (eventP->eType == frmOpenEvent) { /* Load the form resource. */
		const FormPtr frmP = FrmGetActiveForm();
		FrmDrawForm(frmP);
		PaintNumbers(RamenTime);
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
	RamenTime = ramen;
	BombTime = 0; /*why did i add this code?*/
	TimerStarted=true; /* Tell SetAlarm() to cancel any previously registered timer */
	SetAlarm();
	PaintNumbers(RamenTime);
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

/* PaintNumbers(Int32 ramen)
	->ramen : The number to be drawn; if 0, it only erases the display area.
	UPDATE: new version uses a field to avoid flickers
*/
static void PaintNumbers(Int32 ramen) {
	const FormPtr frmP = FrmGetActiveForm();
	const UInt16 fldIndex = FrmGetObjectIndex(frmP, DISPLAY0);
	const FieldPtr fieldP = FrmGetObjectPtr(frmP, fldIndex);
	if(!ramen) {
		FrmHideObject(frmP, fldIndex);
	} else {
		FldSetInsertionPoint(fieldP,0);
		FldSetSelection(fieldP,0,0);
		StrIToA(NumLabel, ramen);
		FldSetTextPtr(fieldP, NumLabel);
		FrmShowObject(frmP, fldIndex);
	}
}

static void SetAlarm() {
	UInt16 cardNo;
	LocalID dbID;
	SysCurAppDatabase(&cardNo, &dbID);
	if (TimerStarted == false) {
		if (!BombTime) {
			BombTime = TimGetSeconds() + RamenTime * 60;
		}
		{
			const Err errID=AlmSetAlarm(cardNo, dbID, 0, BombTime, true);
			if(errID == almErrMemory) {
				FrmAlert(OUTOFMEMORYALERT);
			} else if(errID == almErrFull) {
				FrmAlert(TABLEFULLALERT);
			} else {
				TimerStarted = true;
			}
		}
	} else { /* cancel timer */
		AlmSetAlarm(cardNo, dbID, 0, 0, true);
		TimerStarted = false;
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
	if (TimerStarted == true) {
		const Int32 now = TimGetSeconds();
		const Int32 interval = now - lastTimeRefreshed;
		if (interval) {
			const Int32 timeremaining = BombTime - now;
			Int32 dispTime;
			if (timeremaining > 60) {
				if(now&0x1) {
					dispTime=0;
				} else {
					dispTime = timeremaining / 60;
				}
			} else if (timeremaining <= 0) {
				dispTime=RamenTime;
				TimerStarted = false;
				lastTimeRefreshed = 0;
				BombTime = 0;
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
		StrIToA(ButtonLabels[i], ButtonNumbers[i]);
		CtlSetLabel(theButton, ButtonLabels[i]);
	} while(i--);
}

/*
	Global-Free Preference Loader for AlarmVolume implementation
*/
static void RamenLoadPreferences(LocalBombTime, LocalRamenTime, LocalTimerStarted, LocalButtonNumbers, LocalAlarmVolume)
	Int32 *LocalBombTime;
	Int32 *LocalRamenTime;
	Boolean *LocalTimerStarted;
	UInt32 *LocalButtonNumbers;
	UInt8 *LocalAlarmVolume;
{
	ramenPreferenceType ramenPrefs;
	UInt16 prefsSize = sizeof(ramenPreferenceType);
	UInt8 counter;
	
	if (PrefGetAppPreferences(MYCREATORID, 0, &ramenPrefs, &prefsSize, false) != noPreferenceFound) {
		if (prefsSize == sizeof(ramenPreferenceType)) {
			*LocalBombTime = ramenPrefs.BombTime;
			*LocalRamenTime = ramenPrefs.RamenTime;
			for (counter=0; counter<5; counter++) {
				*(LocalButtonNumbers + counter) = ramenPrefs.Presets[counter];
			}
			*LocalAlarmVolume = ramenPrefs.AlarmVolume;
			if(!(TimGetSeconds() > *LocalBombTime)) {
				*LocalTimerStarted = ramenPrefs.TimerStarted;
			}
			if(*LocalTimerStarted == false) {
				*LocalBombTime = 0;
			}
		}
	}
}

static void RamenSavePreferences() {
	ramenPreferenceType rPT1;
	
	rPT1.BombTime = BombTime;
	rPT1.RamenTime = RamenTime;
	rPT1.TimerStarted = TimerStarted;
	rPT1.AlarmVolume = AlarmVolume;
	{
		unsigned int i=4; do {
			rPT1.Presets[i] = ButtonNumbers[i];
		} while(i--);
	}
	PrefSetAppPreferences(MYCREATORID, 0, 1, &rPT1, sizeof(rPT1), false);
}

/* Function to play alarm sound; NO GLOBAL VARIABLE CAN BE USED!!! */
static void RamenPlaySound() {
	Int32 junk[5];
	Boolean BoolJunk = false;
	UInt8 VolumeOfAlarm;
	SndCommandType snd;
	RamenLoadPreferences(junk, junk, &BoolJunk, junk, &VolumeOfAlarm);
	if(VolumeOfAlarm<3) {
		snd.param3 = 16 + VolumeOfAlarm * 24;
	} else {
		const UInt16 prefver=PrefGetPreference(prefVersion);
		if(prefver<3) {
			const SoundLevelTypeV20 sl=PrefGetPreference(prefAlarmSoundLevelV20);
			snd.param3=(sl==slOn)?sndMaxAmp:0;
		} else {
			snd.param3=PrefGetPreference(prefAlarmSoundVolume);
		}
	}
	snd.param1=1760;
	snd.param2=230;
	snd.cmd = sndCmdFreqDurationAmp;
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
		RamenLoadPreferences(&BombTime, &RamenTime, &TimerStarted, ButtonNumbers, &AlarmVolume);
		
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
