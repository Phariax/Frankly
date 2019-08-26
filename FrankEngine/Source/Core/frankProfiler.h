////////////////////////////////////////////////////////////////////////////////////////
/*
	Frank Engine Profilier
	Copyright 2013 Frank Force - http://www.frankforce.com
*/
////////////////////////////////////////////////////////////////////////////////////////

#ifndef FRANK_PROFILER_H
#define FRANK_PROFILER_H

#include <list>

////////////////////////////////////////////////////////////////////////////////////////

// use this macro to easily add profile entries
#define FrankProfilerEntryDefine(name, color, sortOrder) FrankProfilerEntryDefineInternal1(name, color, sortOrder, __COUNTER__)

////////////////////////////////////////////////////////////////////////////////////////

struct FrankProfilerEntry;

struct FrankProfiler 
{
	static void Render();
	static void AddEntry(FrankProfilerEntry& entry);
	static void ToggleDisplay() { showProfileDisplay = !showProfileDisplay; }
	
	static list<FrankProfilerEntry*>& GetEntries()
	{
		// allow declaring profile entries as global statics
		static list<FrankProfilerEntry*> entries;
		return entries;
	}

	static bool showProfileDisplay;
};

/////////////////////////////////////////////////////////////////////////////////////////////
// helper classes for profiler, should not normally be instanced directly, use macro instead

struct FrankProfilerEntry 
{
	FrankProfilerEntry(const WCHAR* _name, const Color& _color = Color::White(), int _sortOrder = 0) : 
		name(_name), 
		color(_color), 
		sortOrder(_sortOrder),
		time(0),
		timeAverage(0),
		timeHigh(0)
	{
		FrankProfiler::AddEntry(*this);
	}

	void AddTime(float deltaTime) { time += deltaTime; }
	static bool SortCompare(FrankProfilerEntry* first, FrankProfilerEntry* second) { return (first->sortOrder < second->sortOrder); }

	const WCHAR* name;
	const Color color;
	const int sortOrder;
	float time;
	float timeAverage;
	float timeHigh;
	GameTimer timerHighTimer;
};

struct FrankProfilerBlockTimer
{
	FrankProfilerBlockTimer(FrankProfilerEntry& _entry) : entry(_entry) { timer.Start(); }
	~FrankProfilerBlockTimer() { entry.AddTime(timer.GetElapsedTime()); }

	FrankProfilerEntry& entry;
	CDXUTTimer timer;
};

/////////////////////////////////////////////////////////////////////////////////////////////
// internal macros, should not be used by clients

#define FrankProfilerEntryDefineInternal1(name, color, sortOrder, id) FrankProfilerEntryDefineInternal2(name, color, sortOrder, id)
#define FrankProfilerEntryDefineInternal2(name, color, sortOrder, id)	\
static FrankProfilerEntry _profilerEntry##id(name, color, sortOrder);	\
	FrankProfilerBlockTimer _profilerBlock##id(_profilerEntry##id);

#endif // FRANK_PROFILER_H