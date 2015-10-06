#ifndef GENERICGTKWIDGETINTERFACE_MDS06G4N
#define GENERICGTKWIDGETINTERFACE_MDS06G4N

#include <util/AutoReg.hpp>

struct GenericGtkWidgetInterface {
    // Get widget from tree according to name.
    // Signature:
    // < GetWidgetFromTree, std::string (name), StrongMsgPtr (out) >
    DUMMY_REG(GetWidgetFromTree,"GWI_GetWidgetFromTree");
};

#endif /* end of include guard: GENERICGTKWIDGETINTERFACE_MDS06G4N */
