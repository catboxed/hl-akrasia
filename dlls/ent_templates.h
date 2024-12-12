#pragma once
#ifndef ENT_TEMPLATES_H
#define ENT_TEMPLATES_H

#include "vector.h"
#include "visuals.h"
#include "soundscripts.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

struct EntTemplate
{
public:
	const char* OwnVisualName() const;
	void SetOwnVisualName(const std::string& name) {
		_ownVisual = name;
	}

	const char* GibVisualName() const;
	void SetGibVisualName(const std::string& name) {
		_gibVisual = name;
	}

	const char* GetSoundScriptNameOverride(const char* name) const;
	void SetSoundScriptReplacement(const char* soundScript, const std::string& replacement);

	const char* GetVisualNameOverride(const char* name) const;
	void SetVisualReplacement(const char* visual, const std::string& replacement);

	const char* GetSoundReplacement(const char* originalSample) const;
	void SetSoundReplacement(const char* originalSample, const char* replacementSample);

	inline std::vector<std::string>::const_iterator PrecachedSoundsBegin() const {
		return _precachedSounds.begin();
	}
	inline std::vector<std::string>::const_iterator PrecachedSoundsEnd() const {
		return _precachedSounds.end();
	}
	void SetPrecachedSounds(std::vector<std::string>&& sounds);

	inline std::vector<std::string>::const_iterator PrecachedSoundScriptsBegin() const {
		return _precachedSoundScripts.begin();
	}
	inline std::vector<std::string>::const_iterator PrecachedSoundScriptsEnd() const {
		return _precachedSoundScripts.end();
	}
	void SetPrecachedSoundScripts(std::vector<std::string>&& soundScripts);

	inline bool AutoPrecacheSounds() const {
		return _autoprecachedSounds;
	}
	inline void SetAutoPrecacheSounds(bool b) {
		_autoprecachedSounds = b;
	}
	inline bool AutoPrecacheSoundScripts() const {
		return _autoprecachedSoundScripts;
	}
	inline void SetAutoPrecacheSoundScripts(bool b) {
		_autoprecachedSoundScripts = b;
	}

	bool IsClassifyDefined() const {
		return (_defined & CLASSIFY_DEFINED) != 0;
	}
	int Classify() const {
		return _classify;
	}
	void SetClassify(int classify) {
		_defined |= CLASSIFY_DEFINED;
		_classify = classify;
	}

	bool IsBloodDefined() const {
		return (_defined & BLOOD_DEFINED) != 0;
	}
	int BloodColor() const {
		return _bloodColor;
	}
	void SetBloodColor(int bloodColor) {
		_defined |= BLOOD_DEFINED;
		_bloodColor = bloodColor;
	}

	bool IsHealthDefined() const {
		return (_defined & HEALTH_DEFINED) != 0;
	}
	float Health() const {
		return _health;
	}
	void SetHealth(float health) {
		_defined |= HEALTH_DEFINED;
		_health = health;
	}

	bool IsFielfOfViewDefined() const {
		return (_defined & FIELDOFVIEW_DEFINED) != 0;
	}
	float FieldOfView() const {
		return _fieldOfView;
	}
	void SetFieldOfView(float fieldOfView) {
		_defined |= FIELDOFVIEW_DEFINED;
		_fieldOfView = fieldOfView;
	}

	bool IsSizeDefined() const {
		return (_defined & SIZE_DEFINED) != 0;
	}
	Vector MinSize() const {
		return _minSize;
	}
	Vector MaxSize() const {
		return _maxSize;
	}
	void SetSize(const Vector& minSize, const Vector& maxSize)
	{
		_defined |= SIZE_DEFINED;
		_minSize = minSize;
		_maxSize = maxSize;
	}

	bool IsSizeForGrappleDefined() const {
		return (_defined & SIZEFORGRAPPLE_DEFINED) != 0;
	}
	int SizeForGrapple() const {
		return _sizeForGrapple;const char* SoundReplacementFor(const char* sound);
	}
	void SetSizeForGrapple(int sizeForGrapple)
	{
		_defined |= SIZEFORGRAPPLE_DEFINED;
		_sizeForGrapple = sizeForGrapple;
	}

	const char* SpeechPrefix() const;
	void SetSpeechPrefix(const std::string& speechPrefix) {
		_speechPrefix = speechPrefix;
	}
private:
	std::map<std::string, std::string> _soundScripts;
	std::map<std::string, std::string> _visuals;
	std::string _ownVisual;
	std::string _gibVisual;

	std::map<std::string, std::string> _soundReplacements;
	std::vector<std::string> _precachedSounds;
	std::vector<std::string> _precachedSoundScripts;
	bool _autoprecachedSounds = false;
	bool _autoprecachedSoundScripts = false;

	enum
	{
		CLASSIFY_DEFINED = (1 << 0),
		BLOOD_DEFINED = (1 << 1),
		HEALTH_DEFINED = (1 << 2),
		FIELDOFVIEW_DEFINED = (1 << 3),
		SIZE_DEFINED = (1 << 4),
		SIZEFORGRAPPLE_DEFINED = (1 << 5),
	};

	int _defined = 0;
	int _classify = 0;
	int _bloodColor = 0;
	float _health = 0.0f;
	float _fieldOfView = 0.0f;
	Vector _minSize = Vector(0,0,0);
	Vector _maxSize = Vector(0,0,0);
	short _sizeForGrapple = 0;

	std::string _speechPrefix;
};

void ReadEntTemplates();

const EntTemplate* GetEntTemplate(const char* name);

void EnsureVisualReplacementForTemplate(const char* templateName, const char* visualName);

#endif
