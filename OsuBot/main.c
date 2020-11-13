#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include "Windows.h"

#pragma pack(push, 1)
typedef struct _MapDifficulty
{
	double HPDrainRate;
	double CircleSize;
	double OverallDifficulty;
	double ApproachRate;
	double SliderMultiplier;
	double SliderTickRate;
} MapDifficulty;

typedef struct _TimingPoint
{
	int time;
	double beatLength;
	int meter;
	int sampleSet;
	int sampleIndex;
	int volume;
	bool uninherited;
	int effects;
} TimingPoint;

typedef struct _HitObject
{
	int x;
	int y;
	int time;
	int type;
	int endTime; //for spinners (index 5)
	int slides; //for sliders (index 6)
	double length; //for sliders (index 7)	
} HitObject;

typedef struct _TimedObject
{
	HitObject* hitObject;
	TimingPoint* timingPoint;
} TimedObject;
#pragma pack(pop)

int GenRand(int lower, int upper)
{
	return (rand() % (upper - lower + 1)) + lower;
}

int ReadMapParams(IN char* beatMapLocation, OUT MapDifficulty* mapDifficulty, OUT HitObject** hitObjects, OUT int* hiCount, OUT TimingPoint** timingPoints, OUT int* tiCount)
{
	if (!hitObjects || !timingPoints || !beatMapLocation)
		return 0x1;
	
	FILE* file;
	fopen_s(&file, beatMapLocation, "r");

	if (!file)
		return 0x2;

	//get HitObject count...
	int hoCount = 0, tpCount = 0, readFlag = 0, readTimeingPointsFlag = 0;
	char line[4098];
	while (fgets(line, 4098, file))
	{
		if (readTimeingPointsFlag && !strcmp(line, "\n")) { readTimeingPointsFlag = 0; }
		if (readTimeingPointsFlag) { tpCount++; }

		if (readFlag) { hoCount++; }
		
		if (strstr(line, "[HitObjects]"))
		{
			readFlag = 1;
		}

		if (strstr(line, "[TimingPoints]"))
		{
			readTimeingPointsFlag = 1;
		}
	}

	if (hoCount <= 0)
		return 0x3;
	if (tpCount <= 0)
		return 0x4;
	readFlag = 0;
	readTimeingPointsFlag = 0;
	rewind(file);

	MapDifficulty _mapDifficulty = {0};
	HitObject* _hitObjects = malloc(hoCount * sizeof(HitObject));
	TimingPoint* _timingPoints = malloc(tpCount * sizeof(TimingPoint));
	
	int x = 0, y = 0, z = 0, readDifficulty = 0;
	while (fgets(line, 4098, file))
	{
		if (readDifficulty)
		{
			if (!strcmp(line, "\n"))
			{
				readDifficulty = 0;
				goto skip;
			}

			int j = 0;
			for (const char* tok = strtok(line, ":"); tok && *tok; j++, tok = strtok(NULL, ":\n"))
			{
				if(j == 1)
				{
					switch (z)
					{
					case 0:
						_mapDifficulty.HPDrainRate = atof(tok);
						break;
					case 1:
						_mapDifficulty.CircleSize = atof(tok);
						break;
					case 2:
						_mapDifficulty.OverallDifficulty = atof(tok);
						break;
					case 3:
						_mapDifficulty.ApproachRate = atof(tok);
						break;
					case 4:
						_mapDifficulty.SliderMultiplier = atof(tok);
						break;
					case 5:
						_mapDifficulty.SliderTickRate = atof(tok);
						break;
					default: ; //ignore
					}
				}
			}
			z++;
		}
		
		if (readTimeingPointsFlag)
		{
			if (!strcmp(line, "\n"))
			{
				readTimeingPointsFlag = 0;
				goto skip;
			}

			int j = 0;
			for (const char* tok = strtok(line, ","); tok && *tok; j++, tok = strtok(NULL, ",\n"))
			{
				switch (j)
				{
				case 0:
					_timingPoints[y].time = atoi(tok);
					break;
				case 1:
					_timingPoints[y].beatLength = atof(tok);
					break;
				case 2:
					_timingPoints[y].meter = atoi(tok);
					break;
				case 3:
					_timingPoints[y].sampleSet = atoi(tok);
					break;
				case 4:
					_timingPoints[y].sampleIndex = atoi(tok);
					break;
				case 5:
					_timingPoints[y].uninherited = (byte)atoi(tok);
					break;
				case 6:
					_timingPoints[y].effects = atoi(tok);
					break;
				default: ; //ignore
				}
			}
			y++;
		}

		if (readFlag)
		{		
			int j = 0;
			for (const char* tok = strtok(line, ","); tok && *tok; j++, tok = strtok(NULL, ",\n"))
			{
				switch (j)
				{
				case 0:
					_hitObjects[x].x = atoi(tok);
					break;
				case 1:
					_hitObjects[x].y = atoi(tok);
					break;
				case 2:
					_hitObjects[x].time = atoi(tok);
					break;
				case 3:
					_hitObjects[x].type = atoi(tok);
					break;
				case 5:
					_hitObjects[x].endTime = atoi(tok);
					break;
				case 6:
					_hitObjects[x].slides = atoi(tok);
					break;
				case 7:
					_hitObjects[x].length = atof(tok);
					break;
				default: ; //ignore
				}
			}
			x++;
		}

		if (strstr(line, "[Difficulty]"))
		{
			readDifficulty = 1;
		}
		
		if (strstr(line, "[TimingPoints]"))
		{
			readTimeingPointsFlag = 1;
		}
		
	skip:
		if (strstr(line, "[HitObjects]"))
		{
			readFlag = 1;
		}
	}

	if (mapDifficulty)
		*mapDifficulty = _mapDifficulty;
	*hitObjects = _hitObjects;
	*timingPoints = _timingPoints;
	if (hiCount)
		*hiCount = hoCount;
	if (tiCount)
		*tiCount = tpCount;
	fclose(file);
	return 0;
}

