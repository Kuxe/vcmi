/*
* DeepDecomposer.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

#include "../Goals/AbstractGoal.h"

namespace NKAI
{

struct GoalHash
{
	uint64_t operator()(const Goals::TSubgoal & goal) const
	{
		return goal->getHash();
	}
};

using TGoalHashSet = std::unordered_map<Goals::TSubgoal, Goals::TGoalVec, GoalHash>;

class DeepDecomposer
{
private:
	std::vector<Goals::TGoalVec> goals;
	std::vector<TGoalHashSet> decompositionCache;
	int depth;
	const Nullkiller * ai;

public:
	DeepDecomposer(const Nullkiller * ai);
	void reset();
	void decompose(Goals::TGoalVec & result, Goals::TSubgoal behavior, int depthLimit);

private:
	Goals::TSubgoal aggregateGoals(int startDepth, Goals::TSubgoal last);
	Goals::TSubgoal unwrapComposition(Goals::TSubgoal goal);
	bool isCompositionLoop(Goals::TSubgoal goal);
	Goals::TGoalVec decomposeCached(Goals::TSubgoal goal, bool & fromCache);
	void addToCache(Goals::TSubgoal goal);
};

}
