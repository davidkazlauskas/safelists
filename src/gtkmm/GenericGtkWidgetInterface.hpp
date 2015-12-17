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

    // Set wether widget is visible.
    // Signature:
    // < SetVisible, bool (value) >
    DUMMY_REG(SetVisible,"GWI_GWT_SetVisible");
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

    // Set text for the button.
    // Signature:
    // < SetButtonText, const std::string (label) >
    DUMMY_REG(SetButtonText,"GWI_GBT_SetButtonText");
};

struct GenericNotebookTrait {
    // Set current tab
    // Signature:
    // < SetCurrentTab, int (number) >
    DUMMY_REG(SetCurrentTab,"GWI_GNT_SetCurrentTab");
};

struct GenericWindowTrait {
    // Set window position according
    // to gtkmm constants
    // Signature:
    // < SetWindowPosition, string (gtkmm enum val) >
    DUMMY_REG(SetWindowPosition,"GWI_GWNT_SetWindowPosition");

    // Set window title
    // Signature:
    // < SetWindowTitle, string (title) >
    DUMMY_REG(SetWindowTitle,"GWI_GWNT_SetWindowTitle");

    // Set window parent
    // Signature:
    // < SetWindowTitle, StrongMsgPtr (parent) >
    DUMMY_REG(SetWindowParent,"GWI_GWNT_SetWindowParent");
};

struct GenericMenuBarTrait {
    // Set model.
    // Uses lua stackless coroutines,
    // might need to port if feature is
    // absent.
    // Signature:
    // < SetModelStackless, StrongMsgPtr (model) >
    DUMMY_REG(SetModelStackless,"GWI_GMIT_SetModelStackless");

    // Query next node in a model.
    // Signature:
    // < QueryNextNode,
    //   int (id, -1 if holder),
    //   std::string (title),
    //   std::string (shortname)
    // >
    DUMMY_REG(QueryNextNode,"GWI_GMIT_QueryNextNode");

    // Sent out when index is clicked.
    // Signature:
    // < OutIndexClicked, int >
    DUMMY_REG(OutIndexClicked,"GWI_GMIT_OutIndexClicked");
};

}

#endif /* end of include guard: GENERICGTKWIDGETINTERFACE_MDS06G4N */
