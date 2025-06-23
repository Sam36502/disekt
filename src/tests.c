#include "../include/analysis.h"


ANA_TestResult TEST_ValidateChecksum(ANA_SectorInfo block) {

	//	Check if we have the metadata
	if (!block.has_transfer_info) return TEST_SKIPPED;

	if (block.checksum == DSK_Checksum(block.data)) return TEST_PASSED;
	else return TEST_FAILED;
}

ANA_TestResult TEST_ValidateDataFormat(ANA_SectorInfo block) {
	if (block.type != SECTYPE_BAM) return TEST_SKIPPED;

	// TODO: Check block formats
	return TEST_SKIPPED;
}


ANA_Test ANA_TEST_LIST[] = {
	{
		.name = "Validate Checksum",
		.test_func = &TEST_ValidateChecksum
	},
	{
		.name = "Check Data Format",
		.test_func = &TEST_ValidateDataFormat
	}
};

#define ANA_TEST_COUNT (sizeof(ANA_TEST_LIST) / sizeof(ANA_Test))
