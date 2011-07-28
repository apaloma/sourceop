/*
    This file is part of SourceOP.

    SourceOP is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    SourceOP is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with SourceOP.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef VOTESYSTEM_H
#define VOTESYSTEM_H

struct VoteData_t
{
    char    item[256];
    int     uid;
};

struct VoteCount_t
{
    char    item[256];
    int     votes;
};

class CVoteSystem
{
public:
    void StartVote();
    bool VoteInProgress() { return voteStatus; }
    void EndVote();
    void AddChoice(const char *szChoice, int uid);
    void ChangeVote(const char *szChoice, int uid);
    unsigned short IsChoice(const char *szChoice);
    unsigned short HasVoted(int uid);
    void RemoveChoice(const char *szChoice);
    void RemoveChoice(int uid);
    unsigned short InvalidIndex();
    VoteCount_t GetTopChoice();
    int GetCount(const char *szChoice);

    float endTime;
    float lastVote;
    bool hasVoted;
private:
    bool voteStatus;

    CUtlLinkedList <VoteData_t, unsigned short> VoteData;
};


#endif // VOTESYSTEM_H
