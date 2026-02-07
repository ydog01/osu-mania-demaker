// demaker.hpp
#pragma once
#include "Osum.hpp"
#include <vector>
#include <map>
#include <set>
#include <algorithm>

class Demaker
{
public:
    void setAlignParams(bool enableAlign, int rangeMs);
    void process(Osum& osum);

private:
    void convertLongNotesToSingle(Osum& osum) const;
    void alignNotes(Osum& osum) const;
    void reverseComplete(Osum& osum) const;
    std::set<int> getOccupiedColumnsAtTime(const Osum& osum, int time) const;
    std::vector<int> getOccupiedTimes(const Osum& osum) const;
    void removeDuplicates(Osum& osum) const;
    std::map<int, std::vector<Note>> groupNotesByTime(const Osum& osum) const;
    void alignNoteGroup(std::vector<Note>& notes) const;

    bool m_enableAlign = false;
    int m_rangeMs = 0;
};

void Demaker::setAlignParams(bool enableAlign, int rangeMs)
{
    m_enableAlign = enableAlign;
    m_rangeMs = rangeMs;
}

void Demaker::process(Osum& osum)
{
    convertLongNotesToSingle(osum);
    
    if (m_enableAlign && m_rangeMs > 0)
        alignNotes(osum);
    
    reverseComplete(osum);
    removeDuplicates(osum);
}

void Demaker::convertLongNotesToSingle(Osum& osum) const
{
    auto& hitObjects = osum.getHitObjects();
    std::vector<HitObject> newHitObjects;
    int keyCount = osum.getKeyCount();
    
    for (const auto& obj : hitObjects)
    {
        if (obj.type & 128)
        {
            Note note = obj.toNote(keyCount);
            note.isLongNote = false;
            note.endTime = note.time;
            
            HitObject singlePoint = HitObject::fromNote(note, keyCount);
            newHitObjects.push_back(singlePoint);
        }
        else
            newHitObjects.push_back(obj);
    }
    
    hitObjects = newHitObjects;
}

void Demaker::alignNotes(Osum& osum) const
{
    auto timeGroups = groupNotesByTime(osum);
    std::vector<Note> allNotes;
    int keyCount = osum.getKeyCount();
    
    for (const auto& obj : osum.getHitObjects())
        allNotes.push_back(obj.toNote(keyCount));
    
    std::sort(allNotes.begin(), allNotes.end(),
        [](const Note& a, const Note& b) {
            return a.time < b.time;
        });
    
    std::vector<Note> alignedNotes;
    std::vector<Note> currentGroup;
    
    for (size_t i(0); i < allNotes.size(); ++i)
    {
        currentGroup.push_back(allNotes[i]);
        
        if (i + 1 < allNotes.size() && 
            allNotes[i + 1].time - allNotes[i].time <= m_rangeMs)
            continue;
        
        if (currentGroup.size() > 1)
            alignNoteGroup(currentGroup);
        
        for (const auto& note : currentGroup)
            alignedNotes.push_back(note);
        
        currentGroup.clear();
    }
    
    if (!currentGroup.empty())
    {
        if (currentGroup.size() > 1)
            alignNoteGroup(currentGroup);
        
        for (const auto& note : currentGroup)
            alignedNotes.push_back(note);
    }
    
    std::vector<HitObject> newHitObjects;
    for (const auto& note : alignedNotes)
        newHitObjects.push_back(HitObject::fromNote(note, keyCount));
    
    osum.getHitObjects() = newHitObjects;
}

void Demaker::alignNoteGroup(std::vector<Note>& notes) const
{
    if (notes.empty()) return;
    
    int earliestTime = notes[0].time;
    for (const auto& note : notes)
        if (note.time < earliestTime)
            earliestTime = note.time;
    
    for (auto& note : notes)
    {
        note.time = earliestTime;
        note.endTime = earliestTime;
    }
}

std::map<int, std::vector<Note>> Demaker::groupNotesByTime(const Osum& osum) const
{
    std::map<int, std::vector<Note>> groups;
    int keyCount = osum.getKeyCount();
    
    for (const auto& obj : osum.getHitObjects())
    {
        Note note = obj.toNote(keyCount);
        groups[note.time].push_back(note);
    }
    
    return groups;
}

void Demaker::reverseComplete(Osum& osum) const
{
    auto& hitObjects = osum.getHitObjects();
    int keyCount = osum.getKeyCount();
    std::vector<int> occupiedTimes = getOccupiedTimes(osum);
    std::vector<int> indicesToRemove;
    std::vector<HitObject> objectsToAdd;
    
    for (int time : occupiedTimes)
    {
        std::set<int> occupiedCols = getOccupiedColumnsAtTime(osum, time);
        std::vector<int> freeCols;
        
        for (int col(0); col < keyCount; ++col)
            if (occupiedCols.find(col) == occupiedCols.end())
                freeCols.push_back(col);
        
        for (size_t i(0); i < hitObjects.size(); ++i)
            if (hitObjects[i].time == time)
                indicesToRemove.push_back(static_cast<int>(i));
        
        for (int col : freeCols)
        {
            Note newNote;
            newNote.time = time;
            newNote.column = col;
            newNote.isLongNote = false;
            newNote.endTime = time;
            
            objectsToAdd.push_back(HitObject::fromNote(newNote, keyCount));
        }
    }
    
    std::sort(indicesToRemove.begin(), indicesToRemove.end(), std::greater<int>());
    for (int idx : indicesToRemove)
        if (idx >= 0 && idx < static_cast<int>(hitObjects.size()))
            hitObjects.erase(hitObjects.begin() + idx);
    
    for (const auto& obj : objectsToAdd)
        hitObjects.push_back(obj);
    
    std::sort(hitObjects.begin(), hitObjects.end(),
        [](const HitObject& a, const HitObject& b) {
            return a.time < b.time;
        });
}

std::set<int> Demaker::getOccupiedColumnsAtTime(const Osum& osum, int time) const
{
    std::set<int> occupiedCols;
    int keyCount = osum.getKeyCount();
    
    for (const auto& obj : osum.getHitObjects())
    {
        if (obj.time == time)
        {
            Note note = obj.toNote(keyCount);
            occupiedCols.insert(note.column);
        }
    }
    
    return occupiedCols;
}

std::vector<int> Demaker::getOccupiedTimes(const Osum& osum) const
{
    std::set<int> times;
    
    for (const auto& obj : osum.getHitObjects())
        times.insert(obj.time);
    
    return std::vector<int>(times.begin(), times.end());
}

void Demaker::removeDuplicates(Osum& osum) const
{
    auto& hitObjects = osum.getHitObjects();
    int keyCount = osum.getKeyCount();
    std::map<std::pair<int, int>, HitObject> uniqueMap;
    
    for (const auto& obj : hitObjects)
    {
        Note note = obj.toNote(keyCount);
        auto key = std::make_pair(note.time, note.column);
        uniqueMap[key] = obj;
    }
    
    std::vector<HitObject> uniqueObjects;
    for (const auto& pair : uniqueMap)
        uniqueObjects.push_back(pair.second);
    
    std::sort(uniqueObjects.begin(), uniqueObjects.end(),
        [](const HitObject& a, const HitObject& b) {
            return a.time < b.time;
        });
    
    hitObjects = uniqueObjects;
}