int main()
{
	SetConsoleTitleA("");
	
	char* beatMapFile = "C:\\Users\\Daniel\\AppData\\Local\\osu!\\Songs\\444079 SawanoHiroyuki[nZk]_Tielle - Into the Sky (TV size)\\SawanoHiroyuki[nZk]Tielle - Into the Sky (TV size) (TT Mouse) [Kencho's Mobile Suit].osu";

	int hCount = 0, tCount = 0;
	MapDifficulty mapDifficulty; HitObject* hitObjects; TimingPoint* timingPoints;
	const int status = ReadMapParams(beatMapFile, &mapDifficulty, &hitObjects, &hCount, &timingPoints, &tCount);
	//printf("FileReader returned: %d\n", status);
	if (status != 0)
		return 0;
	printf("Beatmap ready!");
	//printf("hCount: %llu\ntCount: %llu\n", hCount, tCount);

	//printf("SliderMultiplier: %f", mapDifficulty.SliderMultiplier);
	
	/*for (int i = 0; i < tCount; ++i)
		printf("t: { %d, %f, %d }\n", timingPoints[i].time, timingPoints[i].beatLength, timingPoints[i].uninherited);

	for (int i = 0; i < hCount; ++i)
		printf("h: { %d, %d, %d, %d }\n", hitObjects[i].x, hitObjects[i].y, hitObjects[i].time, hitObjects[i].type);*/
	
	const int BeatMapEnd = hitObjects[hCount - 1].time;

	//assign timingPoints for HitObjects:
	TimedObject* timedObjects = malloc(hCount * sizeof(TimedObject));
	for (int i = 0; i < hCount; ++i)
	{
		timedObjects[i].hitObject = &hitObjects[i];
		for (int j = tCount - 1; j >= 0; --j)
		{
			if (timingPoints[j].time <= hitObjects[i].time)
			{
				timedObjects[i].timingPoint = &timingPoints[j];
				break;
			}
		}
		//nullpointer security
		if (!timedObjects[i].timingPoint)
		{
			timedObjects[i].timingPoint = &timingPoints[0];
		}
	}

	/*for (int i = 0; i < hCount; ++i)
		printf("to: { %d, %d }\n", timedObjects[i].hitObject->time, timedObjects[i].timingPoint->time);*/
	
	int rstFlag = 0, keySwitchFlag = 1;
	while (true)
	{
		if (!rstFlag && GetAsyncKeyState(VK_SPACE) & 0x8000)
		{
			printf("Playing new map!\n");
			const clock_t before = clock();
			rstFlag = 1;

			int ms;
			int count = 0;
			int i = 0;
			double beatLength = timingPoints[0].beatLength;
			do
			{
				if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) { printf("Aborted!\n"); break; }
				
				const clock_t difference = clock() - before;
				ms = difference * 1000 / CLOCKS_PER_SEC;
				
				if (timedObjects[i].hitObject->time == ms)
				{
					count++;
					Sleep(GenRand(0, 4));
					keybd_event(keySwitchFlag ? 0x43 /*C*/ : 0x56 /*V*/, 0x38, 0, 0);

					if (((timedObjects[i].hitObject->type >> 1) & 1) == 1) //it's a slider
					{
						printf("%d | slider: hit! - %d :", timedObjects[i].hitObject->time, count);
						double _beatLength = beatLength;
						double velocity = 1;

						if (timedObjects[i].timingPoint->uninherited)
						{
							_beatLength = beatLength = timedObjects[i].timingPoint->beatLength;
						}
						else if (!timedObjects[i].timingPoint->uninherited)
						{
							//_beatLength *= abs((int)timedObjects[i].timingPoint->beatLength)/100.0;
						}
						
						const double sliderLength = timedObjects[i].hitObject->length * timedObjects[i].hitObject->slides;

						Sleep((int)(sliderLength / (mapDifficulty.SliderMultiplier * 100) * _beatLength));
						printf("%d", (int)(sliderLength / (mapDifficulty.SliderMultiplier * 100) * _beatLength));
					}
					else if (((timedObjects[i].hitObject->type >> 3) & 1) == 1) //it's a spinner
					{
						printf("%d | spinner: hit! - %d :", timedObjects[i].hitObject->time, count);
						Sleep(timedObjects[i].hitObject->endTime - timedObjects[i].hitObject->time + GenRand(2, 14));
						printf("%d", timedObjects[i].hitObject->endTime - timedObjects[i].hitObject->time);
					}
					else if ((timedObjects[i].hitObject->type & 1) == 1)
					{
						printf("%d | circle: hit! - %d :", timedObjects[i].hitObject->time, count);
						Sleep(GenRand(7, 10));
						printf("%d", GenRand(7, 10));
					}

					printf(": release!\n");
					keybd_event(keySwitchFlag ? 0x43 /*C*/ : 0x56 /*V*/, 0x38, KEYEVENTF_KEYUP, 0);
					keySwitchFlag = !keySwitchFlag;
					if (i < hCount - 1)
						i++;
				}
			}
			while (ms < BeatMapEnd+1);
			printf("Finished beatmap!\n");
		}
		if (GetAsyncKeyState(VK_SPACE) == 0) { rstFlag = 0; }
	}
}
