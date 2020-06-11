#include <stdio.h>
#include "test21.h"


int main() {
	llvm::mca::SummaryView* summary = new llvm::mca::SummaryView();
	llvm::mca::HWEventListener* listener = (llvm::mca::HWEventListener*) summary;

	llvm::mca::HWInstructionEvent event;
	listener->onCycleBegin();
	listener->onCycleEnd();
	listener->onEvent(event);
	llvm::raw_ostream stream;
	summary->printView(stream);

	return 0;
}