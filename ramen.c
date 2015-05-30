#include <PalmOS.h>
#include <PalmChars.h>
#include <SonyChars.h>
#include "ramen.h"
#include "ramenrsc.h"
#include "ramenprefs.h"
#define DOWNALLOW			"\2"
#define UPALLOW				"\1"
#define DOWNDISABLEDALLOW	"\4"
#define UPDISABLEDALLOW		"\3"

static void PaintNumbers(Int32 ramen);
static void setRamenTo(Int32 ramen);
static void SetAlarm(void);
static void DisplayAlarm(void);
static Boolean refreshDisplay(void);
/*static void eraseArea(void);*/
static Boolean isTheDeviceSupported(void);
static void refreshButtons(void);
/* static void playMIDI(UInt16 itemNum); */
static Boolean RamenLoadPreferences(Int32 *LocalBombTime, Int32 *LocalRamenTime, Boolean *LocalTimerStarted, UInt32 *LocalButtonNumbers, UInt8 *LocalAlarmVolume);
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
	Boolean handled = false;
	FormType *frmP;
	Char thekey;
	
	switch (eventP->eType) {
	case menuEvent: 
		if (eventP->data.menu.itemID == MenuAboutRamen) {
			MenuEraseStatus(0);
			FrmAlert(AboutAlert);
			handled = true;
			break;
		} else if (eventP->data.menu.itemID == MenuPrefs) {
			DoPrefs(ButtonNumbers, &AlarmVolume);
			refreshButtons();
			RamenSavePreferences();
			handled = true;
			break;
		}
		break;
	case frmOpenEvent:
		frmP = FrmGetActiveForm();
		FrmDrawForm(frmP);
		handled = true;
		break;
	case ctlRepeatEvent:
		switch (eventP->data.ctlRepeat.controlID) {
		case REPEATUP:
			setRamenTo(RamenTime + 1);
			break; /* leave unhandled so buttons can repeat */
		case REPEATDOWN:
			setRamenTo(RamenTime - 1);
			break; /* leave unhandled so buttons can repeat */
		}
		break;
	case ctlSelectEvent:
		switch(eventP->data.ctlSelect.controlID) {
		case STARTBUTTON:
			SetAlarm();
			handled=true;
			break;
		case PRESET1:
		case PRESET2:
		case PRESET3:
		case PRESET4:
		case PRESET5:
			setRamenTo(ButtonNumbers[eventP->data.ctlSelect.controlID - PRESET1]);
			break;
		default:
			break;
		}
		break;
	case keyDownEvent:
		if(NavSelectPressed(eventP)) {
			SetAlarm();
			handled=true;
			break;
		}
		thekey = eventP->data.keyDown.chr;
		if (('0' <= thekey) && (thekey <= '9')) {
			if(RamenTime > 9) {
				setRamenTo(StrAToI(&thekey));
			} else {
				setRamenTo(RamenTime*10+StrAToI(&thekey));
			}
			handled = true;
			break;
		}
		switch (eventP->data.keyDown.chr) {
		case vchrPageUp:
		case vchrJogUp:
			setRamenTo(RamenTime+1);
			handled=true;
			break;
		case vchrPageDown:
		case vchrJogDown:
			setRamenTo(RamenTime-1);
			handled=true;
			break;
		case vchrJogPush:
			SetAlarm();
			handled = true;
			break;
		case vchrJogPushedUp:
			setRamenTo(RamenTime + 10);
			handled = true;
			break;
		case vchrJogPushedDown:
			setRamenTo(RamenTime - 10);
			handled = true;
			break;
		}
		break;
	case nilEvent:
		handled = refreshDisplay();
		break;
	default:
		break;
  }
  return handled;
}

