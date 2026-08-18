#include <demo/syncTimeline.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace demo;

std::string InterpToString(Keyframe::Interp _interp)
{
	switch (_interp)
	{
	case Keyframe::Interp::Step:   return "step";
	case Keyframe::Interp::Smooth: return "smooth";
	case Keyframe::Interp::Linear:
	default:                       return "linear";
	}
}

Keyframe::Interp InterpFromString(const std::string& _str)
{
	std::string lower = _str;
	std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return static_cast<char>(::tolower(c)); });

	if (lower == "step")
		return Keyframe::Interp::Step;

	if (lower == "smooth")
		return Keyframe::Interp::Smooth;

	return Keyframe::Interp::Linear;
}

glm::vec4 ParseValue(const std::string& _str)
{
	glm::vec4 value(0.0f);

	std::istringstream stream(_str);
	std::string component;
	int idx = 0;

	while (std::getline(stream, component, ',') && idx < 4)
	{
		try
		{
			value[idx] = std::stof(component);
		}
		catch (const std::exception&)
		{
			value[idx] = 0.0f;
		}

		++idx;
	}

	return value;
}

std::string FormatValue(const glm::vec4& _value)
{
	std::ostringstream stream;
	stream << _value.x << "," << _value.y << "," << _value.z << "," << _value.w;
	return stream.str();
}


glm::vec4 Track::Evaluate(float _row) const
{
	if (keyframes.empty())
		return glm::vec4(0.0f);

	if (_row <= keyframes.front().row)
		return keyframes.front().value;

	if (_row >= keyframes.back().row)
		return keyframes.back().value;

	for (size_t i = 0; i + 1 < keyframes.size(); ++i)
	{
		const Keyframe& a = keyframes[i];
		const Keyframe& b = keyframes[i + 1];

		if (_row < a.row || _row > b.row)
			continue;

		const float span = b.row - a.row;
		float t = span > 0.0f ? (_row - a.row) / span : 0.0f;

		switch (a.interp)
		{
		case Keyframe::Interp::Step:
			return a.value;

		case Keyframe::Interp::Smooth:
			t = t * t * (3.0f - 2.0f * t);
			return glm::mix(a.value, b.value, t);

		case Keyframe::Interp::Linear:
		default:
			return glm::mix(a.value, b.value, t);
		}
	}

	return keyframes.back().value;
}

void Track::AddKeyframe(float _row, const glm::vec4& _value, Keyframe::Interp _interp)
{
	for (Keyframe& kf : keyframes)
	{
		if (std::abs(kf.row - _row) < 0.0001f)
		{
			kf.value = _value;
			kf.interp = _interp;
			return;
		}
	}

	keyframes.push_back({ _row, _value, _interp });

	std::sort(keyframes.begin(), keyframes.end(), [](const Keyframe& a, const Keyframe& b)
		{
			return a.row < b.row;
		});
}

Track& SyncTimeline::GetOrCreateTrack(const std::string& _name)
{
	auto it = tracks.find(_name);

	if (it != tracks.end())
		return it->second;

	Track track;
	track.name = _name;

	auto [inserted, ok] = tracks.emplace(_name, std::move(track));
	return inserted->second;
}

bool SyncTimeline::LoadFromFile(const std::string& _path)
{
	std::ifstream file(_path);

	if (!file.is_open())
	{
		std::cerr << "SyncTimeline: failed to open '" << _path << "'\n";
		return false;
	}

	tracks.clear();

	std::string line;

	while (std::getline(file, line))
	{
		const size_t start = line.find_first_not_of(" \t\r\n");

		if (start == std::string::npos)
			continue; 

		if (line[start] == '#')
		{
			std::istringstream headerStream(line.substr(start + 1));
			std::string token;

			while (headerStream >> token)
			{
				const size_t eq = token.find('=');

				if (eq == std::string::npos)
					continue;

				const std::string key = token.substr(0, eq);
				const std::string val = token.substr(eq + 1);

				try
				{
					if (key == "bpm")
						bpm = std::stof(val);
					else if (key == "rowsPerBeat")
						rowsPerBeat = static_cast<uint32_t>(std::stoul(val));
				}
				catch (const std::exception&)
				{
				}
			}

			continue;
		}

		std::istringstream lineStream(line.substr(start));

		std::string trackName;
		float row = 0.0f;
		std::string valueStr;
		std::string interpStr = "linear";

		if (!(lineStream >> trackName >> row >> valueStr))
			continue; 

		lineStream >> interpStr; 

		GetOrCreateTrack(trackName).AddKeyframe(
			row,
			ParseValue(valueStr),
			InterpFromString(interpStr)
		);
	}

	currentPath = _path;

	if (std::filesystem::exists(_path))
		lastWriteTime = std::filesystem::last_write_time(_path);

	std::cout << "SyncTimeline: loaded '" << _path << "' (" << tracks.size() << " tracks)\n";

	return true;
}

