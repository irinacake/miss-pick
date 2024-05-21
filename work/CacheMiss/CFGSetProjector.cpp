#include "CFGSetProjector.h"


p::interfaced_feature<ProjectedCFGColl> CFG_SET_PROJECTOR_FEATURE("CFG_SET_PROJECTOR_FEATURE", p::make<CfgSetProjectorProcessor>());


CfgSetProjectorProcessor::CfgSetProjectorProcessor(): CFGProcessor(reg) {
}

p::declare CfgSetProjectorProcessor::reg = p::init("CfgSetProjectorProcessor", Version(1, 0, 0))
    .make<CfgSetProjectorProcessor>()
    .extend<CFGProcessor>()
    .provide(CFG_SET_PROJECTOR_FEATURE)
    .require(DECODED_TEXT)
    .require(COLLECTED_CFG_FEATURE)
    .require(otawa::hard::CACHE_CONFIGURATION_FEATURE);


void CfgSetProjectorProcessor::processAll(WorkSpace *ws) {  
	sys::StopWatch mySW;
	
    auto maincfg = taskCFG();
    auto icache = hard::CACHE_CONFIGURATION_FEATURE.get(workspace())->instCache();

    setCount = icache->setCount();
    cfgsP.allocate(setCount);


    cout << "Bonjour has been called" << endl;

}



void CfgSetProjectorProcessor::dump(WorkSpace *ws, Output &out) {

}