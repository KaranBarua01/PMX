#include "ui/LayoutPolicy.h"

int main()
{
    const auto hd=pmx::ui::LayoutPolicy::compute(1920,1080);
    if(!hd.supported || hd.contentWidth<=0 || hd.contentHeight<=0) return 1;
    if(hd.dialogWidth>560 || hd.dialogHeight>470) return 2;
    if(hd.presetCardWidth<220) return 3;

    const auto laptop=pmx::ui::LayoutPolicy::compute(1366,768);
    if(!laptop.supported) return 4;
    if(laptop.presetCardWidth<180) return 5;
    if(laptop.dialogWidth>1286 || laptop.dialogHeight>688) return 6;

    const auto tooSmall=pmx::ui::LayoutPolicy::compute(900,600);
    if(tooSmall.supported) return 7;
    return 0;
}
