#include "tunable.hpp"

// Define all tunable parameters
DEFINE_TUNABLE(Killer1_Bonus, 1461, 500, 3000);
DEFINE_TUNABLE(Killer2_Bonus, 831, 300, 2000);
DEFINE_TUNABLE(Counter_Bonus, 1003, 300, 2500);
DEFINE_TUNABLE(RFP_Threshold, 131, 50, 300);
DEFINE_TUNABLE(Aspiration_Window, 36, 10, 100);
DEFINE_TUNABLE(NMP_Reduction, 4, 2, 6);
DEFINE_TUNABLE(Futility_Threshold1, 312, 100, 800);
DEFINE_TUNABLE(Futility_Threshold2, 678, 300, 1500);
DEFINE_TUNABLE(Razor_Margin, 250, 100, 600);
DEFINE_TUNABLE(History_Bonus_A, 153, 50, 300);
DEFINE_TUNABLE(History_Bonus_B, 87, 30, 200);
DEFINE_TUNABLE(History_Bonus_C, 65, 10, 150);
DEFINE_TUNABLE(Capture_Bonus_A, 182, 50, 400);
DEFINE_TUNABLE(Capture_Bonus_B, 49, 10, 150);
DEFINE_TUNABLE(Capture_Bonus_C, 39, 5, 100);
DEFINE_TUNABLE(Time_Factor, 52, 30, 80);
DEFINE_TUNABLE(IIR_Depth, 4, 2, 8);
DEFINE_TUNABLE(IIR_Reduction, 2, 1, 4);
DEFINE_TUNABLE(Check_Extension, 1, 0, 2);

void register_all_tunables() {
    TunableManager& manager = TunableManager::instance();
    manager.register_param("Killer1_Bonus", &Killer1_Bonus);
    manager.register_param("Killer2_Bonus", &Killer2_Bonus);
    manager.register_param("Counter_Bonus", &Counter_Bonus);
    manager.register_param("RFP_Threshold", &RFP_Threshold);
    manager.register_param("Aspiration_Window", &Aspiration_Window);
    manager.register_param("NMP_Reduction", &NMP_Reduction);
    manager.register_param("Futility_Threshold1", &Futility_Threshold1);
    manager.register_param("Futility_Threshold2", &Futility_Threshold2);
    manager.register_param("Razor_Margin", &Razor_Margin);
    manager.register_param("History_Bonus_A", &History_Bonus_A);
    manager.register_param("History_Bonus_B", &History_Bonus_B);
    manager.register_param("History_Bonus_C", &History_Bonus_C);
    manager.register_param("Capture_Bonus_A", &Capture_Bonus_A);
    manager.register_param("Capture_Bonus_B", &Capture_Bonus_B);
    manager.register_param("Capture_Bonus_C", &Capture_Bonus_C);
    manager.register_param("Time_Factor", &Time_Factor);
    manager.register_param("IIR_Depth", &IIR_Depth);
	manager.register_param("IIR_Reduction", &IIR_Reduction);
    manager.register_param("Check_Extension", &Check_Extension);
}
