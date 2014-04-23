//#pragma once
#ifndef SAFPLUSAMF_H
#define SAFPLUSAMF_H

#include <clMgtModule.hxx>
#include <clMgtApi.hxx>

#include <Application.hxx>
#include <Component.hxx>
#include <ServiceUnit.hxx>
#include <ServiceInstance.hxx>
#include <ComponentServiceInstance.hxx>
#include <Node.hxx>
#include <ServiceGroup.hxx>

#include <EntityByName.hxx>
#include <EntityById.hxx>


namespace SAFplusAmf
{
  // Top level class the represents the log.yang file.  I think that this should be auto-generated.
  class SAFplusAmfRoot : public ClMgtObject
  {
  public:
    ClMgtList clusterList;
    ClMgtList nodeList;
    ClMgtList serviceGroupList;
    ClMgtList componentList;
    ClMgtList componentServiceInstanceList;
    ClMgtList serviceInstanceList;
    ClMgtList serviceUnitList;
    ClMgtList applicationList;

    ClMgtList entityByNameList;
    ClMgtList entityByIdList;

    // sometype healthCheckPeriod;
    // sometype healthCheckMaxSilence;

    /*? Load module configuration from database & create tracking objects */
    void load(ClMgtDatabase* db);
    SAFplusAmfRoot();
  };
};

#endif
