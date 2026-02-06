//by ydog01. -- 2026/2/6
#pragma once
#include "Osum.hpp"
#include <vector>
#include <map>
#include <set>
#include <algorithm>

class Demaker
{
public:
    void process(Osum& osum);

private:
    void processTimePoint(Osum& osum, int time, 
                         std::vector<int>& notesToRemove,
                         std::vector<HitObject>& notesToAdd) const;
    
    std::vector<int> getOccupiedTimes(const Osum& osum) const;
    
    std::set<int> getOccupiedColumnsAtTime(const Osum& osum, int time) const;
    
    std::vector<int> getReverseColumns(int keyCount, const std::set<int>& occupiedColumns) const;
};

void Demaker::process(Osum& osum)
{
    std::vector<int> occupiedTimes = getOccupiedTimes(osum);
    
    std::vector<int> notesToRemove;
    std::vector<HitObject> notesToAdd;
    
    for (int time : occupiedTimes)
        processTimePoint(osum, time, notesToRemove, notesToAdd);
    
    std::sort(notesToRemove.begin(), notesToRemove.end(), std::greater<int>());
    auto& hitObjects = osum.getHitObjects();
    
    for (int index : notesToRemove)
        if (index >= 0 && index < static_cast<int>(hitObjects.size()))
            hitObjects.erase(hitObjects.begin() + index);
    
    for (const auto& obj : notesToAdd)
        hitObjects.push_back(obj);
    
    std::sort(hitObjects.begin(), hitObjects.end(),
              [](const HitObject& a, const HitObject& b)
              {
                  return a.time < b.time;
              });
}

std::vector<int> Demaker::getOccupiedTimes(const Osum& osum) const
{
    std::vector<int> times;
    const auto& hitObjects = osum.getHitObjects();
    
    for (const auto& obj : hitObjects)
        times.push_back(obj.time);
    
    std::sort(times.begin(), times.end());
    times.erase(std::unique(times.begin(), times.end()), times.end());
    
    return times;
}

std::set<int> Demaker::getOccupiedColumnsAtTime(const Osum& osum, int time) const
{
    std::set<int> occupiedColumns;
    const auto& hitObjects = osum.getHitObjects();
    int keyCount = osum.getKeyCount();
    
    for (const auto& obj : hitObjects)
    {
        if (obj.time == time)
        {
            Note note = obj.toNote(keyCount);
            occupiedColumns.insert(note.column);
        }
    }
    
    return occupiedColumns;
}

std::vector<int> Demaker::getReverseColumns(int keyCount, const std::set<int>& occupiedColumns) const
{
    std::vector<int> reverseColumns;
    
    for (int col = 0; col < keyCount; ++col)
        if (occupiedColumns.find(col) == occupiedColumns.end())
            reverseColumns.push_back(col);
    
    return reverseColumns;
}

void Demaker::processTimePoint(Osum& osum, int time,
                              std::vector<int>& notesToRemove,
                              std::vector<HitObject>& notesToAdd) const
{
    const auto& hitObjects = osum.getHitObjects();
    int keyCount = osum.getKeyCount();
    
    std::set<int> occupiedColumns = getOccupiedColumnsAtTime(osum, time);
    
    if (occupiedColumns.empty())
        return;
    
    std::vector<int> reverseColumns = getReverseColumns(keyCount, occupiedColumns);
    
    for (int col : reverseColumns)
    {
        Note reverseNote;
        reverseNote.time = time;
        reverseNote.column = col;
        reverseNote.isLongNote = false;
        reverseNote.endTime = time;
        
        notesToAdd.push_back(HitObject::fromNote(reverseNote, keyCount));
    }
    
    for (size_t i = 0; i < hitObjects.size(); ++i)
        if (hitObjects[i].time == time)
            notesToRemove.push_back(static_cast<int>(i));
}