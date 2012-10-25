
/** Call to initialize the python interpreter  The module appModule is loaded into the interpreter "from appmodule import *".
    If you want arguments in sys.argv pass them in argc, argv, otherwise pass 1 empty argument. */
extern void clPyGlueInit(char* appModule,void (*init_extensions[])(void),int argc, char**argv);
extern void clPyGlueStart(int block); /* Pass 1 if you want to block until quit signaled, else 0 */
extern void clPyGlueTerminate();
extern ClRcT clPyGlueHealthCheck();
extern void clPyGlueCsiSet(
                     const SaNameT       *compName,
                     SaAmfHAStateT       haState,
                     SaAmfCSIDescriptorT* csiDescriptor);
extern void clPyGlueCsiRemove(
                       const SaNameT       *compName,
                       const SaNameT       *csiName,
                       SaAmfCSIFlagsT      csiFlags);
