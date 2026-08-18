#ifndef AQUILA_EDITOR_DEMO_SYNC_TIMELINE_H
#define AQUILA_EDITOR_DEMO_SYNC_TIMELINE_H
#pragma once

#include <glm/glm.hpp>

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace demo
{
	struct Keyframe
	{
		enum class Interp
		{
			Step,   
			Linear,
			Smooth  
		};

		float     row = 0.0f;
		glm::vec4 value = glm::vec4(0.0f);
		Interp    interp = Interp::Linear;
	};

	struct Track
	{
		std::string name;
		std::vector<Keyframe> keyframes;

		glm::vec4 Evaluate(float _row) const;
		void AddKeyframe(float _row, const glm::vec4& _value, Keyframe::Interp _interp = Keyframe::Interp::Linear);
	};

	class SyncTimeline
	{
	public:
		bool LoadFromFile(const std::string& _path);
		bool SaveToFile(const std::string& _path) const;

		void CreateNewFile(
			const std::string& _path,
			const std::vector<std::string>& _meshTrackNames,
			const std::vector<glm::vec3>& _meshPositions,
			const glm::vec3& _cameraPos,
			const glm::vec3& _cameraTarget,
			const glm::vec3& _sunDir);

		bool ReloadIfChanged();

		float RowFromTime(float _time) const
		{
			return _time * (bpm / 60.0f) * static_cast<float>(rowsPerBeat);
		}

		glm::vec4 Evaluate(const std::string& _trackName, float _row) const;

		void AddKeyframeAndSave(
			const std::string& _trackName,
			float _row,
			const glm::vec4& _value,
			Keyframe::Interp _interp = Keyframe::Interp::Linear);

		bool     HasFile() const { return !currentPath.empty(); }
		float    GetBPM() const { return bpm; }
		uint32_t GetRowsPerBeat() const { return rowsPerBeat; }

	private:
		Track& GetOrCreateTrack(const std::string& _name);

		std::unordered_map<std::string, Track> tracks;

		float    bpm = 130.0f;
		uint32_t rowsPerBeat = 8;

		std::string currentPath;
		std::filesystem::file_time_type lastWriteTime{};
	};
}

#endif // AQUILA_EDITOR_DEMO_SYNC_TIMELINE_H