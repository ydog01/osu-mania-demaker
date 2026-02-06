//by ydog01. -- 2026/2/6
#pragma once
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <iostream>

struct TimingPoint;
struct HitObject;
struct Note;

class Osum
{
public:
    void read(const std::string& path);
    void write(const std::string& path);

    int getKeyCount() const
    {
        return m_keyCount;
    }

    std::vector<HitObject>& getHitObjects()
    {
        return m_hitObjects;
    }

    const std::vector<HitObject>& getHitObjects() const
    {
        return m_hitObjects;
    }

    std::vector<TimingPoint>& getTimingPoints()
    {
        return m_timingPoints;
    }

    const std::vector<TimingPoint>& getTimingPoints() const
    {
        return m_timingPoints;
    }

    const std::string& getSection(const std::string& sectionName) const
    {
        auto it(m_sections.find(sectionName));
        if (it != m_sections.end())
            return it->second;
        static const std::string empty;
        return empty;
    }

    std::string& getSection(const std::string& sectionName)
    {
        auto it(m_sections.find(sectionName));
        if (it != m_sections.end())
            return it->second;
        static std::string empty;
        return empty;
    }

private:
    void parseHitObjects(const std::string& content);
    void parseTimingPoints(const std::string& content);
    std::string generateHitObjectsContent() const;
    std::string generateTimingPointsContent() const;
    void extractKeyCount(const std::string& difficultySection);
    void checkMode(const std::string& generalSection);

    std::map<std::string, std::string> m_sections;
    std::vector<HitObject> m_hitObjects;
    std::vector<TimingPoint> m_timingPoints;
    int m_keyCount;
};

struct TimingPoint
{
    double time;
    double beatLength;
    int meter;
    int sampleSet;
    int sampleIndex;
    int volume;
    bool uninherited;
    int effects;

    static TimingPoint fromString(const std::string& line);
    std::string toString() const;
};

struct HitObject
{
    int x;
    int y;
    int time;
    int type;//1=点，128=长条
    int hitSound;
    std::string extras;

    Note toNote(int keyCount) const;// 轨道编号 = x / (512 / 总键数)
    static HitObject fromNote(const Note& note, int keyCount);
    static HitObject fromString(const std::string& line);
    std::string toString() const;
};

struct Note
{
    int time;
    int column;
    bool isLongNote;
    int endTime;
};

void Osum::read(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open())
        throw std::runtime_error("Failed to open file: " + path);

    m_sections.clear();
    m_hitObjects.clear();
    m_timingPoints.clear();

    std::string line;
    std::string currentSection;
    std::stringstream sectionContent;

    while (std::getline(file, line))
    {
        if (line.length() > 2 && line[0] == '[' && line.back() == ']')
        {
            if (!currentSection.empty())
            {
                m_sections[currentSection] = sectionContent.str();
                sectionContent.str("");
                sectionContent.clear();
            }
            currentSection = line.substr(1, line.length() - 2);
            continue;
        }

        if (!currentSection.empty())
            sectionContent << line << "\n";
    }

    if (!currentSection.empty())
        m_sections[currentSection] = sectionContent.str();

    checkMode(getSection("General"));
    extractKeyCount(getSection("Difficulty"));
    parseHitObjects(getSection("HitObjects"));
    parseTimingPoints(getSection("TimingPoints"));
}

void Osum::write(const std::string& path)
{
    std::ofstream file(path);
    if (!file.is_open())
        throw std::runtime_error("Failed to write file: " + path);

    const std::vector<std::string> sectionOrder
    {
        "osu file format v14",
        "General",
        "Editor",
        "Metadata",
        "Difficulty",
        "Events",
        "TimingPoints",
        "HitObjects"
    };

    for (const auto& sectionName : sectionOrder)
    {
        if (sectionName == "osu file format v14")
        {
            file << "osu file format v14\n\n";
            continue;
        }

        if (sectionName == "HitObjects")
        {
            file << "[HitObjects]\n";
            file << generateHitObjectsContent();
            if (!m_hitObjects.empty())
                file << "\n";
            continue;
        }

        if (sectionName == "TimingPoints")
        {
            file << "[TimingPoints]\n";
            file << generateTimingPointsContent();
            if (!m_timingPoints.empty())
                file << "\n";
            continue;
        }

        auto it(m_sections.find(sectionName));
        if (it != m_sections.end() && !it->second.empty())
        {
            file << "[" << sectionName << "]\n";
            file << it->second;
            if (it->second.back() != '\n')
                file << "\n";
        }
    }
}

void Osum::parseHitObjects(const std::string& content)
{
    m_hitObjects.clear();
    std::istringstream ss(content);
    std::string line;

    while (std::getline(ss, line))
    {
        if (line.empty())
            continue;
        try
        {
            m_hitObjects.push_back(HitObject::fromString(line));
        }
        catch (const std::exception& e)
        {
            std::cerr << "Warning: Failed to parse HitObject: " << line << " - " << e.what() << "\n";
        }
    }
}

