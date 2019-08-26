////////////////////////////////////////////////////////////////////////////////////////
/*
	Frank Engine Profiling Tool
	Copyright 2013 Frank Force - http://www.frankforce.com
*/
////////////////////////////////////////////////////////////////////////////////////////

#include "frankEngine.h"

bool FrankProfiler::showProfileDisplay = false;
ConsoleCommand(FrankProfiler::showProfileDisplay, profilerEnable);

void FrankProfiler::Render()
{
	FrankProfilerEntryDefine(L"FrankProfiler::Render()", Color::White(), 1000);

	list<FrankProfilerEntry*>& entries = GetEntries();
	if (!showProfileDisplay)
	{
		// clear out all the times
		for (list<FrankProfilerEntry*>::iterator it = entries.begin(); it != entries.end(); ++it) 
		{
			FrankProfilerEntry& entry = **it;
			entry.time = 0;
			entry.timeAverage = 0;
			entry.timeHigh = 0;
		}

		return;
	}

	const int nameGapSize = 400;
	const int timeGapSize = 150;
	int x = 400, y = 50;

	g_textHelper->Begin();
	g_textHelper->SetInsertionPos( x, y );
	g_textHelper->SetForegroundColor( Color::White() );
	g_textHelper->DrawFormattedTextLine( L"Profiler Entry Name" );
	g_textHelper->SetInsertionPos( x + nameGapSize, y);
	g_textHelper->DrawFormattedTextLine( L"Ave Time" );
	g_textHelper->SetInsertionPos( x + nameGapSize + timeGapSize, y );
	g_textHelper->DrawFormattedTextLine( L"High Time" );
	POINT insertionPos = g_textHelper->GetInsertionPos();
	g_textHelper->SetInsertionPos( x, insertionPos.y + 15 );

	{
		for (list<FrankProfilerEntry*>::iterator it = entries.begin(); it != entries.end(); ++it) 
		{
			FrankProfilerEntry& entry = **it;

			POINT insertionPos = g_textHelper->GetInsertionPos();
			g_textHelper->SetInsertionPos( x, insertionPos.y );
			g_textHelper->SetForegroundColor( entry.color );
			g_textHelper->DrawTextLine( entry.name);
			
			g_textHelper->SetInsertionPos( x + nameGapSize, insertionPos.y );
			g_textHelper->DrawFormattedTextLine( L"%.5f", 1000*entry.timeAverage );

			if (entry.timeHigh > 2*entry.timeAverage && 1000*entry.timeHigh > 0.1f)
			{
				// show spikes in red
				g_textHelper->SetForegroundColor( Color::Red() );
			}

			g_textHelper->SetInsertionPos( x + nameGapSize + timeGapSize, insertionPos.y );
			g_textHelper->DrawFormattedTextLine( L"%.5f", 1000*entry.timeHigh );
			
			if (GetFocus() == DXUTGetHWND())
			{
				if (entry.time > entry.timeHigh)
				{
					entry.timeHigh = entry.time;
					entry.timerHighTimer.Set();
				}
				if (entry.timerHighTimer > 1.0f)
					entry.timeHigh = entry.time;

				entry.timeAverage = 0.05f*entry.time + 0.95f*entry.timeAverage;
			}

			entry.time = 0;
		}
	}
	g_textHelper->End();
}

void FrankProfiler::AddEntry(FrankProfilerEntry& entry)
{
	list<FrankProfilerEntry*>& entries = GetEntries();

	entries.push_back(&entry); 
	entries.sort(FrankProfilerEntry::SortCompare);
}