bool SyncTimeline::SaveToFile(const std::string& _path) const
{
	std::ofstream file(_path, std::ios::trunc);

	if (!file.is_open())
	{
		std::cerr << "SyncTimeline: failed to write '" << _path << "'\n";
		return false;
	}

	file << "# bpm=" << bpm
		<< " rowsPerBeat=" << rowsPerBeat
		<< "\n\n";

	std::vector<const Track*> sortedTracks;
	sortedTracks.reserve(tracks.size());

	for (const auto& [name, track] : tracks)
		sortedTracks.push_back(&track);

	std::sort(
		sortedTracks.begin(),
		sortedTracks.end(),
		[](const Track* a, const Track* b)
		{
			return a->name < b->name;
		});

	for (const Track* track : sortedTracks)
	{
		for (const Keyframe& kf : track->keyframes)
		{
			file << track->name << "\t"
				<< kf.row << "\t"
				<< FormatValue(kf.value) << "\t"
				<< InterpToString(kf.interp)
				<< "\n";
		}
	}

	file.close();

	return true;
}

void SyncTimeline::CreateNewFile(
	const std::string& _path,
	const std::vector<std::string>& _meshTrackNames,
	const std::vector<glm::vec3>& _meshPositions,
	const glm::vec3& _cameraPos,
	const glm::vec3& _cameraTarget,
	const glm::vec3& _sunDir)
{
	tracks.clear();

	const size_t count = std::min(_meshTrackNames.size(), _meshPositions.size());

	for (size_t i = 0; i < count; ++i)
	{
		GetOrCreateTrack("mesh." + _meshTrackNames[i] + ".pos")
			.AddKeyframe(0.0f, glm::vec4(_meshPositions[i], 0.0f), Keyframe::Interp::Linear);
	}

	GetOrCreateTrack("camera.pos")
		.AddKeyframe(0.0f, glm::vec4(_cameraPos, 0.0f), Keyframe::Interp::Linear);

	GetOrCreateTrack("camera.target")
		.AddKeyframe(0.0f, glm::vec4(_cameraTarget, 0.0f), Keyframe::Interp::Linear);

	GetOrCreateTrack("sun.dir")
		.AddKeyframe(0.0f, glm::vec4(_sunDir, 0.0f), Keyframe::Interp::Linear);

	SaveToFile(_path);

	currentPath = _path;

	if (std::filesystem::exists(_path))
		lastWriteTime = std::filesystem::last_write_time(_path);

	std::cout << "SyncTimeline: created new timeline '" << _path << "' (" << tracks.size() << " tracks)\n";
}

bool SyncTimeline::ReloadIfChanged()
{
	if (currentPath.empty())
		return false;

	if (!std::filesystem::exists(currentPath))
		return false;

	const auto writeTime = std::filesystem::last_write_time(currentPath);

	if (writeTime == lastWriteTime)
		return false;

	const std::string path = currentPath;
	return LoadFromFile(path);
}

glm::vec4 SyncTimeline::Evaluate(const std::string& _trackName, float _row) const
{
	auto it = tracks.find(_trackName);

	if (it == tracks.end())
		return glm::vec4(0.0f);

	return it->second.Evaluate(_row);
}

void SyncTimeline::AddKeyframeAndSave(
	const std::string& _trackName,
	float _row,
	const glm::vec4& _value,
	Keyframe::Interp _interp)
{
	GetOrCreateTrack(_trackName).AddKeyframe(_row, _value, _interp);

	if (currentPath.empty())
		return;

	SaveToFile(currentPath);

	if (std::filesystem::exists(currentPath))
		lastWriteTime = std::filesystem::last_write_time(currentPath);
}