void Osum::parseTimingPoints(const std::string& content)
{
    m_timingPoints.clear();
    std::istringstream ss(content);
    std::string line;

    while (std::getline(ss, line))
    {
        if (line.empty())
            continue;
        try
        {
            m_timingPoints.push_back(TimingPoint::fromString(line));
        }
        catch (const std::exception& e)
        {
            std::cerr << "Warning: Failed to parse TimingPoint: " << line << " - " << e.what() << "\n";
        }
    }
}

std::string Osum::generateHitObjectsContent() const
{
    std::stringstream ss;
    for (size_t i(0); i < m_hitObjects.size(); ++i)
    {
        ss << m_hitObjects[i].toString();
        if (i != m_hitObjects.size() - 1)
            ss << "\n";
    }
    return ss.str();
}

std::string Osum::generateTimingPointsContent() const
{
    std::stringstream ss;
    for (size_t i(0); i < m_timingPoints.size(); ++i)
    {
        ss << m_timingPoints[i].toString();
        if (i != m_timingPoints.size() - 1)
            ss << "\n";
    }
    return ss.str();
}

void Osum::extractKeyCount(const std::string& difficultySection)
{
    std::istringstream ss(difficultySection);
    std::string line;

    while (std::getline(ss, line))
    {
        if (line.find("CircleSize:") == 0)
        {
            try
            {
                m_keyCount = std::stoi(line.substr(11));
                return;
            }
            catch (...)
            {
                m_keyCount = 4;
                return;
            }
        }
    }

    m_keyCount = 4;
}

void Osum::checkMode(const std::string& generalSection)
{
    std::istringstream ss(generalSection);
    std::string line;

    while (std::getline(ss, line))
    {
        if (line.find("Mode:") == 0)
        {
            int mode = std::stoi(line.substr(5));
            if (mode != 3)
                throw std::runtime_error("Not a mania map (Mode: " + std::to_string(mode) + ")");
            return;
        }
    }

    throw std::runtime_error("Mode field not found");
}

TimingPoint TimingPoint::fromString(const std::string& line)
{
    TimingPoint tp;
    std::istringstream ss(line);
    std::string token;

    std::getline(ss, token, ',');
    tp.time = std::stod(token);

    std::getline(ss, token, ',');
    tp.beatLength = std::stod(token);

    std::getline(ss, token, ',');
    tp.meter = std::stoi(token);

    std::getline(ss, token, ',');
    tp.sampleSet = std::stoi(token);

    std::getline(ss, token, ',');
    tp.sampleIndex = std::stoi(token);

    std::getline(ss, token, ',');
    tp.volume = std::stoi(token);

    std::getline(ss, token, ',');
    tp.uninherited = (std::stoi(token) == 1);

    std::getline(ss, token, ',');
    tp.effects = std::stoi(token);

    return tp;
}

std::string TimingPoint::toString() const
{
    std::stringstream ss;
    ss << static_cast<int>(time) << ","
       << beatLength << ","
       << meter << ","
       << sampleSet << ","
       << sampleIndex << ","
       << volume << ","
       << (uninherited ? 1 : 0) << ","
       << effects;
    return ss.str();
}

HitObject HitObject::fromString(const std::string& line)
{
    HitObject obj;
    std::istringstream ss(line);
    std::string token;

    std::getline(ss, token, ',');
    obj.x = std::stoi(token);

    std::getline(ss, token, ',');
    obj.y = std::stoi(token);

    std::getline(ss, token, ',');
    obj.time = std::stoi(token);

    std::getline(ss, token, ',');
    obj.type = std::stoi(token);

    std::getline(ss, token, ',');
    obj.hitSound = std::stoi(token);

    std::getline(ss, obj.extras);

    return obj;
}

std::string HitObject::toString() const
{
    std::stringstream ss;
    ss << x << "," << y << "," << time << "," << type << "," << hitSound;
    if (!extras.empty())
        ss << "," << extras;
    return ss.str();
}

Note HitObject::toNote(int keyCount) const
{
    Note note;
    note.time = time;
    note.isLongNote = (type & 128) != 0;

    int columnWidth = 512 / keyCount;
    int column = x / columnWidth;
    if (column < 0)
        column = 0;
    if (column >= keyCount)
        column = keyCount - 1;
    note.column = column;

    if (note.isLongNote && !extras.empty())
    {
        size_t colonPos(extras.find(':'));
        if (colonPos != std::string::npos)
        {
            try
            {
                note.endTime = std::stoi(extras.substr(0, colonPos));
            }
            catch (...)
            {
                note.endTime = time;
            }
        }
        else
            note.endTime = time;
    }
    else
        note.endTime = time;

    return note;
}

HitObject HitObject::fromNote(const Note& note, int keyCount)
{
    HitObject obj;

    int columnWidth = 512 / keyCount;
    obj.x = note.column * columnWidth + columnWidth / 2;
    obj.y = 192;
    obj.time = note.time;
    obj.type = note.isLongNote ? 128 : 1;
    obj.hitSound = 0;

    if (note.isLongNote)
        obj.extras = std::to_string(note.endTime) + ":0:0:0:0:";

    return obj;
}