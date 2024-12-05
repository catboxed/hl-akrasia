#pragma once
#ifndef SOUNDREPLACEMENT_H
#define SOUNDREPLACEMENT_H

#include <map>
#include <string>

class SoundReplacementList
{
public:
	typedef std::map<std::string, std::string> ReplacementMap;
	SoundReplacementList();
	SoundReplacementList(ReplacementMap&& replacementMap);
	bool IsValid() const;
	const std::string& ReplacementFor(const std::string& soundName) const;
	void ReportReplacements() const;
private:
	ReplacementMap _replacementMap;
	bool _isValid;
};

class SoundReplacementSystem
{
public:
	bool EnsureReplacementFile(const char* fileName);
	const std::string& FindReplacement(const char* fileName, const char* originalSoundName) const;

	void ReportSoundReplacements() const;

	static const std::string emptyString;
private:
	std::map<std::string, SoundReplacementList> _fileMap;
};

extern SoundReplacementSystem g_soundReplacement;

#endif
