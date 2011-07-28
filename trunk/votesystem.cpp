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

#include <stdio.h>
#include <ctype.h>

#include "AdminOP.h"
#include "adminopplayer.h"
#include "votesystem.h"
#include "recipientfilter.h"
#include "bitbuf.h"
#include "cvars.h"
#include "utllinkedlist.h"

#include "tier0/memdbgon.h"

void CVoteSystem :: StartVote()
{
    voteStatus = 1;
    VoteData.Purge();
}

void CVoteSystem :: EndVote()
{
    lastVote = engine->Time();
    voteStatus = 0;
    hasVoted = 1;
    VoteData.Purge();
}

void CVoteSystem :: AddChoice(const char *szChoice, int uid)
{
    VoteData_t newChoice;
    strcpy(newChoice.item, szChoice);
    newChoice.uid = uid;

    VoteData.AddToTail(newChoice);
}

void CVoteSystem :: ChangeVote(const char *szChoice, int uid)
{
    int voteid = HasVoted(uid);
    
    if(voteid != VoteData.InvalidIndex())
        strcpy(VoteData.Element(voteid).item, szChoice);
    else
        AddChoice(szChoice, uid);
}

unsigned short CVoteSystem :: IsChoice(const char *szChoice)
{
    for ( unsigned short i=VoteData.Head(); i != VoteData.InvalidIndex(); i = VoteData.Next( i ) )
    {
        if ( !Q_strcasecmp(VoteData.Element( i ).item, szChoice) )
            return i;
    }
    return VoteData.InvalidIndex();
}

unsigned short CVoteSystem :: HasVoted(int uid)
{
    for ( unsigned short i=VoteData.Head(); i != VoteData.InvalidIndex(); i = VoteData.Next( i ) )
    {
        if ( VoteData.Element( i ).uid == uid )
            return i;
    }
    return VoteData.InvalidIndex();
}

void CVoteSystem :: RemoveChoice(const char *szChoice)
{
    int NextI = VoteData.Head();
    for ( unsigned short i=VoteData.Head(); i != VoteData.InvalidIndex(); i = NextI )
    {
        NextI = VoteData.Next(i);
        if ( !Q_strcasecmp(VoteData.Element( i ).item, szChoice) )
            VoteData.Remove(i);
    }
}

void CVoteSystem :: RemoveChoice(int uid)
{
    unsigned short i = HasVoted(uid);
    if( i != VoteData.InvalidIndex())
    {
        VoteData.Remove(i);
    }
}

unsigned short CVoteSystem :: InvalidIndex()
{
    return VoteData.InvalidIndex();
}

VoteCount_t CVoteSystem :: GetTopChoice()
{
    int maxvotecount = 0;
    int maxvotechoice = VoteData.InvalidIndex();
    CUtlLinkedList <VoteCount_t, unsigned short> VoteCount;
    VoteCount.Purge();

    for ( unsigned short i=VoteData.Head(); i != VoteData.InvalidIndex(); i = VoteData.Next( i ) )
    {
        bool added = 0;
        //Msg("VoteLook: %s\n", VoteData.Element(i).item);
        for ( unsigned short j=VoteCount.Head(); j != VoteCount.InvalidIndex(); j = VoteCount.Next( j ) )
        {
            if ( !Q_strcasecmp(VoteCount.Element( j ).item, VoteData.Element( i ).item) )
            {
                added = 1;
                //Msg("Voteadd: %s\n", VoteData.Element(i).item);
                VoteCount.Element( j ).votes++;
                break;
            }
        }
        if(added == 0)
        {
            //Msg("Votenew: %s\n", VoteData.Element(i).item);
            VoteCount_t NewVoteCount;
            strcpy(NewVoteCount.item, VoteData.Element(i).item);
            NewVoteCount.votes = 1;
            VoteCount.AddToTail(NewVoteCount);
        }
    }
    for ( unsigned short k=VoteCount.Head(); k != VoteCount.InvalidIndex(); k = VoteCount.Next( k ) )
    {
        if(VoteCount.Element(k).votes > maxvotecount)
        {
            //Msg("HighVote: %s %i\n", VoteCount.Element(k).item, VoteCount.Element(k).votes);
            maxvotecount = VoteCount.Element(k).votes;
            maxvotechoice = k;
        }
    }
    if(maxvotechoice != VoteData.InvalidIndex())
    {
        //Msg("ReturnHighVote: %s %i\n", VoteCount.Element(maxvotechoice).item, VoteCount.Element(maxvotechoice).votes);
        return VoteCount.Element(maxvotechoice);
    }

    VoteCount_t nullvote;
    nullvote.item[0] = '\0';
    nullvote.votes = 0;
    return nullvote;
}

int CVoteSystem :: GetCount(const char *szChoice)
{
    int count = 0;
    for ( unsigned short i=VoteData.Head(); i != VoteData.InvalidIndex(); i = VoteData.Next( i ) )
    {
        if ( !Q_strcasecmp(VoteData.Element( i ).item, szChoice) )
            count++;
    }
    return count;
}
