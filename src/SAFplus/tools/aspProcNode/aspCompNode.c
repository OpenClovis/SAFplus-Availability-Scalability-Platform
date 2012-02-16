#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <ezxml.h>

static const char **aspMapSU(const char *aspSU)
{
    static struct aspSUMap
    {
        const char *su;
        const char *comp[3];
    } aspSUMap[] = { 
            {"gmsSU", {"safplus_gms", NULL}},
            {"eventSU", {"safplus_event", NULL}},
            {"ckptSU", {"safplus_ckpt", NULL}},
            {"logSU", {"safplus_logd", NULL}},
            {"msgSU", {"safplus_msg", NULL}},
            {"nameSU", {"safplus_name", NULL}},
            {"oampSU", {"safplus_alarm", "safplus_fault", NULL}},
            {"corSU", {"safplus_cor", NULL}},
            {"txnSU", {"safplus_txn", NULL}},
    };
    register int i;
    for(i = 0; i < sizeof(aspSUMap)/sizeof(aspSUMap[0]); ++i)
    {
        if(!strncmp(aspSU, aspSUMap[i].su, strlen(aspSU)))
        {
            return aspSUMap[i].comp;
        }
    }
    return NULL;
}

static int checkComp(const char *comp, char **comps, int numComps)
{
    register int i;
    for(i = 0 ; i < numComps; ++i)
    {
        if(!strcmp(comp, comps[i]))
            return -1;
    }
    return 0;
}

static int parseConfigFile(const char *pConfigFile,
                           const char *pNodeName,
                           const char *pNodeType,
                           char ***comps,
                           int *numComps)
{
    int err = -1;
    ezxml_t top, xml, xmlver;
    char **pComps = NULL;
    int c = 0;
    if(!pConfigFile || !pNodeName || !pNodeType || !comps || !numComps)
        return -1;
    top = xml = ezxml_parse_file(pConfigFile);
    if(!xml)
    {
        fprintf(stderr, "Error parsing xml file\n");
        return -1;
    }
    /*
     * Parse the cpmconfigs for the nodename first. to get asp component list.
     */
    xml = ezxml_child(xml, "version");
    if(!xml)
    {
        fprintf(stderr, "Unable to find version tag. Assuming old config file format\n");
        xmlver = top;
    }
    else
    {
        xmlver = xml->child;
    }
    xml = ezxml_child(xmlver, "cpmConfigs");
    if(!xml)
    {
        fprintf(stderr, "Unable to find cpmConfigs tag\n");
        goto out_free;
    }
    xml = ezxml_child(xml, "cpmConfig");
    if(!xml)
    {
        fprintf(stderr, "Unable to find cpmConfig tag\n");
        goto out_free;
    }

    while(xml)
    {
        char *nodetype = (char*)ezxml_attr(xml, "nodeType");
        if(!nodetype)
        {
            fprintf(stderr, "Unable to find nodeType attribute for cpmConfig tag\n");
            goto out_free;
        }
        if(!strncmp(nodetype, pNodeType, strlen(pNodeType)))
        {
            /*
             * found the hit for the node type. Copy the asp component list now
             */
            xml = ezxml_child(xml, "aspServiceUnits");
            if(!xml)
            {
                fprintf(stderr, "Unable to find asp service units\n");
                goto out_free;
            }
            xml = ezxml_child(xml, "aspServiceUnit");
            while(xml)
            {
                char *aspUnit = (char*)ezxml_attr(xml, "name");
                const char **compMap = NULL;
                if(!aspUnit)
                {
                    fprintf(stderr, "Unable to find asp service unit\n");
                    goto out_free;
                }
                compMap = aspMapSU(aspUnit);
                assert(compMap != NULL);
                while(*compMap)
                {
                    pComps = realloc(pComps, (c+1)*sizeof(*pComps));
                    assert(pComps);
                    pComps[c] = strdup(*compMap);
                    assert(pComps[c]);
                    ++c;
                    ++compMap;
                }
                xml = xml->next;
            }
            goto out_find;
        }
        xml = xml->next;
    }
    
    goto out_free;

    out_find:
    xml = ezxml_child(xmlver, "nodeInstances");
    if(!xml)
    {
        fprintf(stderr, "Unable to find nodeInstances tag\n");
        goto out_free;
    }
    xml = ezxml_child(xml, "nodeInstance");
    while(xml)
    {
        char *nodename = (char*)ezxml_attr(xml, "name");
        if(!nodename)
        {
            fprintf(stderr, "Unable to find nodename tag\n");
            goto out_free;
        }
        if(!strncmp(nodename, pNodeName, strlen(pNodeName)))
        {
            /*
             * node matched. Gather components in this node.
             */
            xml = ezxml_child(xml, "serviceUnitInstances");
            if(!xml)
            {
                fprintf(stderr, "Unable to parse service unit instances\n");
                goto out_free;
            }
            xml = ezxml_child(xml, "serviceUnitInstance");
            if(!xml)
            {
                fprintf(stderr, "Unable to parse service unit tag\n");
                goto out_free;
            }
            while(xml)
            {
                ezxml_t xmlComp;
                ezxml_t xmlComps = ezxml_child(xml, "componentInstances");
                if(!xmlComps)
                {
                    goto next;
                }
                xmlComp = ezxml_child(xmlComps, "componentInstance");
                while(xmlComp)
                {
                    char *compName = (char*)ezxml_attr(xmlComp, "type");
                    if(!compName)
                    {
                        fprintf(stderr, "Unable to find comp name type\n");
                        goto out_free;
                    }
                    pComps = realloc(pComps, (c+1)*sizeof(*pComps));
                    assert(pComps);
                    pComps[c] = strdup(compName);
                    assert(pComps[c]);
                    ++c;
                    xmlComp = xmlComp->next;
                }
                next:
                xml = xml->next;
            }
            goto out_free;
        }
        xml = xml->next;
    }

    out_free:
    ezxml_free(top);
    if(!c)
    {
        if(pComps)
            free(pComps);
    }
    else
    {
        *comps = pComps;
        *numComps = c;
        err = 0;
    }
    return err;
}

