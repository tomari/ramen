typedef struct {
	Int32 BombTime;
	Int32 RamenTime;
	Boolean TimerStarted;
	UInt32 Presets[5];
	UInt8 AlarmVolume;
} ramenPreferenceType;

#define MAXTIME 99
#define MINTIME 1
#define DEFTIME 3
#define MYCREATORID 'Ramn'
#define DISPLAYX 60
#define DISPLAYY 44
