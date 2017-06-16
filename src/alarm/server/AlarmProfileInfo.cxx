// /*
//  * Copyright (C) 2002-2014 OpenClovis Solutions Inc.  All Rights Reserved.
//  *
//  * This file is available  under  a  commercial  license  from  the
//  * copyright  holder or the GNU General Public License Version 2.0.
//  *
//  * The source code for  this program is not published  or otherwise
//  * divested of  its trade secrets, irrespective  of  what  has been
//  * deposited with the U.S. Copyright office.
//  *
//  * This program is distributed in the  hope that it will be useful,
//  * but WITHOUT ANY WARRANTY; without even the implied  warranty  of
//  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
//  * General Public License for more details.
//  *
//  * For more  information, see  the file  COPYING provided with this
//  * material.
//  */

#include <AlarmProfileInfo.hxx>
#include <clLogIpi.hxx>

namespace SAFplus
{
  //constructor
  AlarmProfileInfo::AlarmProfileInfo()
  {
	isSend = false;
	specificProblem = 0;
	intAssertSoakingTime = 0;
	intClearSoakingTime = 0;
	//intPollTime = 0;
	generationRuleBitmap = 0;
	genRuleRelation = AlarmRuleRelation::INVALID;
	suppressionRuleBitmap = 0;
	suppRuleRelation = AlarmRuleRelation::INVALID;
    intIndex = 0;
	affectedBitmap = 0;
  }
  AlarmProfileInfo::AlarmProfileInfo(const AlarmSpecificProblem& aspecificProblem,const bool& aisSend, const int& aintAssertSoakingTime,const int& aintClearSoakingTime,
  			const AlarmRuleRelation& agenRuleRelation,const BITMAP64& agenerationRuleBitmap,const AlarmRuleRelation& asuppRuleRelation,const BITMAP64 asuppressionRuleBitmap,
  			const int& aintIndex,const BITMAP64 aaffectedBitmap)
  {
	  specificProblem = aspecificProblem;
	  isSend = aisSend;
	  intAssertSoakingTime = aintAssertSoakingTime;
	  intClearSoakingTime = aintClearSoakingTime;
	  genRuleRelation = agenRuleRelation;
	  generationRuleBitmap = agenerationRuleBitmap;
	  suppRuleRelation = asuppRuleRelation;
	  suppressionRuleBitmap = asuppressionRuleBitmap;
	  intIndex = aintIndex;
	  affectedBitmap = aaffectedBitmap;
  }
  AlarmProfileInfo AlarmProfileInfo::operator=(const AlarmProfileInfo& other)
  {
	isSend = other.isSend;
	specificProblem = other.specificProblem;
	intAssertSoakingTime = other.intAssertSoakingTime;
	intClearSoakingTime = other.intClearSoakingTime;
	//intPollTime = other.intPollTime;
	generationRuleBitmap = other.generationRuleBitmap;
	genRuleRelation = other.genRuleRelation;
	suppressionRuleBitmap = other.suppressionRuleBitmap;
	suppRuleRelation = other.suppRuleRelation;
	intIndex = other.intIndex;
	affectedBitmap = other.affectedBitmap;
  }
  bool AlarmProfileInfo::operator==(const AlarmProfileInfo& other)
  {
	  return(
			(isSend == other.isSend)&&
			(specificProblem == other.specificProblem)&&
			(intAssertSoakingTime == other.intAssertSoakingTime)&&
			(intClearSoakingTime == other.intClearSoakingTime)&&
			//(intPollTime == other.intPollTime)&&
			(generationRuleBitmap == other.generationRuleBitmap)&&
			(genRuleRelation == other.genRuleRelation)&&
			(suppressionRuleBitmap == other.suppressionRuleBitmap)&&
			(suppRuleRelation == other.suppRuleRelation)&&
			(intIndex == other.intIndex)&&
			(affectedBitmap == other.affectedBitmap)
	  );

  }
  std::string AlarmProfileInfo::toString() const
  {
	std::ostringstream oss;
	oss <<" specificProblem:["<<specificProblem<<"] intAssertSoakingTime:["<<intAssertSoakingTime<<"] intClearSoakingTime:["
		<<intClearSoakingTime/*<<"] intPollTime:["<<intPollTime*/<<"] genRuleRelation:["<<genRuleRelation<<"] generateRuleBitmap:["
		<<generationRuleBitmap<<"] suppRuleRelation:["<<suppRuleRelation<<"] suppressionRuleBitmap:["<<suppressionRuleBitmap
		<<"] affectedBitmap:["<<affectedBitmap<<"] intIndex:["<<intIndex<<"]";
	return oss.str();
  }
}