static Boolean AppHandleEvent(EventPtr eventP) {
	Boolean handled = false;
	FormType *frmP;
	
	if (eventP->eType == frmLoadEvent) { /* Initialize and activate the form resource. */
		frmP = FrmInitForm(eventP->data.frmLoad.formID);
		FrmSetActiveForm(frmP);
		refreshButtons();
		if (eventP->data.frmLoad.formID == MainForm)
			FrmSetEventHandler(frmP, MainFormHandleEvent);
		handled = true;
	} else if (eventP->eType == frmOpenEvent) { /* Load the form resource. */
		frmP = FrmGetActiveForm();
		FrmDrawForm(frmP);
		PaintNumbers(RamenTime);
		refreshDisplay();
		handled = true;
	} else if (eventP->eType == appStopEvent) { /* Unload the form resource. */
		frmP = FrmGetActiveForm();
		FrmEraseForm(frmP);
		FrmDeleteForm(frmP);
		RamenSavePreferences();	/* Store my preferences*/
		handled = true;
	}
	
	return(handled);
}

static void setRamenTo(Int32 ramen) {
	ControlType *UpButtonP;
	ControlType *DownButtonP;
	FormType *frmP = FrmGetActiveForm();
	Boolean scrollableUp=true;
	Boolean scrollableDown=true;
	
	UpButtonP = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP,REPEATUP)); /*Get ptr to buttons */
	DownButtonP = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, REPEATDOWN));/*so we can disable them*/
	
	if (ramen <= MINTIME) {
		ramen = MINTIME;
		SndPlaySystemSound(sndWarning);
		scrollableUp=true;
		scrollableDown=false;
	}
	
	if (ramen >= MAXTIME) {
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

/*	PaintNumbers(Int32 ramen)
		->ramen : The number to be drawn; if 0, it only erases the display area.
		UPDATE: new version uses a field to avoid flickers
*/
static void PaintNumbers(Int32 ramen) {
	FormType *frmP = FrmGetActiveForm();
	UInt16 fldIndex = FrmGetObjectIndex(frmP, DISPLAY0);
	FieldType *fieldP = FrmGetObjectPtr(frmP, fldIndex);
	if(ramen == 0) {
		// FldSetUsable(fieldP, false);
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
	Err errID;
	SysCurAppDatabase(&cardNo, &dbID);
	if (TimerStarted == false) {
		if (BombTime == 0) {
			BombTime = TimGetSeconds() + RamenTime * 60;
		}
		if ((errID = AlmSetAlarm(cardNo, dbID, 0, BombTime, true)) != 0) {
			switch(errID) {
			case almErrMemory:
				FrmAlert(OUTOFMEMORYALERT);
				break;
			case almErrFull:
				FrmAlert(TABLEFULLALERT);
				break;
			}
		} else {
			TimerStarted = true;
		}
	} else { /* cancel timer */
		AlmSetAlarm(cardNo, dbID, 0, 0, true);
		TimerStarted = false;
	}
}

static void DisplayAlarm() {
	FormPtr frmP;
	
	MenuEraseStatus(0);
	frmP = FrmInitForm(ALARMFORM);
	FrmDoDialog(frmP);
	FrmEraseForm(frmP);
	FrmDeleteForm(frmP);
}

static Boolean refreshDisplay() {
	Int32 now;
	Int32 interval;
	Int32 timeremaining;
	Boolean blink = false;
	Boolean handled = false;
	if (TimerStarted == true) {
		now = TimGetSeconds();
		interval = now - lastTimeRefreshed;
		if ((interval > 1 )||(interval = 1)) {
			timeremaining = BombTime - now;
			if (timeremaining > 60) {
				timeremaining /= 60;
				blink=true;
			}
			if (timeremaining <= 0) {
				timeremaining=0;
				TimerStarted = false;
			lastTimeRefreshed = 0;
			BombTime = 0;
			}
			if (blink==true && (now & 0x1) == 1) {
				PaintNumbers(0);
			} else {
				PaintNumbers(timeremaining);
			}
			lastTimeRefreshed = now;
		}
		handled = true;
	}
	return handled;
}

static Boolean isTheDeviceSupported() {
	UInt32 romversion = 0;
	
	FtrGet(sysFtrCreator,sysFtrNumROMVersion, &romversion);
	if (romversion>=0x02003000) { /* PalmOS 2.0 */
		return true;
	} else {
		return false;
	}
}

static void refreshButtons() {
	int i;
	FormType *frmP = FrmGetActiveForm();
	ControlType *theButton;
	for(i=0; i<5; i++) {
		theButton = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP,PRESET1+i));
		
		StrIToA(ButtonLabels[i], ButtonNumbers[i]);
		CtlSetLabel(theButton, ButtonLabels[i]);
	}
}

