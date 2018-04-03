copy "deps\\imgui-sfml\\imconfig-SFML.h" "deps\\imgui"
del "deps\\imgui\\imconfig.h"
rename "deps\\imgui\\imconfig-SFML.h" "imconfig.h"