#include "test21.h"
#include <stdio.h>

namespace llvm {
namespace mca {

SummaryView::SummaryView() : TotalCycles(0) {
  printf("Hello SummaryView %p\n", this);
}

void SummaryView::onEvent(const HWInstructionEvent &Event) {
  printf("SummaryView::onEvent %p\n", this);
}

void SummaryView::printView(raw_ostream &OS) const {
  printf("SummaryView::printView %p\n", this);
}

} // namespace mca
} // namespace llvm