/*
	Global-Free Preference Loader for AlarmVolume implementation
*/
static Boolean RamenLoadPreferences(LocalBombTime, LocalRamenTime, LocalTimerStarted, LocalButtonNumbers, LocalAlarmVolume)
	Int32 *LocalBombTime;
	Int32 *LocalRamenTime;
	Boolean *LocalTimerStarted;
	UInt32 *LocalButtonNumbers;
	UInt8 *LocalAlarmVolume;
{
	ramenPreferenceType ramenPrefs;
	UInt16 prefsSize = sizeof(ramenPreferenceType);
	UInt8 counter;
	Boolean Loaded = false;
	
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
			Loaded = true;
		}
	}
	return Loaded;
}

static void RamenSavePreferences() {
	ramenPreferenceType rPT1;
	UInt8 i;
	
	rPT1.BombTime = BombTime;
	rPT1.RamenTime = RamenTime;
	rPT1.TimerStarted = TimerStarted;
	rPT1.AlarmVolume = AlarmVolume;
	for(i=0; i<5; i++) {
		rPT1.Presets[i] = ButtonNumbers[i];
	}
	PrefSetAppPreferences(MYCREATORID, 0, 1, &rPT1, sizeof(rPT1), false);
}

/* Function to play alarm sound; NO GLOBAL VARIABLE CAN BE USED!!! */
static void RamenPlaySound() {
	Int32 junk[5];
	Boolean BoolJunk = false;
	UInt8 VolumeOfAlarm;
	SndCommandType snd;
	UInt8 counter0;
	UInt8 counter1;
	RamenLoadPreferences(junk, junk, &BoolJunk, junk, &VolumeOfAlarm);
	if(VolumeOfAlarm<3) {
		snd.param3 = 16 + VolumeOfAlarm * 24;
	} else {
		UInt16 prefver=PrefGetPreference(prefVersion);
		if(prefver<3) {
			SoundLevelTypeV20 sl;
			sl=PrefGetPreference(prefAlarmSoundLevelV20);
			snd.param3=(sl==slOn)?sndMaxAmp:0;
		} else {
			snd.param3=PrefGetPreference(prefAlarmSoundVolume);
		}
	}
	snd.param1=1760;
	snd.param2=230;
	snd.cmd = sndCmdFreqDurationAmp;
	for(counter1=0; counter1<3; counter1++) {
		for(counter0=0; counter0<3; counter0++) {
			SndDoCmd(NULL, &snd, true);
			SysTaskDelay(SysTicksPerSecond() / 45);
		}
		SysTaskDelay(SysTicksPerSecond() /30);
	}
}

UInt32 PilotMain(UInt16 cmd, MemPtr cmdPBP, UInt16 launchFlags) {
	EventType event;
	Err err = 0;

	switch (cmd) {
	case sysAppLaunchCmdNormalLaunch:
		if(isTheDeviceSupported() == false) {
			FrmAlert(NOINTLMGRALERT);
			break;
		}
		/* Load my preferences */
		RamenLoadPreferences(&BombTime, &RamenTime, &TimerStarted, ButtonNumbers, &AlarmVolume);
		
		FrmGotoForm(MainForm);
		do {
			UInt16 MenuError;
			EvtGetEvent(&event, SysTicksPerSecond()/3);
			if (! SysHandleEvent(&event))
				if (! MenuHandleEvent(0, &event, &MenuError))
					if (! AppHandleEvent(&event))
						FrmDispatchEvent(&event);
		} while (event.eType != appStopEvent);
		break;
	case sysAppLaunchCmdAlarmTriggered:
		//SndPlaySystemSound(sndAlarm);
		RamenPlaySound();
		break;
	case sysAppLaunchCmdDisplayAlarm:
		DisplayAlarm();
		break;
	default:
		break;
	}    
	return(err);
}