int main(int argc, char **argv)
{
    char configFile[0xff+1] = "clAmfConfig.xml";
    char nodename[0xff+1] = "ControllerI0";
    char nodetype[0xff+1] = {0};
    char **comps = NULL;
    char **seenComps = NULL;
    int numSeen = 0;
    int numComps = 0;
    int i;
    if(argc != 2 && argc != 3 && argc != 4)
    {
        fprintf(stderr, "%s [clAmfConfig.xml] [ asp node name] [ asp node type]\n", argv[0]);
        exit(127);
    }
    if(argc > 1)
    {
        strncpy(configFile, argv[1], sizeof(configFile)-1);
    }
    if(argc > 2)
    {
        strncpy(nodename, argv[2], sizeof(nodename)-1);
    }
    if(argc > 3)
    {
        strncpy(nodetype, argv[3], sizeof(nodetype)-1);
    }
    if(!nodetype[0])
    {
        /*  
         *find node type. check for nodenameN or nodenameIN where N is a digit.
         */
        char *s = nodename;
        s += strlen(nodename)-1;
        while(s > nodename && isdigit(*s)) --s;
        if(*s == 'I' && s != nodename) --s;
        strncpy(nodetype, nodename, (s-nodename)+1); 
        fprintf(stderr, "Inferred nodetype [%s] for nodename [%s]\n", nodetype, nodename);
    }
    if(parseConfigFile(configFile, nodename, nodetype, &comps, &numComps) < 0)
    {
        fprintf(stderr, "Error parsing file [%s] for node [%s]\n", configFile, nodename);
        exit(127);
    }
    seenComps = calloc(numComps, sizeof(*seenComps));
    assert(seenComps);
    fprintf(stderr, "Dumping components matching node [%s], nodetype [%s]\n", nodename, nodetype);
    printf("safplus_amf\n");
    for(i = 0; i < numComps; ++i)
    {
        if(!checkComp(comps[i], seenComps, numSeen))
        {
            printf("%s\n", comps[i]);
            seenComps[numSeen++] = comps[i];
        }
    }
    for(i = 0; i < numComps; ++i)
        free(comps[i]);
    if(comps) free(comps);
    if(seenComps) free(seenComps);
    return 0;
}
