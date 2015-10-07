#ifndef GENERICGTKWIDGETINTERFACE_MDS06G4N
#define GENERICGTKWIDGETINTERFACE_MDS06G4N

#include <util/AutoReg.hpp>

namespace SafeLists {

struct GenericGtkWidgetInterface {
    // Get widget from tree according to name.
    // Signature:
    // < GetWidgetFromTree, std::string (name), StrongMsgPtr (out) >
    DUMMY_REG(GetWidgetFromTree,"GWI_GetWidgetFromTree");

    // Set notifier for output signals
    // Signature:
    // < SetNotifier, StrongMsgPtr >
    DUMMY_REG(SetNotifier,"GWI_SetNotifier");
};

struct GenericWidgetTrait {
    // Set wether widget is active.
    // Signature:
    // < SetActive, bool (value) >
    DUMMY_REG(SetActive,"GWI_GWT_SetActive");
};

struct GenericLabelTrait {
    // Set label value.
    // Signature:
    // < SetValue, std::string (value) >
    DUMMY_REG(SetValue,"GWI_GLT_SetValue");
};

struct GenericInputTrait {
    // Query input value (as string)
    // Signature:
    // < QueryValue, std::string (out value) >
    DUMMY_REG(QueryValue,"GWI_GIT_QueryValue");

    // Set input value (as string)
    // Signature:
    // < SetValue, std::string (in value) >
    DUMMY_REG(SetValue,"GWI_GIT_SetValue");
};

struct GenericButtonTrait {
    // Hook click event and set output number
    // Signature:
    // < HookClickEvent, int (out identifier) >
    DUMMY_REG(HookClickEvent,"GWI_GBT_HookClickEvent");

    // Output click event with identifier
    // Signature:
    // < OutClickEvent, int (identifier) >
    DUMMY_REG(OutClickEvent,"GWI_GBT_OutClickEvent");
};

}

#endif /* end of include guard: GENERICGTKWIDGETINTERFACE_MDS06G4N */
