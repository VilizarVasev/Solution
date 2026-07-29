#include "stdafx.h"
#include "QBRecords.h"

#include <assert.h>
#include <chrono>
#include <iostream>
#include <ratio>

int main(int argc, _TCHAR* argv[])
{
    using namespace std::chrono;

    // Build the original 1,000-record demonstration dataset.
    auto data = populateDummyData("testdata", 1000);

    // Preserve the original combined profiler for the interactive demo. The
    // automated benchmark measures these scenarios separately and repeatedly.
    auto startTimer = steady_clock::now();
    auto filteredSet = QBFindMatchingRecords(data, "column1", "testdata500");
    auto filteredSet2 = QBFindMatchingRecords(data, "column2", "24");
    std::cout << "profiler: " << double((steady_clock::now() - startTimer).count()) * steady_clock::period::num / steady_clock::period::den << std::endl;

    // Retain the original smoke test for the string-search result.
    assert(filteredSet.size() == 1);
	return 0;
}

