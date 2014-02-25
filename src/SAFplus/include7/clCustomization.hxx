/* This file contains configurable defaults for the entire set of SAFplus services */

/* Configuration parameters that are part of the API */
namespace SAFplus
{
    
};


/* Configuration parameters that are used internally */
namespace SAFplusI
{

    /* LOGGING */
    enum
    {
        LogDefaultFileBufferSize = 16*1024,
        LogDefaultMessageBufferSize = 16*1024,
    };

    /* CHECKPOINT */
    enum
    {
        CkptMinSize = 4*1024,
        CkptDefaultSize = 64*1024,

        CkptMinRows = 2,
        CkptDefaultRows = 256,


    };



};

