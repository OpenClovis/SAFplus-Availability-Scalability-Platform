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

#include <AlarmProfileData.hxx>
#include <clLogIpi.hxx>

namespace SAFplus
{
//constructor
AlarmProfileData::AlarmProfileData()
{
  memset(resourceId, 0, SAFplusI::MAX_RESOURCE_NAME_SIZE);
  category = AlarmCategory::INVALID;
  probCause = AlarmProbableCause::INVALID;
  isSend = false;
  specificProblem = 0;
  intAssertSoakingTime = 0;
  intClearSoakingTime = 0;
  generationRuleBitmap = 0;
  genRuleRelation = AlarmRuleRelation::INVALID;
  suppressionRuleBitmap = 0;
  suppRuleRelation = AlarmRuleRelation::INVALID;
  intIndex = 0;
  affectedBitmap = 0;
  isSuppressChild = true;
}
AlarmProfileData::AlarmProfileData(const std::string& inresourceId, const AlarmCategory& incategory, const AlarmProbableCause& inprobCause, const AlarmSpecificProblem& aspecificProblem, const bool& aisSend, const int& aintAssertSoakingTime, const int& aintClearSoakingTime,
    const AlarmRuleRelation& agenRuleRelation, const BITMAP64& agenerationRuleBitmap, const AlarmRuleRelation& asuppRuleRelation, const BITMAP64& asuppressionRuleBitmap, const int& aintIndex, const BITMAP64& aaffectedBitmap,const bool& aisSuppressChild)
{
  memset(resourceId, 0, SAFplusI::MAX_RESOURCE_NAME_SIZE);
  strcpy(resourceId, inresourceId.c_str());
  category = incategory;
  probCause = inprobCause;
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
  isSuppressChild = aisSuppressChild;
}
AlarmProfileData AlarmProfileData::operator=(const AlarmProfileData& other)
{
  memset(resourceId, 0, SAFplusI::MAX_RESOURCE_NAME_SIZE);
  strcpy(resourceId, other.resourceId);
  category = other.category;
  probCause = other.probCause;
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
  isSuppressChild = other.isSuppressChild;
}
bool AlarmProfileData::operator==(const AlarmProfileData& other)
{
  return ((strcmp(resourceId, other.resourceId) == 0) && (category == other.category) && (probCause == other.probCause) && (isSend == other.isSend) && (specificProblem == other.specificProblem) && (intAssertSoakingTime == other.intAssertSoakingTime)
      && (intClearSoakingTime == other.intClearSoakingTime) && (generationRuleBitmap == other.generationRuleBitmap) && (genRuleRelation == other.genRuleRelation) && (suppressionRuleBitmap == other.suppressionRuleBitmap)
      && (suppRuleRelation == other.suppRuleRelation) && (intIndex == other.intIndex) && (affectedBitmap == other.affectedBitmap) && (isSuppressChild == other.isSuppressChild));

}
std::string AlarmProfileData::toString() const
{
  std::ostringstream oss;
  oss << "resourceId:[" << resourceId << "] category:[" << category << "] probCause:[" << probCause << "] specificProblem:[" << specificProblem << "] intAssertSoakingTime:[" << intAssertSoakingTime << "] intClearSoakingTime:[" << intClearSoakingTime/*<<"] intPollTime:["<<intPollTime*/
      << "] genRuleRelation:[" << genRuleRelation << "] generateRuleBitmap:[" << generationRuleBitmap << "] suppRuleRelation:[" << suppRuleRelation << "] suppressionRuleBitmap:[" << suppressionRuleBitmap << "] affectedBitmap:[" << affectedBitmap
      << "] intIndex:[" << intIndex <<"] isSuppressChild:["<<isSuppressChild<<"]";
  return oss.str();
}
}
